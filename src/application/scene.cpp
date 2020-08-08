#include <application/scene.h>

#include <system/imguimanager.h>

#include <graphic/visual.h>
#include <graphic/gfx/gfxdefaultassets.h>

static bool gs_SceneShowSelectedVisualPropertiesWindow = true;
static bool gs_SceneShowSceneVisualsWindow = true;

void Scene::Initialize()
{
    static bool s_OpenSceneCB = false;
    g_IMGUIManager.RegisterTopMenu("Scene", "Open Scene", &s_OpenSceneCB);
    g_IMGUIManager.RegisterGeneralButtonCB([&]() { OpenSceneWindow(); }, &s_OpenSceneCB);

    static bool s_SaveSceneCB = false;
    g_IMGUIManager.RegisterTopMenu("Scene", "Save Scene", &s_SaveSceneCB);
    g_IMGUIManager.RegisterGeneralButtonCB([&]() { SaveSceneWindow(); }, &s_SaveSceneCB);

    static bool s_SaveSceneAsCB = false;
    g_IMGUIManager.RegisterTopMenu("Scene", "Save Scene as...", &s_SaveSceneAsCB);
    g_IMGUIManager.RegisterGeneralButtonCB([&]() { SaveSceneAsWindow(); }, &s_SaveSceneAsCB);
    
    static bool s_AddNewVisualCB = false;
    g_IMGUIManager.RegisterTopMenu("Scene", "Add New Visual", &s_AddNewVisualCB);
    g_IMGUIManager.RegisterGeneralButtonCB([&]() { AddNewVisual(); }, &s_AddNewVisualCB);

    g_IMGUIManager.RegisterTopMenu("Scene", "Show Selected Visual Properties Window", &gs_SceneShowSelectedVisualPropertiesWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateSelectedVisualPropertiesWindow(); });

    g_IMGUIManager.RegisterTopMenu("Scene", "Show Scene Visuals Window", &gs_SceneShowSceneVisualsWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateSceneVisualsWindow(); });
}

void Scene::Update()
{

}

void Scene::ShutDown()
{

}

Visual* Scene::GetVisualFromID(ObjectID id)
{
    if (id == ID_InvalidObject) return nullptr;

    auto it = std::find_if(m_AllVisuals.begin(), m_AllVisuals.end(), [&](Visual* v) { return v->m_ObjectID == id; });
    return it == m_AllVisuals.end() ? nullptr : *it;
}

void Scene::OpenSceneWindow()
{
    g_IMGUIManager.RegisterFileDialog("Scene::OpenSceneWindow", ".scene", [](const std::string& filePath) {}); // TODO: Finalizer
}

void Scene::SaveSceneWindow()
{
    if (m_CurrentSceneFile.empty())
    {
        SaveSceneAsWindow();
        return;
    }
}

void Scene::SaveSceneAsWindow()
{
    g_IMGUIManager.RegisterFileDialog("Scene::OpenSceneWindow", ".scene", [](const std::string& filePath) {}); // TODO: Finalizer
}

void Scene::AddNewVisual()
{
    Visual* newVisual = m_VisualsPool.construct();
    newVisual->m_ObjectID = GenerateObjectID();

    // add to container in the beginning of the next engine frame
    g_System.AddSystemCommand([&, newVisual]()
        {
            bbeMultiThreadDetector();
            assert(m_AllVisuals.size() < C_VISUALS_ARRAY_SZ); // Ensure it doesnt grow and invalid all Visual ptrs in the engine
            m_AllVisuals.push_back(newVisual);
        });

    m_SelectedVisual = newVisual;
}

void Scene::UpdateSelectedVisualPropertiesWindow()
{
    if (!gs_SceneShowSelectedVisualPropertiesWindow)
        return;

    ScopedIMGUIWindow window("Selected Visual");

    if (!m_SelectedVisual)
    {
        ImGui::Text("Select a Visual from the Visuals List window");
        return;
    }

    m_SelectedVisual->UpdateIMGUI();
}

void Scene::UpdateSceneVisualsWindow()
{
    if (!gs_SceneShowSceneVisualsWindow)
        return;

    ScopedIMGUIWindow window("Scene Visuals");

    if (ImGui::Button("Delete Selected Visual") && m_SelectedVisual)
    {
        // remove from container in the beginning of the next engine frame
        Visual* visualToDelete = m_SelectedVisual;
        g_System.AddSystemCommand([&, visualToDelete]()
            {
                bbeMultiThreadDetector();
                auto it = std::find(m_AllVisuals.begin(), m_AllVisuals.end(), visualToDelete);
                assert(it != m_AllVisuals.end());
                std::swap(*it, m_AllVisuals.back());
                m_AllVisuals.pop_back();
            });

        m_SelectedVisual = nullptr;
    }

    for (Visual* visual : m_AllVisuals)
    {
        ScopedIMGUIID scopedID{ visual };
        if (ImGui::Selectable(visual->m_Name.c_str(), m_SelectedVisual == visual))
        {
            m_SelectedVisual = visual;
        }
    }
}
