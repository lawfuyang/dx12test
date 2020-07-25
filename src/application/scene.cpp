#include <application/scene.h>

#include <system/imguimanager.h>

static bool g_SceneOpenSceneWindow = false;

void Scene::Initialize()
{
    g_IMGUIManager.RegisterTopMenu("Scene", "Open Scene", &g_SceneOpenSceneWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { OpenSceneWindow(); });

    //g_IMGUIManager.RegisterTopMenu("Scene", "Save Scene");
    //g_IMGUIManager.RegisterWindowUpdateCB([&]() { SaveSceneWindow(); });

    //g_IMGUIManager.RegisterTopMenu("Scene", "Save Scene as...");
    //g_IMGUIManager.RegisterWindowUpdateCB([&]() { SaveSceneAsWindow(); });
}

void Scene::Update()
{

}

void Scene::ShutDown()
{

}

void Scene::OpenSceneWindow()
{
    if (!g_SceneOpenSceneWindow)
        return;
    g_SceneOpenSceneWindow = false;
}

void Scene::SaveSceneWindow()
{

}

void Scene::SaveSceneAsWindow()
{

}
