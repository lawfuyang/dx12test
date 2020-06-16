#pragma once

class GfxLightsManager
{
public:
    DeclareSingletonFunctions(GfxLightsManager);

    float GetSunLightIntensity() const { return m_SunLightIntensity; }
    const bbeVector4& GetSunLightDir() const { return m_SunLightDir; }

    void Initialize();

private:
    void UpdateIMGUI();
    void UpdateDirLight();

    float m_SunMaxElevation = 90.0f;
    float m_SunMinElevation = 0.0f;
    float m_WorldLatitude = -30.0f;
    float m_TimeOfDay = 12.0f;
    float m_SunLightIntensity = 1.0f;
    bbeVector4 m_SunLightDir;
};
#define g_GfxLightsManager GfxLightsManager::GetInstance()
