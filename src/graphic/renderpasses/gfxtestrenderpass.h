#pragma once

#include "graphic/renderpasses/gfxrenderpass.h"

#include "graphic/gfxtexturesandbuffers.h"

class GfxContext;

class GfxTestRenderPass : public GfxRenderPass
{
public:
    DeclareSingletonFunctions(GfxTestRenderPass);

    void Initialize();
    void ShutDown() override;
    void Render(GfxContext&) override;

private:
    GfxConstantBuffer m_RenderPassCB;
    GfxTexture m_DepthBuffer;
};
#define g_GfxTestRenderPass GfxTestRenderPass::GetInstance()
