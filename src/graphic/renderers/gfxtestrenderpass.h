#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;

class GfxTestRenderPass : public GfxRendererBase
{
public:
    DeclareSingletonFunctions(GfxTestRenderPass);

    void Initialize();
    void ShutDown() override;
    void PopulateCommandList() override;

    const char* GetName() const override { return "GfxTestRenderPass"; }

private:
    GfxRootSignature m_RootSignature;
    GfxConstantBuffer m_RenderPassCB;
};
#define g_GfxTestRenderPass GfxTestRenderPass::GetInstance()
