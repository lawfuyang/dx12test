#include <application/scene.h>

#include <system/imguimanager.h>

static bool gs_ShowOpenSceneIMGUIWindow = false;
static bool gs_ShowSaveSceneIMGUIWindow = false;
static bool gs_ShowSaveSceneAsIMGUIWindow = false;

void Scene::Initialize()
{
    g_IMGUIManager.RegisterTopMenu("Scene", "Open Scene", &gs_ShowOpenSceneIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { OpenSceneWindow(); });

    g_IMGUIManager.RegisterTopMenu("Scene", "Save Scene", &gs_ShowSaveSceneIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { SaveSceneWindow(); });

    g_IMGUIManager.RegisterTopMenu("Scene", "Save Scene as...", &gs_ShowSaveSceneAsIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { SaveSceneAsWindow(); });
}

void Scene::Update()
{

}

void Scene::ShutDown()
{

}

void Scene::OpenSceneWindow()
{
    if (!gs_ShowOpenSceneIMGUIWindow)
        return;
}

void Scene::SaveSceneWindow()
{
    if (!gs_ShowSaveSceneIMGUIWindow)
        return;
}

void Scene::SaveSceneAsWindow()
{
    if (!gs_ShowSaveSceneAsIMGUIWindow)
        return;
}
