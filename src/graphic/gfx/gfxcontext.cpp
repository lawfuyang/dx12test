#include <graphic/gfx/gfxcontext.h>
#include <graphic/pch.h>

void GfxContext::Initialize(D3D12_COMMAND_LIST_TYPE cmdListType, const std::string& name)
{
    m_CommandList = g_GfxCommandListsManager.Allocate(cmdListType, name);

    // Just set default viewport/scissor vals that make sense
    const CD3DX12_VIEWPORT viewport{ 0.0f, 0.0f, (float)g_CommandLineOptions.m_WindowWidth, (float)g_CommandLineOptions.m_WindowHeight };
    const CD3DX12_RECT scissorRect{ 0, 0, (LONG)g_CommandLineOptions.m_WindowWidth, (LONG)g_CommandLineOptions.m_WindowHeight };
    m_CommandList->Dev()->RSSetViewports(1, &viewport);
    m_CommandList->Dev()->RSSetScissorRects(1, &scissorRect);

    // set descriptor heap for this commandlist from the shader visible descriptor heap allocator
    ID3D12DescriptorHeap* ppHeaps[] = { g_GfxGPUDescriptorAllocator.GetInternalHeap().Dev() };
    m_CommandList->Dev()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    static const D3D12_VERTEX_BUFFER_VIEW NullVertexBufferView = {};
    static const D3D12_INDEX_BUFFER_VIEW NullIndexBufferView = {};
    m_CommandList->Dev()->IASetVertexBuffers(0, 1, &NullVertexBufferView);
    m_CommandList->Dev()->IASetIndexBuffer(&NullIndexBufferView);
}

void GfxContext::ClearRenderTargetView(GfxTexture& tex, const bbeVector4& clearColor)
{
    bbeProfileFunction();

    GfxContext& gfxContext = *this;
    bbeProfileGPUFunction(gfxContext);

    TransitionResource(tex, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;
    m_CommandList->Dev()->ClearRenderTargetView(tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), (const FLOAT*)&clearColor, numRects, pRects);
}

static void ClearDepthStencilInternal(GfxContext& gfxContext, GfxTexture& tex, float depth, uint8_t stencil, D3D12_CLEAR_FLAGS flags)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(gfxContext);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;

    gfxContext.GetCommandList().Dev()->ClearDepthStencilView(tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), flags, depth, stencil, numRects, pRects);
}

void GfxContext::ClearDepth(GfxTexture& tex, float depth)
{
    ClearDepthStencilInternal(*this, tex, depth, 0, D3D12_CLEAR_FLAG_DEPTH);
}

void GfxContext::ClearDepthStencil(GfxTexture& tex, float depth, uint8_t stencil)
{
    ClearDepthStencilInternal(*this, tex, depth, stencil, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);
}

void GfxContext::SetRenderTarget(uint32_t idx, GfxTexture& tex)
{
    assert(idx < _countof(m_RTVs));

    m_PSO.SetRenderTargetFormat(idx, tex.GetFormat());
    m_RTVs[idx] = &tex;
}

void GfxContext::SetDepthStencil(GfxTexture& tex)
{
    m_PSO.SetDepthStencilFormat(tex.GetFormat());
    m_DSV = &tex;
}

void GfxContext::SetVertexBuffer(GfxVertexBuffer& vBuffer)
{
    if (m_VertexBuffer != &vBuffer)
    {
        m_VertexBuffer = &vBuffer;
        m_DirtyBuffers = true;
    }
}

void GfxContext::SetIndexBuffer(GfxIndexBuffer& iBuffer)
{
    if (m_IndexBuffer != &iBuffer)
    {
        m_IndexBuffer = &iBuffer;
        m_DirtyBuffers = true;
    }
}

void GfxContext::SetRootSignature(GfxRootSignature& rootSig)
{
    // do check to prevent needless descriptor heap allocation
    if (m_PSO.m_RootSig == &rootSig)
        return;

    // Upon setting a new Root Signature, ALL staged resources will be set to null views!
    // Make sure you double check after setting a new root sig
    m_StagedResources = rootSig.m_Params;

    m_PSO.SetRootSignature(rootSig);
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

void GfxContext::StageDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, uint32_t rootIndex, uint32_t offset)
{
    D3D12_CPU_DESCRIPTOR_HANDLE& destDesc = m_StagedResources[rootIndex].m_Descriptors[offset];

    if (destDesc.ptr == srcDescriptor.ptr)
        return;

    destDesc = srcDescriptor;
    m_StaleResourcesBitMap.set(rootIndex);
}

void GfxContext::CompileAndSetGraphicsPipelineState()
{
    //bbeProfileFunction();

    assert(m_PSO.m_RootSig && m_PSO.m_RootSig->Dev());
    assert(m_CommandList);

    if (m_VertexBuffer)
    {
        assert(m_PSO.m_VertexFormat);
        assert(m_VertexBuffer->GetD3D12Resource());
        assert(m_VertexBuffer->GetStrideInBytes() > 0);
        assert(m_VertexBuffer->GetSizeInBytes() > 0);
    }

    for (uint32_t i = 0; i < m_PSO.m_RenderTargets.NumRenderTargets; ++i)
    {
        assert(m_RTVs[i] != nullptr);
        assert(m_PSO.m_RenderTargets.RTFormats[i] != DXGI_FORMAT_UNKNOWN);
    }

    m_PSO.m_Hash = std::hash<GfxPipelineStateObject>{}(m_PSO);
    if (m_LastUsedPSOHash != m_PSO.m_Hash)
    {
        bbeProfile("Set PSO Params");

        m_LastUsedPSOHash = m_PSO.m_Hash;

        // PSO
        m_CommandList->Dev()->SetPipelineState(g_GfxPSOManager.GetGraphicsPSO(m_PSO));

        // Input Assembler
        m_CommandList->Dev()->IASetPrimitiveTopology(m_PSO.m_PrimitiveTopology);

        // Output Merger
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[_countof(D3D12_RT_FORMAT_ARRAY::RTFormats)] = {};
        for (uint32_t i = 0; i < m_PSO.m_RenderTargets.NumRenderTargets; ++i)
        {
            rtvHandles[i] = m_RTVs[i]->GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart();

            TransitionResource(*m_RTVs[i], D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
        if (m_DSV)
        {
            dsvHandle = m_DSV->GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart();
        }

        m_CommandList->Dev()->OMSetRenderTargets(m_PSO.m_RenderTargets.NumRenderTargets, rtvHandles, FALSE, m_DSV ? &dsvHandle : nullptr);
    }


    if (m_DirtyBuffers && m_VertexBuffer)
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
    assert(m_CommandList);

    const D3D12_RESOURCE_STATES oldState = resource.m_CurrentResourceState;

    if (m_CommandList->GetType() == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        const uint32_t VALID_COMPUTE_QUEUE_RESOURCE_STATES = D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_COPY_SOURCE;

        assert((oldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == oldState);
        assert((newState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == newState);
    }

    if (oldState != newState)
    {
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarriers.emplace_back(D3D12_RESOURCE_BARRIER{});

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = resource.m_HazardTrackedResource;
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = oldState;
        BarrierDesc.Transition.StateAfter = newState;
        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        // Check to see if we already started the transition for this resource from another GfxContext
        if (newState == resource.m_TransitioningState)
        {
            assert(resource.m_TransitioningState != (D3D12_RESOURCE_STATES)-1);

            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
            resource.m_TransitioningState = (D3D12_RESOURCE_STATES)-1;
        }

        resource.m_CurrentResourceState = newState;
    }
    else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        InsertUAVBarrier(resource, flushImmediate);

    if (flushImmediate)
        FlushResourceBarriers();
}

void GfxContext::InsertUAVBarrier(GfxHazardTrackedResource& resource, bool flushImmediate)
{
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarriers.emplace_back(D3D12_RESOURCE_BARRIER{});

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.UAV.pResource = resource.m_HazardTrackedResource;

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

    // Upload CBV bytes
    for (const auto& elem : m_StagedCBVs)
    {
        const uint32_t cbRegister = elem.first;
        const StagedCBV& stagedCBV = elem.second;

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

        // Describe and create a constant buffer view.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
        cbvDesc.BufferLocation = uploadHeap->GetResource()->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = stagedCBV.m_CBBytes.size();

        GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
        gfxDevice.Dev()->CreateConstantBufferView(&cbvDesc, descHeap.Dev()->GetCPUDescriptorHandleForHeapStart());

        void* mappedMemory = nullptr;
        DX12_CALL(uploadHeap->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedMemory)));
        memcpy(mappedMemory, stagedCBV.m_CBBytes.data(), stagedCBV.m_CBBytes.size());
        uploadHeap->GetResource()->Unmap(0, nullptr);

        const uint32_t RootOffset = 0; // TODO?
        StageDescriptor(descHeap.Dev()->GetCPUDescriptorHandleForHeapStart(), cbRegister, RootOffset);

        // free Descriptor & Upload heaps next gpu frame
        g_GfxManager.AddGraphicCommand([descHeap, uploadHeap]()
        {
            // release CB heap
            GfxBufferCommon::ReleaseAllocation(uploadHeap);

            // descHeap then goes out of scope and decrements the last ref count of ComPtr<ID3D12DescriptorHeap>, destroying it
        });
    }

    // Resources
    if (m_StaleResourcesBitMap.none())
        return;
    
    uint32_t numHeapsNeeded = 0;
    RunOnAllBits(m_StaleResourcesBitMap.to_ulong(), [&](uint32_t rootIndex) { numHeapsNeeded += m_StagedResources[rootIndex].m_Descriptors.size(); });
    assert(numHeapsNeeded > 0);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    // allocate shader visible heaps
    GfxDescriptorHeapHandle destHandle = g_GfxGPUDescriptorAllocator.Allocate(numHeapsNeeded);

    static const uint32_t descriptorSize = gfxDevice.Dev()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    RunOnAllBits(m_StaleResourcesBitMap.to_ulong(), [&](uint32_t rootIndex)
    {
        // set desc heap for this table
        m_CommandList->Dev()->SetGraphicsRootDescriptorTable(rootIndex, destHandle.m_GPUHandle);

        const GfxRootSignature::StagedResourcesDescriptors& resourceDesc = m_StagedResources[rootIndex];

        // copy descriptors to shader visible heaps 1 by 1
        for (uint32_t i = 0; i < resourceDesc.m_Descriptors.size(); ++i)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE srcDesc = resourceDesc.m_Descriptors[i];

            if (srcDesc.ptr == 0)
            {
                D3D12_DESCRIPTOR_RANGE_TYPE srcDescType = resourceDesc.m_Types[i];
                CreateNullView(srcDescType, destHandle.m_CPUHandle);
            }
            else
            {
                gfxDevice.Dev()->CopyDescriptorsSimple(1, destHandle.m_CPUHandle, srcDesc, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }

            destHandle.Offset(1, descriptorSize);
        }
    });
}

void GfxContext::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation)
{
    //bbeProfileFunction();

    assert(m_IndexBuffer);

    PreDraw();
    m_CommandList->Dev()->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    PostDraw();
}

void GfxContext::PreDraw()
{
    FlushResourceBarriers();
    CompileAndSetGraphicsPipelineState();
    CommitStagedResources();
}

void GfxContext::PostDraw()
{
    // reset dirty flag for Vertex/Index buffers
    m_DirtyBuffers = false;

    m_StaleResourcesBitMap.reset();
    m_StagedCBVs.clear();
}
