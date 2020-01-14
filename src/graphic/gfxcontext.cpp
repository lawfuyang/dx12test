#include "graphic/gfxcontext.h"

#include "graphic/gfxcommandlist.h"
#include "graphic/gfxtexturesandbuffers.h"
#include "graphic/gfxrootsignature.h"

void GfxContext::ClearRenderTargetView(GfxRenderTargetView& rtv, XMFLOAT4 clearColor) const
{
    bbeProfileFunction();

    // TODO: merge all required resource barriers and run them all at once just before GfxDevice::ExecuteAllActiveCommandLists or something
    rtv.GetHazardTrackedResource().Transition(*m_CommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;

    const FLOAT colorInFloat[] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
    m_CommandList->Dev()->ClearRenderTargetView(rtv.GetCPUDescHandle(), colorInFloat, numRects, pRects);
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
    }

    ID3D12PipelineState* compiledPSO = m_PSOManager->GetPSO(m_PSO);
    assert(compiledPSO);

    m_CommandList->Dev()->SetPipelineState(compiledPSO);
}
