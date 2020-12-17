#include <graphic/gfx/gfxcontext.h>
#include <graphic/pch.h>

void GfxContext::BeginTrackResourceState(GfxResourceBase& resource, D3D12_RESOURCE_STATES assumedInitialState)
{
    m_TrackedResourceStates[resource.GetD3D12Resource()] = assumedInitialState;
}

D3D12_RESOURCE_STATES GfxContext::GetCurrentResourceState(GfxResourceBase& resource) const
{
    return m_TrackedResourceStates.at(resource.GetD3D12Resource());
}

void GfxContext::Initialize(D3D12_COMMAND_LIST_TYPE cmdListType, std::string_view name)
{
    assert(m_CommandList == nullptr);
    m_CommandList = g_GfxCommandListsManager.GetCommandQueue(cmdListType).AllocateCommandList(name);

    // Just set default viewport/scissor vals that make sense
    const CD3DX12_VIEWPORT viewport{ 0.0f, 0.0f, (float)g_CommandLineOptions.m_WindowWidth, (float)g_CommandLineOptions.m_WindowHeight };
    const CD3DX12_RECT scissorRect{ 0, 0, (LONG)g_CommandLineOptions.m_WindowWidth, (LONG)g_CommandLineOptions.m_WindowHeight };
    m_CommandList->Dev()->RSSetViewports(1, &viewport);
    m_CommandList->Dev()->RSSetScissorRects(1, &scissorRect);

    static const D3D12_VERTEX_BUFFER_VIEW NullVertexBufferView = {};
    static const D3D12_INDEX_BUFFER_VIEW NullIndexBufferView = {};
    m_CommandList->Dev()->IASetVertexBuffers(0, 1, &NullVertexBufferView);
    m_CommandList->Dev()->IASetIndexBuffer(&NullIndexBufferView);

    m_PSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // allocate static heaps
    static const uint32_t NbStaticDescriptors = 128;
    m_StaticHeaps.set_capacity(NbStaticDescriptors);
    for (uint32_t i = 0; i < NbStaticDescriptors; ++i)
    {
        m_StaticHeaps.push_back();
    }
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GfxContext::AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, std::string_view debugName)
{
    // Get a heap from the front of buffer & re-init it to appropriate type
    GfxDescriptorHeap& toRet = m_StaticHeaps.front();
    toRet.Release();
    toRet.Initialize(type, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

    SetD3DDebugName(toRet.Dev(), debugName);

    // rotate circular buffer to the next element, so we always use the "oldest" heap
    assert(m_StaticHeaps.full());
    m_StaticHeaps.rotate(m_StaticHeaps.begin() + 1);

    return CD3DX12_CPU_DESCRIPTOR_HANDLE{ toRet.Dev()->GetCPUDescriptorHandleForHeapStart() };
}

void GfxContext::ClearRenderTargetView(GfxTexture& tex, const bbeVector4& clearColor)
{
    bbeProfileGPUFunction((*this));

    LazyTransitionResource(tex, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

    CD3D12_RENDER_TARGET_VIEW_DESC desc{ D3D12_RTV_DIMENSION_TEXTURE2D, tex.GetFormat() };
    CD3DX12_CPU_DESCRIPTOR_HANDLE descHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, __FUNCTION__);
    g_GfxManager.GetGfxDevice().Dev()->CreateRenderTargetView(tex.GetD3D12Resource(), &desc, descHeap);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;
    m_CommandList->Dev()->ClearRenderTargetView(descHeap, (const FLOAT*)&clearColor, numRects, pRects);
}

void GfxContext::ClearDSVInternal(GfxTexture& tex, float depth, uint8_t stencil, D3D12_CLEAR_FLAGS flags)
{
    bbeProfileGPUFunction((*this));

    LazyTransitionResource(tex, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

    D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
    desc.Format = tex.GetFormat();
    desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    CD3DX12_CPU_DESCRIPTOR_HANDLE descHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, __FUNCTION__);

    g_GfxManager.GetGfxDevice().Dev()->CreateDepthStencilView(tex.GetD3D12Resource(), &desc, descHeap);

    m_CommandList->Dev()->ClearDepthStencilView(descHeap, flags, depth, stencil, 0, nullptr);
}

void GfxContext::ClearUAVF(GfxTexture& tex, const bbeVector4& clearValue)
{
    bbeProfileGPUFunction((*this));

    LazyTransitionResource(tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

    SetShaderVisibleDescriptorHeap();

    CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, __FUNCTION__);

    GfxDescriptorHeapHandle destHandle = g_GfxGPUDescriptorAllocator.AllocateShaderVisible(1);

    g_GfxManager.GetGfxDevice().Dev()->CopyDescriptorsSimple(1, destHandle.m_CPUHandle, CPUDescHeap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_CommandList->Dev()->ClearUnorderedAccessViewFloat(destHandle.m_GPUHandle, CPUDescHeap, tex.GetD3D12Resource(), (const FLOAT*)&clearValue, 0, nullptr);
}

void GfxContext::ClearUAVU(GfxTexture& tex, const bbeVector4U& clearValue)
{
    bbeProfileGPUFunction((*this));

    LazyTransitionResource(tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

    SetShaderVisibleDescriptorHeap();

    CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, __FUNCTION__);

    GfxDescriptorHeapHandle destHandle = g_GfxGPUDescriptorAllocator.AllocateShaderVisible(1);
    g_GfxManager.GetGfxDevice().Dev()->CopyDescriptorsSimple(1, destHandle.m_CPUHandle, CPUDescHeap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_CommandList->Dev()->ClearUnorderedAccessViewUint(destHandle.m_GPUHandle, CPUDescHeap, tex.GetD3D12Resource(), (const UINT*)&clearValue, 0, nullptr);
}

template <uint32_t NbRTs>
void GfxContext::SetRTVHelper(GfxTexture* (&RTVs)[NbRTs])
{
    m_RTVs.resize(NbRTs);
    (&m_PSO.RTVFormats)->NumRenderTargets = NbRTs;

    for (uint32_t i = 0; i < NbRTs; ++i)
    {
        m_RTVs[i].m_Tex = RTVs[i];
        (&m_PSO.RTVFormats)->RTFormats[i] = RTVs[i]->GetFormat();

        // TODO: Any other dimensions for RTV other than Texture2D?
        const D3D12_RESOURCE_DESC resourceDesc = RTVs[i]->GetD3D12Resource()->GetDesc();
        CD3D12_RENDER_TARGET_VIEW_DESC RTVdesc{ D3D12_RTV_DIMENSION_TEXTURE2D, resourceDesc.Format };

        m_RTVs[i].m_CPUDescHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, __FUNCTION__);
        g_GfxManager.GetGfxDevice().Dev()->CreateRenderTargetView(RTVs[i]->GetD3D12Resource(), &RTVdesc, m_RTVs[i].m_CPUDescHeap);

        LazyTransitionResource(*m_RTVs[i].m_Tex, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
}

void GfxContext::SetRenderTarget(GfxTexture& rt1)
{
    GfxTexture* RTsArr[] = { &rt1 };
    SetRTVHelper(RTsArr);
}

void GfxContext::SetRenderTargets(GfxTexture& rt1, GfxTexture& rt2)
{
    GfxTexture* RTsArr[] = { &rt1, &rt2 };
    SetRTVHelper(RTsArr);
}

void GfxContext::SetRenderTargets(GfxTexture& rt1, GfxTexture& rt2, GfxTexture& rt3)
{
    GfxTexture* RTsArr[] = { &rt1, &rt2, &rt3 };
    SetRTVHelper(RTsArr);
}

void GfxContext::SetTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
    m_Topology = topology;
    m_PSO.PrimitiveTopologyType = GetD3D12PrimitiveTopologyType(topology);
}

void GfxContext::SetBlendStates(uint32_t renderTarget, const D3D12_RENDER_TARGET_BLEND_DESC& blendState)
{
    assert(renderTarget < _countof(D3D12_BLEND_DESC::RenderTarget));
    (&m_PSO.BlendState)->RenderTarget[renderTarget] = blendState;
}

void GfxContext::SetShader(const GfxShader& shader)
{
    m_Shaders[shader.m_Type] = &shader;

    D3D12_SHADER_BYTECODE* byteCodeArr[GfxShaderType_Count] = { &m_PSO.VS, &m_PSO.PS, &m_PSO.CS };
    byteCodeArr[shader.m_Type]->BytecodeLength = shader.m_ShaderBlob->GetBufferSize();
    byteCodeArr[shader.m_Type]->pShaderBytecode = shader.m_ShaderBlob->GetBufferPointer();
}

void GfxContext::SetDepthStencil(GfxTexture& tex)
{
    LazyTransitionResource(tex, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    m_PSO.DSVFormat = tex.GetFormat();
    m_DSV = &tex;
}

void GfxContext::SetVertexBuffer(GfxVertexBuffer& vBuffer)
{
    LazyTransitionResource(vBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    m_VertexBuffer = &vBuffer;
}

void GfxContext::SetIndexBuffer(GfxIndexBuffer& iBuffer)
{
    LazyTransitionResource(iBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    m_IndexBuffer = &iBuffer;
}

void GfxContext::SetRootSignature(std::size_t rootSigHash)
{
    const GfxRootSignature* rootSig = g_GfxRootSignatureManager.GetRootSig(rootSigHash);

    // do check to prevent needless descriptor heap allocation
    if (m_RootSig == rootSig)
        return;

    // Upon setting a new Root Signature, ALL staged resources will be set to null views!
    // Make sure you double check after setting a new root sig
    m_StagedResources = rootSig->m_Params;

    m_PSO.pRootSignature = rootSig->Dev();
    m_RootSig = rootSig;
}

void GfxContext::StageCBVInternal(const void* data, uint32_t bufferSize, uint32_t cbRegister, const char* CBName)
{
    assert((bbeIsAligned(bufferSize, 16))); // CBuffers are 16 bytes aligned
    const uint32_t sizeInBytes = AlignUp(bufferSize, 256); // CB size is required to be 256-byte aligned.

    const uint32_t RootOffset = 0; // TODO?
    CheckStagingResourceInputs(cbRegister, RootOffset, D3D12_DESCRIPTOR_RANGE_TYPE_CBV);

    StagedCBV& stagedCBV = m_StagedCBVs[cbRegister];
    stagedCBV.m_Name = CBName;

    // copy over bytes to upload later
    stagedCBV.m_CBBytes.resize(sizeInBytes);
    memcpy(stagedCBV.m_CBBytes.data(), data, bufferSize);
}

void GfxContext::SetShaderVisibleDescriptorHeap()
{
    if (!m_ShaderVisibleDescHeapsSet)
    {
        ID3D12DescriptorHeap* ppHeaps[] = { g_GfxGPUDescriptorAllocator.GetInternalShaderVisibleHeap().Dev() };
        m_CommandList->Dev()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
        m_ShaderVisibleDescHeapsSet = true;
    }
}

void GfxContext::CheckStagingResourceInputs(uint32_t rootIndex, uint32_t offset, D3D12_DESCRIPTOR_RANGE_TYPE type)
{
    assert(rootIndex < GfxRootSignature::MaxRootParams);
    assert(rootIndex < m_StagedResources.size());
    assert(offset < m_StagedResources[rootIndex].m_Descriptors.size());
    assert(m_StagedResources[rootIndex].m_Types[offset] == type);
}

static D3D12_SHADER_RESOURCE_VIEW_DESC CreateSRVDesc(GfxTexture& tex)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};

    const D3D12_RESOURCE_DESC resourceDesc = tex.GetD3D12Resource()->GetDesc();
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        SRVDesc = CD3D12_SHADER_RESOURCE_VIEW_DESC{ D3D12_SRV_DIMENSION_BUFFER, resourceDesc.Format };
        if (resourceDesc.Format == DXGI_FORMAT_UNKNOWN)
        {
            SRVDesc.Buffer.NumElements = tex.GetNumElements();
            SRVDesc.Buffer.StructureByteStride = tex.GetStructureByteStride();
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        SRVDesc = CD3D12_SHADER_RESOURCE_VIEW_DESC{ D3D12_SRV_DIMENSION_TEXTURE2D, resourceDesc.Format };
        SRVDesc.Texture2D.MipLevels = 1; // TODO: Mips
        break;

        // TODO: Other dimensions when needed
    default: assert(false);
    }

    return SRVDesc;
}

void GfxContext::StageSRV(GfxTexture& tex, uint32_t rootIndex, uint32_t offset)
{
    CheckStagingResourceInputs(rootIndex, offset, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

    const D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = CreateSRVDesc(tex);

    StaticString<128> heapDebugName = StringFormat("SRV. Index: %d, Offset: %d", rootIndex, offset);
    CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, heapDebugName.c_str());
    g_GfxManager.GetGfxDevice().Dev()->CreateShaderResourceView(tex.GetD3D12Resource(), &SRVDesc, CPUDescHeap);

    // TODO: Specific states for Pixel/Non-Pixel resources?
    LazyTransitionResource(tex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    StageDescriptor(CPUDescHeap, rootIndex, offset);
}

static D3D12_UNORDERED_ACCESS_VIEW_DESC CreateUAVDesc(GfxTexture& tex)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};

    const D3D12_RESOURCE_DESC resourceDesc = tex.GetD3D12Resource()->GetDesc();
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        UAVDesc = CD3D12_UNORDERED_ACCESS_VIEW_DESC{ D3D12_UAV_DIMENSION_BUFFER, resourceDesc.Format };
        if (resourceDesc.Format == DXGI_FORMAT_UNKNOWN)
        {
            UAVDesc.Buffer.NumElements = tex.m_NumElements;
            UAVDesc.Buffer.StructureByteStride = tex.m_StructureByteStride;
        }
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        UAVDesc = CD3D12_UNORDERED_ACCESS_VIEW_DESC{ D3D12_UAV_DIMENSION_TEXTURE2D, resourceDesc.Format };
        break;

        // TODO: Other dimensions when needed
    default: assert(false);
    }

    return UAVDesc;
}

void GfxContext::StageUAV(GfxTexture& tex, uint32_t rootIndex, uint32_t offset)
{
    CheckStagingResourceInputs(rootIndex, offset, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);

    const D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = CreateUAVDesc(tex);

    static ID3D12Resource* pCounterResource = nullptr; // TODO

    StaticString<128> heapDebugName = StringFormat("UAV. Index: %d, Offset: %d", rootIndex, offset);
    CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, heapDebugName.c_str());
    g_GfxManager.GetGfxDevice().Dev()->CreateUnorderedAccessView(tex.GetD3D12Resource(), pCounterResource, &UAVDesc, CPUDescHeap);

    LazyTransitionResource(tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    StageDescriptor(CPUDescHeap, rootIndex, offset);
}

void GfxContext::StageDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, uint32_t rootIndex, uint32_t offset)
{
    D3D12_CPU_DESCRIPTOR_HANDLE& destDesc = m_StagedResources[rootIndex].m_Descriptors[offset];

    if (destDesc.ptr == srcDescriptor.ptr)
        return;

    destDesc = srcDescriptor;
    m_StaleResourcesBitMap.set(rootIndex);
}

std::size_t GfxContext::GetPSOHash(bool forGraphicPSO)
{
    // Always get root sig Hash
    std::size_t finalHash = m_RootSig->m_Hash;

    if (forGraphicPSO)
    {
        // VS/PS Shaders
        if (m_Shaders[VS])
            boost::hash_combine(finalHash, m_Shaders[VS]->m_Hash);
        if (m_Shaders[PS])
            boost::hash_combine(finalHash, m_Shaders[PS]->m_Hash);

        // Blend States
        boost::hash_combine(finalHash, GenericTypeHash(m_PSO.BlendState));

        // Rasterizer States
        boost::hash_combine(finalHash, GenericTypeHash(m_PSO.RasterizerState));

        // Depth Stencil State
        // Note: Have to manually extract each Depth Stencil State 1-by-1 to achieve consistent hash and I have no fking idea why
        const CD3DX12_DEPTH_STENCIL_DESC1 depthStencilState = (CD3DX12_DEPTH_STENCIL_DESC1)m_PSO.DepthStencilState;
        if (depthStencilState.DepthEnable)
        {
            boost::hash_combine(finalHash, depthStencilState.DepthWriteMask);
            boost::hash_combine(finalHash, depthStencilState.DepthFunc);
            boost::hash_combine(finalHash, depthStencilState.StencilEnable);
            boost::hash_combine(finalHash, depthStencilState.StencilReadMask);
            boost::hash_combine(finalHash, depthStencilState.StencilWriteMask);
            boost::hash_combine(finalHash, depthStencilState.FrontFace.StencilFailOp);
            boost::hash_combine(finalHash, depthStencilState.FrontFace.StencilDepthFailOp);
            boost::hash_combine(finalHash, depthStencilState.FrontFace.StencilPassOp);
            boost::hash_combine(finalHash, depthStencilState.FrontFace.StencilFunc);
            boost::hash_combine(finalHash, depthStencilState.BackFace.StencilFailOp);
            boost::hash_combine(finalHash, depthStencilState.BackFace.StencilDepthFailOp);
            boost::hash_combine(finalHash, depthStencilState.BackFace.StencilPassOp);
            boost::hash_combine(finalHash, depthStencilState.BackFace.StencilFunc);
            boost::hash_combine(finalHash, depthStencilState.DepthBoundsTestEnable);
        }

        // DSV Format
        boost::hash_combine(finalHash, GenericTypeHash(m_PSO.DSVFormat));

        // Vertex Input Layout
        boost::hash_combine(finalHash, m_VertexFormat->GetHash());

        // Topology
        boost::hash_combine(finalHash, m_Topology);
        boost::hash_combine(finalHash, GenericTypeHash(m_PSO.PrimitiveTopologyType));

        // RTV Formats
        boost::hash_combine(finalHash, m_RTVs.size());
        for (uint32_t i = 0; i < m_RTVs.size(); ++i)
        {
            boost::hash_combine(finalHash, m_RTVs[i].m_Tex->GetFormat());
        }

        // Sample Descriptors
        boost::hash_combine(finalHash, GenericTypeHash(m_PSO.SampleDesc));
    }
    else
    {
        // We only need to hash CS shader
        boost::hash_combine(finalHash, m_Shaders[CS]->m_Hash);
    }

    return finalHash;
}

void GfxContext::PrepareGraphicsStates()
{
    //bbeProfileFunction();

    assert(m_RootSig && m_RootSig->Dev());
    assert(m_CommandList);
    assert(!m_RTVs.empty());
    assert(m_Shaders[VS]);

    if (m_VertexBuffer)
    {
        assert(m_VertexFormat);
        assert(m_VertexBuffer->GetD3D12Resource());
        assert(m_VertexBuffer->GetStrideInBytes() > 0);
        assert(m_VertexBuffer->GetNumVertices() > 0);
    }

    if (m_IndexBuffer)
    {
        assert(m_IndexBuffer->GetNumIndices() > 0);
        assert(m_IndexBuffer->GetFormat() == DXGI_FORMAT_R16_UINT || m_IndexBuffer->GetFormat() == DXGI_FORMAT_R32_UINT);
    }

    for (uint32_t i = 0; i < m_RTVs.size(); ++i)
    {
        assert(m_RTVs[i].m_Tex != nullptr);
        assert(m_RTVs[i].m_CPUDescHeap.ptr != 0);
        assert((&m_PSO.RTVFormats)->RTFormats[i] != DXGI_FORMAT_UNKNOWN);
    }

    std::size_t psoHash = GetPSOHash(true);
    if (m_PSOHash != psoHash)
    {
        bbeProfile("Set PSO Params");

        m_PSOHash = psoHash;

        // Root Sig
        m_CommandList->Dev()->SetGraphicsRootSignature(m_RootSig->Dev());

        // PSO
        m_CommandList->Dev()->SetPipelineState(g_GfxPSOManager.GetPSO(*this, m_PSO.GraphicsDescV0(), m_PSOHash));

        // Input Assembler
        m_CommandList->Dev()->IASetPrimitiveTopology(m_Topology);

        // RTVs
        InplaceArray<D3D12_CPU_DESCRIPTOR_HANDLE, 3> rtvHandles;
        for (const RTVDesc& rtvDesc : m_RTVs)
        {
            rtvHandles.push_back(rtvDesc.m_CPUDescHeap);
        }

        // DSV
        D3D12_CPU_DESCRIPTOR_HANDLE DSVDescHandle{};
        if (m_DSV)
        {
            DSVDescHandle = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, "DSV");

            D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc{ m_DSV->GetFormat(), D3D12_DSV_DIMENSION_TEXTURE2D, D3D12_DSV_FLAG_READ_ONLY_DEPTH };
            g_GfxManager.GetGfxDevice().Dev()->CreateDepthStencilView(m_DSV->GetD3D12Resource(), &DSVDesc, DSVDescHandle);
        }

        // Set Render Targets
        m_CommandList->Dev()->OMSetRenderTargets(m_RTVs.size(), rtvHandles.data(), FALSE, m_DSV ? &DSVDescHandle : nullptr);
    }

    std::size_t buffersHash = std::hash<void*>{}(m_VertexBuffer);
    boost::hash_combine(buffersHash, m_IndexBuffer);
    if (m_LastBuffersHash != buffersHash)
    {
        m_LastBuffersHash = buffersHash;

        if (m_VertexBuffer)
        {
            bbeProfile("Set Buffer Params");

            D3D12_VERTEX_BUFFER_VIEW vBufferView{};
            vBufferView.BufferLocation = m_VertexBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
            vBufferView.StrideInBytes = m_VertexBuffer->GetStrideInBytes();
            vBufferView.SizeInBytes = m_VertexBuffer->GetNumVertices() * m_VertexBuffer->GetStrideInBytes();

            m_CommandList->Dev()->IASetVertexBuffers(0, 1, &vBufferView);

            if (m_IndexBuffer)
            {
                D3D12_INDEX_BUFFER_VIEW indexBufferView{};
                indexBufferView.BufferLocation = m_IndexBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
                indexBufferView.Format = DXGI_FORMAT_R16_UINT;
                indexBufferView.SizeInBytes = m_IndexBuffer->GetNumIndices() * 2;

                m_CommandList->Dev()->IASetIndexBuffer(&indexBufferView);
            }
        }
    }
}

void GfxContext::PrepareComputeStates()
{
    //bbeProfileFunction();

    assert(m_RootSig && m_RootSig->Dev());
    assert(m_CommandList);
    assert(m_Shaders[CS]);

    std::size_t psoHash = GetPSOHash(false);
    if (m_PSOHash != psoHash)
    {
        bbeProfile("Set PSO Params");

        m_PSOHash = psoHash;

        m_CommandList->Dev()->SetComputeRootSignature(m_RootSig->Dev());

        // PSO
        m_CommandList->Dev()->SetPipelineState(g_GfxPSOManager.GetPSO(*this, m_PSO.ComputeDescV0(), m_PSOHash));
    }
}

void GfxContext::FlushResourceBarriers()
{
    //bbeProfileFunction();

    assert(m_CommandList);

    if (m_ResourceBarriers.size() > 0)
    {
        m_CommandList->Dev()->ResourceBarrier(m_ResourceBarriers.size(), m_ResourceBarriers.data());
        m_ResourceBarriers.clear();
    }
}

void GfxContext::UAVBarrier(GfxResourceBase& resource)
{
    const D3D12_RESOURCE_STATES currentState = m_TrackedResourceStates.at(resource.GetD3D12Resource());
    assert((currentState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);

    m_ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(resource.GetD3D12Resource()));
}

void GfxContext::TransitionResource(GfxResourceBase& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
    if (D3D12_RESOURCE_STATES& currentState = m_TrackedResourceStates.at(resource.GetD3D12Resource());
        currentState != newState)
    {
        m_ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource.GetD3D12Resource(), currentState, newState));
        currentState = newState;

        // Insert UAV barrier on SRV<->UAV transitions
        if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            UAVBarrier(resource);
    }
    else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        UAVBarrier(resource);
    }

    if (flushImmediate)
        FlushResourceBarriers();
}

void GfxContext::LazyTransitionResource(GfxResourceBase& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
    if (!m_TrackedResourceStates.contains(resource.GetD3D12Resource()))
        BeginTrackResourceState(resource, newState);

    TransitionResource(resource, newState, flushImmediate);
}

static void CreateNullCBV(D3D12_CPU_DESCRIPTOR_HANDLE destDesc)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    D3D12_CONSTANT_BUFFER_VIEW_DESC nullCBDesc = {};
    gfxDevice.Dev()->CreateConstantBufferView(&nullCBDesc, destDesc);
}

static void CreateNullSRV(D3D12_CPU_DESCRIPTOR_HANDLE destDesc)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    CD3D12_SHADER_RESOURCE_VIEW_DESC desc{ D3D12_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 1 };
    gfxDevice.Dev()->CreateShaderResourceView(nullptr, &desc, destDesc);
}

static void CreateNullUAV(D3D12_CPU_DESCRIPTOR_HANDLE destDesc)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    CD3D12_UNORDERED_ACCESS_VIEW_DESC desc{ D3D12_UAV_DIMENSION_BUFFER, DXGI_FORMAT_R8G8B8A8_UNORM };
    gfxDevice.Dev()->CreateUnorderedAccessView(nullptr, nullptr, &desc, destDesc);
}

static void CreateNullView(D3D12_DESCRIPTOR_RANGE_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE destHandle)
{
    switch (type)
    {
    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV: CreateNullSRV(destHandle); break;
    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV: CreateNullUAV(destHandle); break;
    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV: CreateNullCBV(destHandle); break;

    default: assert(0);
    }
}

void GfxContext::CommitStagedResources(RootDescTableSetter rootDescSetterFunc)
{
    //bbeProfileFunction();

    FlushResourceBarriers();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    // Upload CBV bytes
    for (uint32_t i = 0; i < _countof(m_StagedCBVs); ++i)
    {
        const uint32_t cbRegister = i;
        const StagedCBV& stagedCBV = m_StagedCBVs[i];

        if (stagedCBV.m_CBBytes.empty())
            continue;

        // Create upload heap for constant buffer
        D3D12MA::Allocation* uploadHeap = g_GfxMemoryAllocator.AllocateVolatile(D3D12_HEAP_TYPE_UPLOAD, CD3DX12_RESOURCE_DESC1::Buffer(stagedCBV.m_CBBytes.size()));
        assert(uploadHeap);

        // init desc heap for cbuffer
        CD3DX12_CPU_DESCRIPTOR_HANDLE descHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, StringFormat("CBV: %s", stagedCBV.m_Name));

        // Describe and create a constant buffer view.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{ uploadHeap->GetResource()->GetGPUVirtualAddress(), (uint32_t)stagedCBV.m_CBBytes.size() };
        gfxDevice.Dev()->CreateConstantBufferView(&cbvDesc, descHeap);

        void* mappedMemory = nullptr;
        DX12_CALL(uploadHeap->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedMemory)));
        memcpy(mappedMemory, stagedCBV.m_CBBytes.data(), stagedCBV.m_CBBytes.size());
        uploadHeap->GetResource()->Unmap(0, nullptr);

        const uint32_t RootOffset = 0; // TODO?
        StageDescriptor(descHeap, cbRegister, RootOffset);
    }

    // Resources
    if (m_StaleResourcesBitMap.none())
        return;
    
    uint32_t numHeapsNeeded = 0;
    RunOnAllBits(m_StaleResourcesBitMap.to_ulong(), [&](uint32_t rootIndex) { numHeapsNeeded += m_StagedResources[rootIndex].m_Descriptors.size(); });
    assert(numHeapsNeeded > 0);

    // allocate shader visible heaps
    GfxDescriptorHeapHandle destHandle = g_GfxGPUDescriptorAllocator.AllocateShaderVisible(numHeapsNeeded);

    SetShaderVisibleDescriptorHeap();

    RunOnAllBits(m_StaleResourcesBitMap.to_ulong(), [&](uint32_t rootIndex)
    {
        // set desc heap for this table
        (m_CommandList->Dev()->*rootDescSetterFunc)(rootIndex, destHandle.m_GPUHandle);

        const GfxRootSignature::StagedResourcesDescriptors& resourceDesc = m_StagedResources[rootIndex];

        // copy descriptors to shader visible heaps 1 by 1
        // TODO: is this ineffcient? Better to use the full "CopyDescriptors" method?
        for (uint32_t i = 0; i < resourceDesc.m_Descriptors.size(); ++i)
        {
            assert(resourceDesc.m_Descriptors[i].ptr != 0);
            gfxDevice.Dev()->CopyDescriptorsSimple(1, destHandle.m_CPUHandle, resourceDesc.m_Descriptors[i], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            static const uint32_t descriptorSize = gfxDevice.Dev()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            destHandle.Offset(1, descriptorSize);
        }
    });
}

void GfxContext::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
    PrepareGraphicsStates();
    CommitStagedResources(&D3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
    m_CommandList->Dev()->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
    PostDraw();
}

void GfxContext::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation)
{
    //bbeProfileFunction();

    assert(m_VertexBuffer);
    assert(m_IndexBuffer);

    PrepareGraphicsStates();
    CommitStagedResources(&D3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
    m_CommandList->Dev()->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    PostDraw();
}

void GfxContext::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    PrepareComputeStates();
    CommitStagedResources(&D3D12GraphicsCommandList::SetComputeRootDescriptorTable);
    m_CommandList->Dev()->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    PostDraw();
}

void GfxContext::PostDraw()
{
    m_StaleResourcesBitMap.reset();

    for (uint32_t i = 0; i < gs_MaxRootSigParams; ++i)
    {
        m_StagedCBVs[i].m_CBBytes.clear();
    }
}
