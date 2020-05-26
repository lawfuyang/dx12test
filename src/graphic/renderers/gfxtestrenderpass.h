#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfxtexturesandbuffers.h>

class GfxContext;

class GfxTestRenderPass : public GfxRendererBase
{
public:
    DeclareSingletonFunctions(GfxTestRenderPass);

    void Initialize();
    void ShutDown() override;
    void PopulateCommandList(GfxContext&) override;

private:
    GfxConstantBuffer m_RenderPassCB;
};
#define g_GfxTestRenderPass GfxTestRenderPass::GetInstance()
