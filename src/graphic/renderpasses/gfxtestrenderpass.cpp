#include "graphic/renderpasses/gfxtestrenderpass.h"

#include "graphic/gfxcontext.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxmanager.h"
#include "graphic/gfxdevice.h"

GfxTestRenderPass::GfxTestRenderPass()
    : GfxRenderPass("GfxTestRenderPass")
{
}

void GfxTestRenderPass::Render(GfxContext& context)
{
    bbeProfileFunction();

    GfxSwapChain& swapChain = GfxManager::GetInstance().GetSwapChain();

    context.ClearRenderTargetView(swapChain.GetCurrentBackBuffer(), XMFLOAT4{ 0.0f, 0.2f, 0.4f, 1.0f });

    swapChain.TransitionBackBufferForPresent(context);
}
