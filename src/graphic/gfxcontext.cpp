#include "graphic/gfxcontext.h"

#include "graphic/gfxcommandlist.h"
#include "graphic/gfxtexturesandbuffers.h"
#include "graphic/gfxrootsignature.h"

void GfxContext::ClearRenderTargetView(GfxRenderTargetView& rtv, XMFLOAT4 clearColor) const
{
    // TODO: merge all required resource barriers and run them all at once just before GfxDevice::ExecuteAllActiveCommandLists or something
    rtv.GetHazardTrackedResource().Transition(*m_CommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;

    const FLOAT colorInFloat[] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
    m_CommandList->Dev()->ClearRenderTargetView(rtv.GetCPUDescHandle(), colorInFloat, numRects, pRects);
}
