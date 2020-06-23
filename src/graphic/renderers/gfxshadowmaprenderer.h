#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;

class GfxShadowMapRenderer : public GfxRendererBase
{
public:
    DeclareSingletonFunctions(GfxShadowMapRenderer);

    void Initialize();
    void ShutDown() override;
    void PopulateCommandList() override;

    const char* GetName() const override { return "GfxShadowMapRenderer"; }

private:
    void UpdateIMGUI();

    GfxRootSignature m_RootSignature;
};
#define g_GfxShadowMapRenderer GfxShadowMapRenderer::GetInstance()
