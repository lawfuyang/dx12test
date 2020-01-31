#include "graphic/gfxcontext.h"

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

void GfxContext::CompileAndSetPipelineState()
{
    assert(m_PSOManager);
    assert(m_CommandList);
    assert(m_PSO.m_RootSig);
    assert(m_PSO.m_VS || m_PSO.m_CS); // Must have either VS or CS
    assert((m_PSO.m_VS && m_PSO.m_CS) == false); // Cannot have both VS & CS stages active simultaneously
  
    if (m_PSO.m_VS)
    {
        assert(m_PSO.m_VertexFormat);
        assert(m_PSO.m_RenderTargets.NumRenderTargets > 0);

        for (uint32_t i = 0; i < m_PSO.m_RenderTargets.NumRenderTargets; ++i)
        {
            assert(m_PSO.m_RenderTargets.RTFormats[i] != DXGI_FORMAT_UNKNOWN);
        }
    }

    ID3D12PipelineState* compiledPSO = m_PSOManager->GetPSO(m_PSO);
    assert(compiledPSO);

    m_CommandList->Dev()->SetPipelineState(compiledPSO);

    m_CommandList->Dev()->RSSetViewports(1, &m_Viewport);
    m_CommandList->Dev()->RSSetScissorRects(1, &m_ScissorRect);

    //for (GfxRenderTargetView* rtv : m_RTVs)
    //{

    //}
    //m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
}
