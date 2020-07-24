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

void Scene::UpdateFileDialogAsync()
{
    if (!m_OpenFileDialog->ready(0))
    {
        g_System.AddBGAsyncCommand([&]() { UpdateFileDialogAsync(); });
        return;
    }

    if (m_OpenFileDialog->result().size())
        m_CurrentSceneFile = m_OpenFileDialog->result()[0];

    delete m_OpenFileDialog;
    m_OpenFileDialog = nullptr;
}

void Scene::OpenSceneWindow()
{
    if (!gs_ShowOpenSceneIMGUIWindow)
        return;
    gs_ShowOpenSceneIMGUIWindow = false;

    if (!m_OpenFileDialog)
    {
        m_OpenFileDialog = new pfd::open_file("Select Scene", "..\\bin\\assets\\Scenes", { "bbescene Files", "*.bbescene" }, pfd::opt::none);
        UpdateFileDialogAsync();
    }
}

void Scene::SaveSceneWindow()
{
    if (!gs_ShowSaveSceneIMGUIWindow)
        return;
    gs_ShowSaveSceneIMGUIWindow = false;
}

void Scene::SaveSceneAsWindow()
{
    if (!gs_ShowSaveSceneAsIMGUIWindow)
        return;
    gs_ShowSaveSceneAsIMGUIWindow = false;
}
