#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;
class GfxRootSignature;

class GfxForwardLightingPass : public GfxRendererBase
{
public:
    void Initialize() override;
    void ShutDown() override;
    bool ShouldRender(GfxContext&) const override;
    void PopulateCommandList(GfxContext& context) override;

    const char* GetName() const override { return "GfxForwardLightingPass"; }

private:
    void UpdateIMGUI();

    GfxRootSignature* m_RootSignature = nullptr;

    bool m_UsePBRConsts = false;
    float m_ConstPBRRoughness = 0.75f;
    float m_ConstPBRMetallic = 0.1f;
};
