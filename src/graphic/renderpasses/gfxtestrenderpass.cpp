#include "graphic/renderpasses/gfxtestrenderpass.h"

#include "graphic/gfxcontext.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxmanager.h"

GfxTestRenderPass::GfxTestRenderPass()
    : GfxRenderPass("GfxTestRenderPass")
{
}

void GfxTestRenderPass::Render(GfxContext& context)
{
    bbeProfileFunction();

    GfxSwapChain& swapChain = GfxManager::GetInstance().GetSwapChain();

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    context.ClearRenderTargetView(swapChain.GetCurrentBackBuffer(), clearColor);

    swapChain.TransitionBackBufferForPresent(context);
}
