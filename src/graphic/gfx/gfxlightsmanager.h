#pragma once

#include <graphic/view.h>

struct DirectionalLight
{
    static const uint32_t ShadowMapResolution = 1024;

    void Update();
    void UpdateIMGUI();
    
    bbeVector3 m_Intensity = { 2.0f, 2.0f, 2.0f };
    bbeVector3 m_Direction = { 0.577f, 0.577f, 0.577f };

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
