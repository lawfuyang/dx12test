#include <application/scene.h>

#include <system/imguimanager.h>

static bool gs_ShowOpenSceneIMGUIWindow = false;
static bool gs_ShowSaveSceneIMGUIWindow = false;

void Scene::Initialize()
{
    g_IMGUIManager.RegisterTopMenu("Scene", "Open Scene", &gs_ShowOpenSceneIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { OpenSceneWindow(); });

    g_IMGUIManager.RegisterTopMenu("Scene", "Save Scene", &gs_ShowSaveSceneIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { SaveSceneWindow(); });
}

void Scene::Update()
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
