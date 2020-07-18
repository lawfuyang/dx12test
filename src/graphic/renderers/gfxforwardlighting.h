#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;

class GfxForwardLightingPass : public GfxRendererBase
{
public:
    DeclareSingletonFunctions(GfxForwardLightingPass);

    void Initialize();
    void ShutDown() override;
    void PopulateCommandList() override;

    const char* GetName() const override { return "GfxForwardLightingPass"; }

private:
    GfxRootSignature m_RootSignature;
    GfxConstantBuffer m_RenderPassCB;
};
#define g_GfxForwardLightingPass GfxForwardLightingPass::GetInstance()
