#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;
class GfxRootSignature;

class GfxBodyGravityParticlesUpdate : public GfxRendererBase
{
public:
    void Initialize() override;
    void PopulateCommandList(GfxContext& context) override;

    const char* GetName() const override { return "GfxBodyGravityParticlesUpdate"; }

    GfxRootSignature* m_RootSignature = nullptr;
};

class GfxBodyGravityParticlesRender : public GfxRendererBase
{
public:
    void Initialize() override;
    void PopulateCommandList(GfxContext& context) override;

    const char* GetName() const override { return "GfxBodyGravityParticlesRender"; }

    GfxRootSignature* m_RootSignature = nullptr;
};
