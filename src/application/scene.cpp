#include <application/scene.h>

#include <system/imguimanager.h>

#include <graphic/visual.h>

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

    // reserve a big chunk so it will never grow. For Thread safety
    m_AllVisuals.reserve(BBE_KB(1));
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

}
