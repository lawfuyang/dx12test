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

void GfxContext::CompileAndSetPipelineStateCommon()
{
    assert(m_PSOManager);
    assert(m_CommandList);
    assert(m_PSO.m_RootSig);
}

void GfxContext::CompileAndSetPipelineStateForDraw()
{
    CompileAndSetPipelineStateCommon();

    assert(m_PSO.m_VS);
    assert(m_PSO.m_VertexFormat);
    assert(m_VertexBuffer);
    assert(m_VertexBuffer->GetBufferView().BufferLocation != 0);
    assert(m_PSO.m_RenderTargets.NumRenderTargets > 0);

    for (uint32_t i = 0; i < m_PSO.m_RenderTargets.NumRenderTargets; ++i)
    {
        assert(m_RTVs[i] != nullptr);
        assert(m_PSO.m_RenderTargets.RTFormats[i] != DXGI_FORMAT_UNKNOWN);
    }

    // PSO
    ID3D12PipelineState* compiledPSO = m_PSOManager->GetPSOForDraw(m_PSO);
    assert(compiledPSO);
    m_CommandList->Dev()->SetPipelineState(compiledPSO);

    // Rasterizer
    m_CommandList->Dev()->RSSetViewports(1, &m_Viewport);
    m_CommandList->Dev()->RSSetScissorRects(1, &m_ScissorRect);

    // Input Assembler
    m_CommandList->Dev()->IASetPrimitiveTopology(m_PSO.m_PrimitiveTopology);
    m_CommandList->Dev()->IASetVertexBuffers(0, 1, &m_VertexBuffer->GetBufferView());

    // Output Merger
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[_countof(D3D12_RT_FORMAT_ARRAY::RTFormats)] = {};
    for (uint32_t i = 0; i < m_PSO.m_RenderTargets.NumRenderTargets; ++i)
    {
        rtvHandles[i] = m_RTVs[i]->GetDescriptorHeap().GetCPUDescHandle();
    }
    m_CommandList->Dev()->OMSetRenderTargets(m_PSO.m_RenderTargets.NumRenderTargets, rtvHandles, FALSE, nullptr); // TODO: Add support for DepthStencil RTV
}

void GfxContext::CompileAndSetPipelineStateForDispatch()
{
    bbeProfileFunction();

    CompileAndSetPipelineStateCommon();

    assert(m_PSO.m_CS);
}

void GfxContext::DrawInstanced(uint32_t VertexCountPerInstance, uint32_t InstanceCount, uint32_t StartVertexLocation, uint32_t StartInstanceLocation)
{
    bbeProfileFunction();

    CompileAndSetPipelineStateForDraw();

    m_CommandList->Dev()->DrawInstanced(3, 1, 0, 0);
}
