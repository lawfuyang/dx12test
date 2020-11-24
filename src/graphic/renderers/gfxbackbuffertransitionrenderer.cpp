
#include <graphic/pch.h>

class GfxBackBufferTransitionRenderer : public GfxRendererBase
{
    // Inherited via GfxRendererBase
    const char* GetName() const override { return "GfxBackBufferTransitionRenderer"; }
    void PopulateCommandList(GfxContext& context) override;
};

void GfxBackBufferTransitionRenderer::PopulateCommandList(GfxContext& context)
{
    GfxTexture& backBufferTex = g_GfxManager.GetSwapChain().GetCurrentBackBuffer();
    context.TransitionResource(backBufferTex, D3D12_RESOURCE_STATE_PRESENT, true);
}

static GfxBackBufferTransitionRenderer gs_GfxBackBufferTransitionRenderer;
GfxRendererBase* g_GfxBackBufferTransitionRenderer = &gs_GfxBackBufferTransitionRenderer;
