#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;

class GfxZPrePassRenderer : public GfxRendererBase
{
public:
    DeclareSingletonFunctions(GfxZPrePassRenderer);

    void Initialize();
    void ShutDown() override;
    void PopulateCommandList(GfxContext&) override;

private:
    GfxConstantBuffer m_RenderPassCB;
};
#define g_ZPrePassRenderer GfxZPrePassRenderer::GetInstance()
