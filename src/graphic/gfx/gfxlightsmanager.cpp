#include <graphic/gfx/gfxlightsmanager.h>

#include <system/imguimanager.h>

static bool gs_ShowGfxLightsManagerIMGUIWindow = false;

static float ComputeSunTime(float timeOfDay)
{
    const float t = timeOfDay;
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float t4 = t3 * t;
    const float t5 = t4 * t;

    // We want the sun to rise a 4h30 and to set at 19h30, but the current equation make the sun rise at 6 and set a 16h.
    const float adjustedTime = (2.03044f * t) - (0.231102f * t2) + (0.0206275f * t3) - (0.000888001f * t4) + (0.0000148f * t5);
    const float normalizedTime = (adjustedTime / 24.0f);

    return normalizedTime;
}

void GfxLightsManager::Initialize()
{
    g_IMGUIManager.RegisterTopMenu("Graphic", "GfxLightsManager", &gs_ShowGfxLightsManagerIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUI(); });

    m_DirectionalLight.Update();
}

void GfxLightsManager::UpdateIMGUI()
{
    if (!gs_ShowGfxLightsManagerIMGUIWindow)
        return;

    ScopedIMGUIWindow window{ "GfxLightsManager" };

    if (ImGui::CollapsingHeader("Sun Light"))
    {
        m_DirectionalLight.UpdateIMGUI();
    }
}

void DirectionalLight::Update()
{
    m_Intensity = bbeSmoothStep(0.0f, 4.5f, m_TimeOfDay) * bbeSmoothStep(24.0f, 19.5f, m_TimeOfDay);
    m_Intensity = std::max(m_Intensity, 0.25f); // some minimum sunlight light

    const float normalizedTime = ComputeSunTime(m_TimeOfDay);
    const bbeMatrix timeOfDayMatrixSun = bbeMatrix::CreateRotationY(normalizedTime * -bbe2PI);
    const bbeMatrix latitudeMatrix = bbeMatrix::CreateRotationX(ConvertToRadians(m_WorldLatitude));
    const bbeMatrix sunMatrix = latitudeMatrix * timeOfDayMatrixSun;

    m_Direction = bbeVector4::Transform(bbeVector4::UnitY, sunMatrix);
    m_Direction.z = std::clamp(m_Direction.z, ConvertToRadians(m_MinElevation), ConvertToRadians(m_MaxElevation));
    m_Direction.Normalize();
}

void DirectionalLight::UpdateIMGUI()
{
    bool updateDirLight = false;
    updateDirLight |= ImGui::SliderFloat("Time (24h)", &m_TimeOfDay, 0.0f, 24.0f);
    updateDirLight |= ImGui::SliderFloat("World Latitude", &m_WorldLatitude, -90.0f, 90.0f);
    updateDirLight |= ImGui::SliderFloat("Min Sun Elevation", &m_MinElevation, -90.0f, 90.0f);
    updateDirLight |= ImGui::SliderFloat("Max SunElevation", &m_MaxElevation, -90.0f, 90.0f);

    if (updateDirLight)
    {
        Update();
    }
}
