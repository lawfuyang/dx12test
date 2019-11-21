#include "graphic/gfxcontext.h"

#include "graphic/gfxcommandlist.h"
#include "graphic/gfxtexturesandbuffers.h"

void GfxContext::ClearRenderTargetView(GfxRenderTargetView& rtv, const float(&clearColor)[4])
{
    const UINT numBarriers = 1;

    rtv.GetHazardTrackedResource().Transition(*m_CommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;

    m_CommandList->Dev()->ClearRenderTargetView(rtv.GetCPUDescHandle(), clearColor, numRects, pRects);
}
