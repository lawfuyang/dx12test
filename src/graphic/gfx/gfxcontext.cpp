#include "graphic/gfx/gfxcontext.h"

#include "graphic/dx12utils.h"
#include "graphic/gfx/gfxmanager.h"
#include "graphic/gfx/gfxcommandlist.h"
#include "graphic/gfx/gfxtexturesandbuffers.h"
#include "graphic/gfx/gfxrootsignature.h"

void GfxContext::ClearRenderTargetView(GfxTexture& tex, const bbeVector4& clearColor)
{
    bbeProfileFunction();

    GfxContext& gfxContext = *this;
    bbeProfileGPUFunction(gfxContext);

    TransitionResource(tex, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;

    const FLOAT colorInFloat[] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
    m_CommandList->Dev()->ClearRenderTargetView(tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), colorInFloat, numRects, pRects);
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

void GfxContext::BindConstantBuffer(GfxConstantBuffer& cBuffer)
{
    m_CBVToBind = &cBuffer.GetDescriptorHeap();
}

void GfxContext::BindSRV(uint32_t textureRegister, GfxTexture& tex)
{
    m_SRVsToBind.resize(textureRegister + 1, nullptr);
    m_SRVsToBind[textureRegister] = &tex;
}

void GfxContext::SetRootSigDescTable(uint32_t rootParamIdx, const GfxDescriptorHeap& heap)
{
    ID3D12DescriptorHeap* ppHeaps[] = { heap.Dev() };
    m_CommandList->Dev()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    m_CommandList->Dev()->SetGraphicsRootDescriptorTable(rootParamIdx, heap.Dev()->GetGPUDescriptorHandleForHeapStart());
}

void GfxContext::CompileAndSetGraphicsPipelineState()
{
    bbeProfileFunction();

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

    // Rasterizer
    if (m_DirtyRasterizer)
    {
        bbeProfile("Set Rasterizer Params");

        m_CommandList->Dev()->RSSetViewports(1, &m_Viewport);
        m_CommandList->Dev()->RSSetScissorRects(1, &m_ScissorRect);
    }

    if (m_DirtyPSO)
    {
        bbeProfile("Set PSO Params");

        // first, set Root Sig. use Default if nothing is specified
        if (!m_PSO.m_RootSig)
        {
            m_PSO.m_RootSig = &DefaultRootSignatures::DefaultGraphicsRootSignature;
        }
        m_CommandList->Dev()->SetGraphicsRootSignature(m_PSO.m_RootSig->Dev());

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

        D3D12_VERTEX_BUFFER_VIEW vBufferView;
        vBufferView.BufferLocation = m_VertexBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
        vBufferView.StrideInBytes = m_VertexBuffer->GetStrideInBytes();
        vBufferView.SizeInBytes = m_VertexBuffer->GetSizeInBytes();

        m_CommandList->Dev()->IASetVertexBuffers(0, 1, &vBufferView);

        if (m_IndexBuffer)
        {
            D3D12_INDEX_BUFFER_VIEW indexBufferView;
            indexBufferView.BufferLocation = m_IndexBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
            indexBufferView.Format = m_IndexBuffer->GetFormat();
            indexBufferView.SizeInBytes = m_IndexBuffer->GetSizeInBytes();

            m_CommandList->Dev()->IASetIndexBuffer(&indexBufferView);
        }
    }

    if (m_DirtyDescTables)
    {
        bbeProfile("Set Descriptor Table Params");

        uint32_t numDescTablesBound = 0;

        // Constant Buffers
        SetRootSigDescTable(numDescTablesBound++, g_GfxManager.GetFrameParams().GetDescriptorHeap());
        if (m_CBVToBind)
        {
            SetRootSigDescTable(numDescTablesBound++, *m_CBVToBind);
        }

        // SRVs
        for (GfxTexture* tex : m_SRVsToBind)
        {
            SetRootSigDescTable(numDescTablesBound++, tex->GetDescriptorHeap());
        }
    }
}

void GfxContext::CompileAndSetComputePipelineState()
{
    assert(m_CommandList);
}

void GfxContext::PostDraw()
{
    // Assume user will manually dirty the flags after each draw
    m_DirtyPSO        = false;
    m_DirtyRasterizer = false;
    m_DirtyBuffers    = false;
    m_DirtyDescTables = false;
}

void GfxContext::FlushResourceBarriers()
{
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

    const D3D12_RESOURCE_STATES oldState = resource.GetCurrentState();

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
        BarrierDesc.Transition.pResource = resource.GetD3D12Resource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = oldState;
        BarrierDesc.Transition.StateAfter = newState;
        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        // Check to see if we already started the transition
        //if (NewState == Resource.m_TransitioningState)
        //{
        //    BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
        //    Resource.m_TransitioningState = (D3D12_RESOURCE_STATES)-1;
        //}

        resource.SetHazardTrackedState(newState);
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
    BarrierDesc.UAV.pResource = resource.GetD3D12Resource();

    if (flushImmediate)
        FlushResourceBarriers();
}

void GfxContext::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
    bbeProfileFunction();

    FlushResourceBarriers();
    CompileAndSetGraphicsPipelineState();

    m_CommandList->Dev()->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);

    PostDraw();
}

void GfxContext::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation)
{
    bbeProfileFunction();

    assert(m_IndexBuffer);

    FlushResourceBarriers();
    CompileAndSetGraphicsPipelineState();

    m_CommandList->Dev()->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);

    PostDraw();
}
