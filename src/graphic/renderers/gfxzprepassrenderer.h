#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;

class GfxZPrePassRenderer : public GfxRendererBase
{
public:
    DeclareSingletonFunctions(GfxZPrePassRenderer);

    void Initialize();
    void ShutDown() override;
    void PopulateCommandList() override;

    const char* GetName() const override { return "GfxZPrePassRenderer"; }

private:
    GfxRootSignature m_RootSignature;
};
#define g_ZPrePassRenderer GfxZPrePassRenderer::GetInstance()
