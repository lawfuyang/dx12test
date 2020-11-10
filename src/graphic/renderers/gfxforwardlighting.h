#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;
class GfxRootSignature;

class GfxForwardLightingPass : public GfxRendererBase
{
public:
    DeclareSingletonFunctions(GfxForwardLightingPass);

    void Initialize();
    void ShutDown() override;
    void PopulateCommandList() override;

    const char* GetName() const override { return "GfxForwardLightingPass"; }

private:
    void UpdateIMGUI();

    GfxRootSignature* m_RootSignature = nullptr;

    bool m_UsePBRConsts = false;
    float m_ConstPBRRoughness = 0.75f;
    float m_ConstPBRMetallic = 0.1f;
};
#define g_GfxForwardLightingPass GfxForwardLightingPass::GetInstance()
