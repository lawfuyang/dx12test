#include <application/scene.h>

#include <system/imguimanager.h>

#include <graphic/visual.h>
#include <graphic/gfx/gfxdefaultassets.h>

static bool gs_SceneShowSelectedVisualPropertiesWindow = true;

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
    g_IMGUIManager.RegisterFileDialog("Scene::OpenSceneWindow", ".scene", [](const std::string&) {}); // TODO: Finalizer
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
    g_IMGUIManager.RegisterFileDialog("Scene::OpenSceneWindow", ".scene", [](const std::string&) {}); // TODO: Finalizer
}

void Scene::AddNewVisual()
{
    Visual* newVisual = m_VisualsPool.construct();
    newVisual->m_ObjectID = GenerateObjectID();

    assert(m_AllVisuals.size() < BBE_KB(1)); // Ensure it doesnt grow and invalid all Visual ptrs in the engine
    m_AllVisuals.push_back(newVisual);

    m_SelectedVisual = newVisual;

    g_Log.info("New Visual: {}", ToString(newVisual->m_ObjectID));
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

    m_SelectedVisual->UpdatePropertiesIMGUI();
}
