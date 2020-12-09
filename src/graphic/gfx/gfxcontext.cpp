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
}

void GfxContext::ClearRenderTargetView(GfxTexture& tex, const bbeVector4& clearColor)
{
    bbeProfileGPUFunction((*this));

    TransitionResource(tex, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;
    m_CommandList->Dev()->ClearRenderTargetView(tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), (const FLOAT*)&clearColor, numRects, pRects);
}

void GfxContext::ClearDepth(GfxTexture& tex, float depth)
{
    bbeProfileGPUFunction((*this));
    m_CommandList->Dev()->ClearDepthStencilView(tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void GfxContext::ClearDepthStencil(GfxTexture& tex, float depth, uint8_t stencil)
{
    bbeProfileGPUFunction((*this));
    m_CommandList->Dev()->ClearDepthStencilView(tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}

void GfxContext::ClearUAVF(GfxTexture& tex, const bbeVector4& clearValue)
{
    TransitionResource(tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
    SetDescriptorHeapIfNeeded();

    GfxDescriptorHeapHandle destHandle = g_GfxGPUDescriptorAllocator.AllocateVolatile(D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1);
    g_GfxManager.GetGfxDevice().Dev()->CopyDescriptorsSimple(1, destHandle.m_CPUHandle, tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_CommandList->Dev()->ClearUnorderedAccessViewFloat(destHandle.m_GPUHandle, tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), tex.GetD3D12Resource(), (const FLOAT*)&clearValue, 0, nullptr);
}

void GfxContext::ClearUAVU(GfxTexture& tex, const bbeVector4U& clearValue)
{
    TransitionResource(tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
    SetDescriptorHeapIfNeeded();

    GfxDescriptorHeapHandle destHandle = g_GfxGPUDescriptorAllocator.AllocateVolatile(D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1);
    g_GfxManager.GetGfxDevice().Dev()->CopyDescriptorsSimple(1, destHandle.m_CPUHandle, tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_CommandList->Dev()->ClearUnorderedAccessViewUint(destHandle.m_GPUHandle, tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), tex.GetD3D12Resource(), (const UINT*)&clearValue, 0, nullptr);
}

template <uint32_t NbRTs>
void GfxContext::SetRTVHelper(GfxTexture* (&RTVs)[NbRTs])
{
    m_RTVs.resize(NbRTs);
    (&m_PSO.RTVFormats)->NumRenderTargets = NbRTs;

    for (uint32_t i = 0; i < NbRTs; ++i)
    {
        m_RTVs[i] = RTVs[i];
        (&m_PSO.RTVFormats)->RTFormats[i] = RTVs[i]->GetFormat();
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

void GfxContext::SetDescriptorHeapIfNeeded()
{
    if (!m_DescHeapsSet)
    {
        ID3D12DescriptorHeap* ppHeaps[] = { g_GfxGPUDescriptorAllocator.GetInternalHeap(D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE).Dev() };
        m_CommandList->Dev()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
        m_DescHeapsSet = true;
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
    StageDescriptor(tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), rootIndex, offset);
}

void GfxContext::StageUAV(GfxTexture& tex, uint32_t rootIndex, uint32_t offset)
{
    CheckStagingResourceInputs(rootIndex, offset, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
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
            boost::hash_combine(finalHash, m_RTVs[i]->GetFormat());
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
        assert(m_VertexBuffer->GetSizeInBytes() > 0);
    }

    for (uint32_t i = 0; i < m_RTVs.size(); ++i)
    {
        assert(m_RTVs[i] != nullptr);
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

        // Output Merger
        InplaceArray<D3D12_CPU_DESCRIPTOR_HANDLE, 3> rtvHandles{ m_RTVs.size() };
        for (uint32_t i = 0; i < m_RTVs.size(); ++i)
        {
            rtvHandles[i] = m_RTVs[i]->GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart();

            TransitionResource(*m_RTVs[i], D3D12_RESOURCE_STATE_RENDER_TARGET, false);
        }
        m_CommandList->Dev()->OMSetRenderTargets(m_RTVs.size(), rtvHandles.data(), FALSE, m_DSV ? &m_DSV->GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart() : nullptr);
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
            vBufferView.SizeInBytes = m_VertexBuffer->GetSizeInBytes();

            m_CommandList->Dev()->IASetVertexBuffers(0, 1, &vBufferView);

            if (m_IndexBuffer)
            {
                D3D12_INDEX_BUFFER_VIEW indexBufferView{};
                indexBufferView.BufferLocation = m_IndexBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
                indexBufferView.Format = m_IndexBuffer->GetFormat();
                indexBufferView.SizeInBytes = m_IndexBuffer->GetSizeInBytes();

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

void GfxContext::CreateRTV(const GfxTexture&)
{

}

void GfxContext::CreateDSV(const GfxTexture&)
{

}

void GfxContext::CreateSRV(const GfxTexture&)
{

}

void GfxContext::CreateUAV(const GfxTexture&)
{

}

void GfxContext::TransitionResource(GfxHazardTrackedResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
    const D3D12_RESOURCE_STATES oldState = resource.m_CurrentResourceState;

    if (oldState != newState)
    {
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarriers.emplace_back(D3D12_RESOURCE_BARRIER{});

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = resource.m_HazardTrackedResource;
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = oldState;
        BarrierDesc.Transition.StateAfter = newState;

        // Insert UAV barrier on SRV<->UAV transitions.
        if (oldState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS || newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            m_ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(resource.m_HazardTrackedResource));

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
        m_ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(resource.m_HazardTrackedResource));

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
        BarrierDesc.Transition.pResource = resource.m_HazardTrackedResource;
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
        GfxBufferCommon::HeapDesc heapDesc;
        heapDesc.m_HeapType = D3D12_HEAP_TYPE_UPLOAD;
        heapDesc.m_ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(stagedCBV.m_CBBytes.size());
        heapDesc.m_InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        heapDesc.m_ResourceName = stagedCBV.m_Name;

        D3D12MA::Allocation* uploadHeap = GfxBufferCommon::CreateHeap(heapDesc);
        assert(uploadHeap);

        // init desc heap for cbuffer
        GfxDescriptorHeap descHeap;
        descHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);
        SetD3DDebugName(descHeap.Dev(), stagedCBV.m_Name);

        // Describe and create a constant buffer view.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
        cbvDesc.BufferLocation = uploadHeap->GetResource()->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = stagedCBV.m_CBBytes.size();

        gfxDevice.Dev()->CreateConstantBufferView(&cbvDesc, descHeap.Dev()->GetCPUDescriptorHandleForHeapStart());

        void* mappedMemory = nullptr;
        DX12_CALL(uploadHeap->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedMemory)));
        memcpy(mappedMemory, stagedCBV.m_CBBytes.data(), stagedCBV.m_CBBytes.size());
        uploadHeap->GetResource()->Unmap(0, nullptr);

        const uint32_t RootOffset = 0; // TODO?
        StageDescriptor(descHeap.Dev()->GetCPUDescriptorHandleForHeapStart(), cbRegister, RootOffset);

        // free Descriptor & Upload heaps 2 frames later
        g_GfxManager.AddDoubleDeferredGraphicCommand([descHeap, uploadHeap]() { GfxBufferCommon::ReleaseAllocation(uploadHeap); });
    }

    // Resources
    if (m_StaleResourcesBitMap.none())
        return;
    
    uint32_t numHeapsNeeded = 0;
    RunOnAllBits(m_StaleResourcesBitMap.to_ulong(), [&](uint32_t rootIndex) { numHeapsNeeded += m_StagedResources[rootIndex].m_Descriptors.size(); });
    assert(numHeapsNeeded > 0);

    // allocate shader visible heaps
    GfxDescriptorHeapHandle destHandle = g_GfxGPUDescriptorAllocator.AllocateVolatile(D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, numHeapsNeeded);

    SetDescriptorHeapIfNeeded();

    RunOnAllBits(m_StaleResourcesBitMap.to_ulong(), [&](uint32_t rootIndex)
    {
        // set desc heap for this table
        m_CommandList->Dev()->SetGraphicsRootDescriptorTable(rootIndex, destHandle.m_GPUHandle);

        const GfxRootSignature::StagedResourcesDescriptors& resourceDesc = m_StagedResources[rootIndex];

        // copy descriptors to shader visible heaps 1 by 1
        // TODO: is this ineffcient? Better to use the full "CopyDescriptors" method?
        for (uint32_t i = 0; i < resourceDesc.m_Descriptors.size(); ++i)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE srcDesc = resourceDesc.m_Descriptors[i];

            if (srcDesc.ptr == 0)
            {
                CreateNullView(resourceDesc.m_Types[i], destHandle.m_CPUHandle);
            }
            else
            {
                gfxDevice.Dev()->CopyDescriptorsSimple(1, destHandle.m_CPUHandle, srcDesc, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }

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
