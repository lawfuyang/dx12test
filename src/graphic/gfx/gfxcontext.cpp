#include <graphic/gfx/gfxcontext.h>
#include <graphic/pch.h>

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

    if (!debugName.empty())
        SetD3DDebugName(toRet.Dev(), debugName);

    // rotate circular buffer to the next element, so we always use the "oldest" heap
    assert(m_StaticHeaps.full());
    m_StaticHeaps.rotate(m_StaticHeaps.begin() + 1);

    return CD3DX12_CPU_DESCRIPTOR_HANDLE{ toRet.Dev()->GetCPUDescriptorHandleForHeapStart() };
}

void GfxContext::ClearRenderTargetView(GfxTexture& tex, const bbeVector4& clearColor)
{
    bbeProfileGPUFunction((*this));

    TransitionResource(tex, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

    CD3D12_RENDER_TARGET_VIEW_DESC desc{ D3D12_RTV_DIMENSION_TEXTURE2D, tex.GetFormat() };
    CD3DX12_CPU_DESCRIPTOR_HANDLE descHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    g_GfxManager.GetGfxDevice().Dev()->CreateRenderTargetView(tex.GetD3D12Resource(), &desc, descHeap);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;
    m_CommandList->Dev()->ClearRenderTargetView(descHeap, (const FLOAT*)&clearColor, numRects, pRects);
}

void GfxContext::ClearDSVInternal(GfxTexture& tex, float depth, uint8_t stencil, D3D12_CLEAR_FLAGS flags)
{
    bbeProfileGPUFunction((*this));

    TransitionResource(tex, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

    D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
    desc.Format = tex.GetFormat();
    desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    CD3DX12_CPU_DESCRIPTOR_HANDLE descHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    g_GfxManager.GetGfxDevice().Dev()->CreateDepthStencilView(tex.GetD3D12Resource(), &desc, descHeap);

    m_CommandList->Dev()->ClearDepthStencilView(descHeap, flags, depth, stencil, 0, nullptr);
}

void GfxContext::ClearUAVF(GfxTexture& tex, const bbeVector4& clearValue)
{
    TransitionResource(tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
    SetShaderVisibleDescriptorHeap();

    CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    GfxDescriptorHeapHandle destHandle = g_GfxGPUDescriptorAllocator.AllocateShaderVisible(1);
    g_GfxManager.GetGfxDevice().Dev()->CopyDescriptorsSimple(1, destHandle.m_CPUHandle, CPUDescHeap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_CommandList->Dev()->ClearUnorderedAccessViewFloat(destHandle.m_GPUHandle, CPUDescHeap, tex.GetD3D12Resource(), (const FLOAT*)&clearValue, 0, nullptr);
}

void GfxContext::ClearUAVU(GfxTexture& tex, const bbeVector4U& clearValue)
{
    TransitionResource(tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
    SetShaderVisibleDescriptorHeap();

    CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

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

        m_RTVs[i].m_CPUDescHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        g_GfxManager.GetGfxDevice().Dev()->CreateRenderTargetView(RTVs[i]->GetD3D12Resource(), &RTVdesc, m_RTVs[i].m_CPUDescHeap);
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
    m_PSO.DSVFormat = tex.GetFormat();
    m_DSV = &tex;
}

void GfxContext::SetRootSignature(GfxRootSignature& rootSig)
{
    // do check to prevent needless descriptor heap allocation
    if (m_RootSig == &rootSig)
        return;

    // Upon setting a new Root Signature, ALL staged resources will be set to null views!
    // Make sure you double check after setting a new root sig
    m_StagedResources = rootSig.m_Params;

    //m_PSO.SetRootSignature(rootSig);
    m_PSO.pRootSignature = rootSig.Dev();
    m_RootSig = &rootSig;
    m_CommandList->Dev()->SetGraphicsRootSignature(rootSig.Dev());
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

void GfxContext::StageSRV(GfxTexture& tex, uint32_t rootIndex, uint32_t offset)
{
    CheckStagingResourceInputs(rootIndex, offset, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVdesc{};

    const D3D12_RESOURCE_DESC resourceDesc = tex.GetD3D12Resource()->GetDesc();
    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        SRVdesc = CD3D12_SHADER_RESOURCE_VIEW_DESC{ D3D12_SRV_DIMENSION_BUFFER, resourceDesc.Format };
        break;
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        SRVdesc = CD3D12_SHADER_RESOURCE_VIEW_DESC{ D3D12_SRV_DIMENSION_TEXTURE2D, resourceDesc.Format };
        break;

        // TODO: Other dimensions when needed
    default: assert(false);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescHeap = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, StringFormat("SRV: %s", GetD3DDebugName(tex.GetD3D12Resource())));
    g_GfxManager.GetGfxDevice().Dev()->CreateShaderResourceView(tex.GetD3D12Resource(), &SRVdesc, CPUDescHeap);

    StageDescriptor(CPUDescHeap, rootIndex, offset);
}

void GfxContext::StageUAV(GfxTexture& tex, uint32_t rootIndex, uint32_t offset)
{
    CheckStagingResourceInputs(rootIndex, offset, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);

    // TODO
}

void GfxContext::StageDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, uint32_t rootIndex, uint32_t offset)
{
    D3D12_CPU_DESCRIPTOR_HANDLE& destDesc = m_StagedResources[rootIndex].m_Descriptors[offset];

    if (destDesc.ptr == srcDescriptor.ptr)
        return;

    destDesc = srcDescriptor;
    m_StaleResourcesBitMap.set(rootIndex);
}

std::size_t GfxContext::GetPSOHash()
{
    std::size_t finalHash = 0;

    // RootSig
    boost::hash_combine(finalHash, m_RootSig->m_Hash);

    if (m_Shaders[VS])
    {
        // VS/PS Shaders
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

        // Primitive Topology
        boost::hash_combine(finalHash, GenericTypeHash(m_PSO.PrimitiveTopologyType));

        // RTV Formats
        boost::hash_combine(finalHash, m_RTVs.size());
        for (uint32_t i = 0; i < m_RTVs.size(); ++i)
        {
            boost::hash_combine(finalHash, m_RTVs[i].m_Tex->GetFormat());
        }

        // Sample Desccriptors
        boost::hash_combine(finalHash, GenericTypeHash(m_PSO.SampleDesc));
    }
    else if (m_Shaders[CS])
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

    if (m_VertexBuffer)
    {
        assert(m_VertexFormat);
        assert(m_VertexBuffer->GetD3D12Resource());
        assert(m_VertexBuffer->GetStrideInBytes() > 0);
        assert(m_VertexBuffer->GetNumVertices() > 0);
    }

    for (uint32_t i = 0; i < m_RTVs.size(); ++i)
    {
        assert(m_RTVs[i].m_Tex != nullptr);
        assert(m_RTVs[i].m_CPUDescHeap.ptr != 0);
        assert((&m_PSO.RTVFormats)->RTFormats[i] != DXGI_FORMAT_UNKNOWN);
    }

    std::size_t psoHash = GetPSOHash();
    if (m_PSOHash != psoHash)
    {
        bbeProfile("Set PSO Params");

        m_PSOHash = psoHash;

        // PSO
        m_CommandList->Dev()->SetPipelineState(g_GfxPSOManager.GetPSO(*this, m_PSO, m_PSOHash));

        // Input Assembler
        m_CommandList->Dev()->IASetPrimitiveTopology(GetD3D12PrimitiveTopology(m_PSO.PrimitiveTopologyType));

        // RTVs
        InplaceArray<D3D12_CPU_DESCRIPTOR_HANDLE, 3> rtvHandles;
        for (const RTVDesc& rtvDesc : m_RTVs)
        {
            TransitionResource(*rtvDesc.m_Tex, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
            rtvHandles.push_back(rtvDesc.m_CPUDescHeap);
        }

        // DSV
        D3D12_CPU_DESCRIPTOR_HANDLE DSVDescHandle{};
        if (m_DSV)
        {
            TransitionResource(*m_DSV, D3D12_RESOURCE_STATE_DEPTH_WRITE, false);
            DSVDescHandle = AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, StringFormat("DSV: %s", GetD3DDebugName(m_DSV->GetD3D12Resource())));

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

    std::size_t psoHash = GetPSOHash();
    if (m_PSOHash != psoHash)
    {
        bbeProfile("Set PSO Params");

        m_PSOHash = psoHash;

        // PSO
        m_CommandList->Dev()->SetPipelineState(g_GfxPSOManager.GetPSO(*this, m_PSO, m_PSOHash));
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

void GfxContext::TransitionResource(GfxHazardTrackedResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
    const D3D12_RESOURCE_STATES oldState = resource.m_CurrentResourceState;

    if (oldState != newState)
    {
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarriers.emplace_back(D3D12_RESOURCE_BARRIER{});

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = resource.GetD3D12Resource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = oldState;
        BarrierDesc.Transition.StateAfter = newState;

        // Insert UAV barrier on SRV<->UAV transitions.
        if (oldState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS || newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            m_ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(resource.GetD3D12Resource()));

        // Check to see if we already started the transition
        if (newState == resource.m_TransitioningState)
        {
            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
            resource.m_TransitioningState = GfxHazardTrackedResource::INVALID_STATE;
        }
        else
            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        resource.m_CurrentResourceState = newState;
    }
    else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        m_ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(resource.GetD3D12Resource()));

    if (flushImmediate)
        FlushResourceBarriers();
}

void GfxContext::BeginResourceTransition(GfxHazardTrackedResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
    assert(m_CommandList);

    // If it's already transitioning, finish that transition
    if (resource.m_TransitioningState != GfxHazardTrackedResource::INVALID_STATE)
        TransitionResource(resource, resource.m_TransitioningState, flushImmediate);

    const D3D12_RESOURCE_STATES oldState = resource.m_CurrentResourceState;

    // verify UAV states
    if (m_CommandList->GetType() == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        static const uint32_t VALID_STATES =
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_COPY_DEST |
            D3D12_RESOURCE_STATE_COPY_SOURCE;

        assert((oldState & VALID_STATES) == oldState);
        assert((newState & VALID_STATES) == newState);
    }

    if (oldState != newState)
    {
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarriers.emplace_back(D3D12_RESOURCE_BARRIER{});

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = resource.GetD3D12Resource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = oldState;
        BarrierDesc.Transition.StateAfter = newState;
        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

        resource.m_TransitioningState = newState;
    }

    if (flushImmediate)
        FlushResourceBarriers();
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

void GfxContext::CommitStagedResources()
{
    //bbeProfileFunction();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    // Upload CBV bytes
    for (uint32_t i = 0; i < _countof(m_StagedCBVs); ++i)
    {
        const uint32_t cbRegister = i;
        const StagedCBV& stagedCBV = m_StagedCBVs[i];

        if (stagedCBV.m_CBBytes.empty())
            continue;

        // Create upload heap for constant buffer
        D3D12MA::Allocation* uploadHeap = GfxHeap::Create(D3D12_HEAP_TYPE_UPLOAD, CD3DX12_RESOURCE_DESC1::Buffer(stagedCBV.m_CBBytes.size()), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, stagedCBV.m_Name);
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

        // free Upload heap 1 frame later
        g_GfxManager.AddGraphicCommand([uploadHeap]() { GfxHeap::Release(uploadHeap); });
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
        m_CommandList->Dev()->SetGraphicsRootDescriptorTable(rootIndex, destHandle.m_GPUHandle);

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
    FlushResourceBarriers();
    CommitStagedResources();
    m_CommandList->Dev()->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
    PostDraw();
}

void GfxContext::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation)
{
    //bbeProfileFunction();

    assert(m_IndexBuffer);

    PrepareGraphicsStates();
    FlushResourceBarriers();
    CommitStagedResources();
    m_CommandList->Dev()->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    PostDraw();
}

void GfxContext::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    PrepareComputeStates();
    FlushResourceBarriers();
    CommitStagedResources();
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
