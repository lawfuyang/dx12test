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
    void SaveSceneWindow();
    void SaveSceneAsWindow();
    void AddVisual();

    std::string m_CurrentSceneFile;
};
#define g_Scene Scene::GetInstance()
