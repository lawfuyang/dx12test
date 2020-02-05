#include "graphic/gfxcontext.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxcommandlist.h"
#include "graphic/gfxtexturesandbuffers.h"
#include "graphic/gfxrootsignature.h"

void GfxContext::ClearRenderTargetView(GfxRenderTargetView& rtv, XMFLOAT4 clearColor) const
{
    bbeProfileFunction();

    // TODO: merge all required resource barriers and run them all at once just before GfxDevice::ExecuteAllActiveCommandLists or something
    rtv.Transition(*m_CommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;

    const FLOAT colorInFloat[] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
    m_CommandList->Dev()->ClearRenderTargetView(rtv.GetDescriptorHeap().GetCPUDescHandle(), colorInFloat, numRects, pRects);
}

void GfxContext::SetRenderTarget(uint32_t idx, GfxRenderTargetView& rtv)
{
    assert(idx < _countof(m_RTVs));

    m_PSO.SetRenderTargetFormat(idx, rtv.GetFormat());
    m_RTVs[idx] = &rtv;
}

void GfxContext::CompileAndSetGraphicsPipelineState()
{
    assert(m_PSOManager);
    assert(m_CommandList);
    assert(m_VertexBuffer);
    assert(m_VertexBuffer->GetD3D12Resource());
    assert(m_VertexBuffer->GetStrideInBytes() > 0);
    assert(m_VertexBuffer->GetSizeInBytes() > 0);

    for (uint32_t i = 0; i < m_PSO.m_RenderTargets.NumRenderTargets; ++i)
    {
        assert(m_RTVs[i] != nullptr);
        assert(m_PSO.m_RenderTargets.RTFormats[i] != DXGI_FORMAT_UNKNOWN);
    }

    // PSO
    m_CommandList->Dev()->SetPipelineState(m_PSOManager->GetGraphicsPSO(m_PSO));

    // Rasterizer
    m_CommandList->Dev()->RSSetViewports(1, &m_Viewport);
    m_CommandList->Dev()->RSSetScissorRects(1, &m_ScissorRect);

    // Input Assembler
    m_CommandList->Dev()->IASetPrimitiveTopology(m_PSO.m_PrimitiveTopology);

    D3D12_VERTEX_BUFFER_VIEW vBufferView;
    vBufferView.BufferLocation = m_VertexBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
    vBufferView.StrideInBytes  = m_VertexBuffer->GetStrideInBytes();
    vBufferView.SizeInBytes    = m_VertexBuffer->GetSizeInBytes();
    m_CommandList->Dev()->IASetVertexBuffers(0, 1, &vBufferView);

    // Output Merger
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[_countof(D3D12_RT_FORMAT_ARRAY::RTFormats)] = {};
    for (uint32_t i = 0; i < m_PSO.m_RenderTargets.NumRenderTargets; ++i)
    {
        rtvHandles[i] = m_RTVs[i]->GetDescriptorHeap().GetCPUDescHandle();
    }
    m_CommandList->Dev()->OMSetRenderTargets(m_PSO.m_RenderTargets.NumRenderTargets, rtvHandles, FALSE, nullptr); // TODO: Add support for DepthStencil RTV

    m_CommandList->Dev()->SetGraphicsRootSignature(m_PSO.m_RootSig->Dev());
}

void GfxContext::CompileAndSetComputePipelineState()
{
    bbeProfileFunction();

    assert(m_PSOManager);
    assert(m_CommandList);
}

void GfxContext::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
    bbeProfileFunction();

    CompileAndSetGraphicsPipelineState();

    m_CommandList->Dev()->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}
