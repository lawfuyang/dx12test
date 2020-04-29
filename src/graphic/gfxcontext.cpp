#include "graphic/gfxcontext.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxmanager.h"
#include "graphic/gfxcommandlist.h"
#include "graphic/gfxtexturesandbuffers.h"
#include "graphic/gfxrootsignature.h"

void GfxContext::ClearRenderTargetView(GfxTexture& tex, const bbeVector4& clearColor)
{
    bbeProfileFunction();

    GfxContext& gfxContext = *this;
    bbeProfileGPUFunction(gfxContext);

    // TODO: merge all required resource barriers and run them all at once just before GfxDevice::ExecuteAllActiveCommandLists or something
    tex.Transition(*m_CommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;

    const FLOAT colorInFloat[] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
    m_CommandList->Dev()->ClearRenderTargetView(tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), colorInFloat, numRects, pRects);
}

void GfxContext::ClearDepthStencilView(GfxTexture& tex, float depth, uint8_t stencil)
{
    bbeProfileFunction();

    GfxContext& gfxContext = *this;
    bbeProfileGPUFunction(gfxContext);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;

    m_CommandList->Dev()->ClearDepthStencilView(tex.GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, depth, stencil, numRects, pRects);
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

void GfxContext::BindConstantBuffer(GfxConstantBuffer& cBuffer)
{
    m_CBVToBind = &cBuffer.GetDescriptorHeap();
}

void GfxContext::BindSRV(GfxTexture& tex)
{
    m_SRVsToBind.push_back(&tex);
}

void GfxContext::SetRootSigDescTable(uint32_t rootParamIdx, const GfxDescriptorHeap& heap)
{
    ID3D12DescriptorHeap* ppHeaps[] = { heap.Dev() };
    m_CommandList->Dev()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    m_CommandList->Dev()->SetGraphicsRootDescriptorTable(rootParamIdx, heap.Dev()->GetGPUDescriptorHandleForHeapStart());
}

void GfxContext::CompileAndSetGraphicsPipelineState()
{
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

    // first, set Root Sig. use Default if nothing is specified
    if (!m_PSO.m_RootSig)
    {
        m_PSO.m_RootSig = &DefaultRootSignatures::DefaultGraphicsRootSignature;
    }
    m_CommandList->Dev()->SetGraphicsRootSignature(m_PSO.m_RootSig->Dev());

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

    // PSO
    m_CommandList->Dev()->SetPipelineState(g_GfxPSOManager.GetGraphicsPSO(m_PSO));

    // Rasterizer
    m_CommandList->Dev()->RSSetViewports(1, &m_Viewport);
    m_CommandList->Dev()->RSSetScissorRects(1, &m_ScissorRect);

    // Input Assembler
    m_CommandList->Dev()->IASetPrimitiveTopology(m_PSO.m_PrimitiveTopology);

    if (m_VertexBuffer)
    {
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

    // Output Merger
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[_countof(D3D12_RT_FORMAT_ARRAY::RTFormats)] = {};
    for (uint32_t i = 0; i < m_PSO.m_RenderTargets.NumRenderTargets; ++i)
    {
        rtvHandles[i] = m_RTVs[i]->GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart();

        m_RTVs[i]->Transition(*m_CommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
    if (m_DSV)
    {
        dsvHandle = m_DSV->GetDescriptorHeap().Dev()->GetCPUDescriptorHandleForHeapStart();
    }

    m_CommandList->Dev()->OMSetRenderTargets(m_PSO.m_RenderTargets.NumRenderTargets, rtvHandles, FALSE, m_DSV ? &dsvHandle : nullptr);
}

void GfxContext::CompileAndSetComputePipelineState()
{
    assert(m_CommandList);
}

void GfxContext::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
    bbeProfileFunction();

    CompileAndSetGraphicsPipelineState();

    m_CommandList->Dev()->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void GfxContext::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation)
{
    bbeProfileFunction();

    assert(m_IndexBuffer);

    CompileAndSetGraphicsPipelineState();

    m_CommandList->Dev()->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}
