#include <graphic/gfx/gfxlightsmanager.h>

#include <graphic/gfx/gfxmanager.h>

#include <system/imguimanager.h>

static bool gs_ShowGfxLightsManagerIMGUIWindow = false;

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

    if (ImGui::CollapsingHeader("Sun Light", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
    {
        m_DirectionalLight.UpdateIMGUI();
    }
}

void DirectionalLight::Update()
{
    m_View.m_ZNearP = 1.0f;
    m_View.m_ZFarP = 2000.0f;

    m_Direction = Normalize3(m_Direction);

    const bbeMatrix lightRotation = bbeMatrix::CreateFromQuaternion(bbeQuaternion{ m_Direction });
    const bbeVector3 to = bbeVector3::TransformNormal(-bbeVector3::UnitY, lightRotation);
    const bbeVector3 up = bbeVector3::TransformNormal(bbeVector3::UnitZ, lightRotation);
    const bbeMatrix lightView = CreateLookAtLH(bbeVector3::Zero, to, up); // important to not move (zero out eye vector) the light view matrix itself because texel snapping must be done on projection matrix!

    // Unproject main frustum corners into world space
    View& mainView = g_GfxManager.GetMainView();
    const bbeVector3 frustumCorners[] =
    {
        bbeVector3::Transform(bbeVector3{ -1, -1, 0 }, mainView.m_InvProjection), // top left near
        bbeVector3::Transform(bbeVector3{ -1, -1, 1 }, mainView.m_InvProjection), // top left far
        bbeVector3::Transform(bbeVector3{ -1,  1, 0 }, mainView.m_InvProjection), // top right near
        bbeVector3::Transform(bbeVector3{ -1,  1, 1 }, mainView.m_InvProjection), // top right far
        bbeVector3::Transform(bbeVector3{  1, -1, 0 }, mainView.m_InvProjection), // bottom left near
        bbeVector3::Transform(bbeVector3{  1, -1, 1 }, mainView.m_InvProjection), // bottom left far
        bbeVector3::Transform(bbeVector3{  1,  1, 0 }, mainView.m_InvProjection), // bottom right near
        bbeVector3::Transform(bbeVector3{  1,  1, 1 }, mainView.m_InvProjection), // bottom right far
    };

    // TODO: Cascading Shadow Maps
    //const float splitNear = splits[cascade];
    //const float splitFar = splits[cascade + 1];

    // Compute cascade sub-frustum in light-view-space from the main frustum corners:
    const float splitNear = 0.0f;
    const float splitFar = 1.0f;
    const bbeVector3 corners[] =
    {
        bbeVector3::Transform(bbeVector3::Lerp(frustumCorners[0], frustumCorners[1], splitNear), lightView),
        bbeVector3::Transform(bbeVector3::Lerp(frustumCorners[0], frustumCorners[1], splitFar), lightView),
        bbeVector3::Transform(bbeVector3::Lerp(frustumCorners[2], frustumCorners[3], splitNear), lightView),
        bbeVector3::Transform(bbeVector3::Lerp(frustumCorners[2], frustumCorners[3], splitFar), lightView),
        bbeVector3::Transform(bbeVector3::Lerp(frustumCorners[4], frustumCorners[5], splitNear), lightView),
        bbeVector3::Transform(bbeVector3::Lerp(frustumCorners[4], frustumCorners[5], splitFar), lightView),
        bbeVector3::Transform(bbeVector3::Lerp(frustumCorners[6], frustumCorners[7], splitNear), lightView),
        bbeVector3::Transform(bbeVector3::Lerp(frustumCorners[6], frustumCorners[7], splitFar), lightView),
    };

    // Compute cascade bounding sphere center & radius:
    bbeVector3 center;
    float radius = 0.0f;
    for (const bbeVector3& corner : corners)
    {
        center += corner;
        radius = std::max(radius, (corner - center).Length());
    }
    center /= _countof(corners);

    // Fit AABB onto bounding sphere:
    bbeVector3 vMin = center - bbeVector3{ radius };
    bbeVector3 vMax = center + bbeVector3{ radius };
    const bbeVector3 extent = vMax - vMin;

    // Snap cascade to texel grid:
    const bbeVector3 texelSize = extent / ShadowMapResolution;
    vMin = Floor(vMin / texelSize) * texelSize;
    vMax = Floor(vMax / texelSize) * texelSize;
    center = (vMin + vMax) * 0.5f;

    // TODO: wtf hack is this?
    // Extrude bounds to avoid early shadow clipping:
    float ext = std::abs(center.z - vMin.z);
    ext = std::max(ext, m_View.m_ZFarP * 0.5f);
    vMin.z = center.z - ext;
    vMax.z = center.z + ext;

    const bbeMatrix lightProjection = bbeMatrix::CreateOrthographicOffCenter(vMin.x, vMax.x, vMin.y, vMax.y, vMin.z, vMax.z);

    m_View.m_VP = lightProjection * lightView;
    m_View.m_Frustum.Create(m_View.m_VP);
}

void DirectionalLight::UpdateIMGUI()
{
    bool updateDirLight = false;
    updateDirLight |= ImGui::SliderFloat3("Direction", &m_Direction.x, 0.0f, 1.0f);
    updateDirLight |= ImGui::SliderFloat("Intensity", &m_Intensity, 0.0f, 1.0f);

    if (updateDirLight)
    {
        Update();
    }
}
