#pragma once

class Scene
{
    DeclareSingletonFunctions(Scene);

public:
    void Initialize();
    void Update();
    void ShutDown();

private:
    void OpenSceneWindow();
    void UpdateFileDialogAsync();
    void SaveSceneWindow();
    void SaveSceneAsWindow();

    std::string m_CurrentSceneFile;
    pfd::open_file* m_OpenFileDialog = nullptr;
};
#define g_Scene Scene::GetInstance()
