#pragma once

#include <graphic/view.h>

struct DirectionalLight
{
    static const uint32_t ShadowMapResolution = 1024;

    void Update();
    void UpdateIMGUI();
    
    float m_Intensity = 1.0f;
    bbeVector4 m_Direction = { 0.577f, 0.577f, 0.577f, 1.0f };

    View m_View;
};

class GfxLightsManager
{
public:
    DeclareSingletonFunctions(GfxLightsManager);

    const DirectionalLight& GetDirectionalLight() const { return m_DirectionalLight; }

    void Initialize();

private:
    void UpdateIMGUI();

    DirectionalLight m_DirectionalLight;
};
#define g_GfxLightsManager GfxLightsManager::GetInstance()
