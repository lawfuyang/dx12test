#pragma once

class Visual;

class Scene
{
    DeclareSingletonFunctions(Scene);

public:
    void Initialize();
    void Update();
    void ShutDown();

    Visual* GetVisualFromID(ObjectID id);

private:
    void OpenSceneWindow();
    void SaveSceneWindow();
    void SaveSceneAsWindow();
    void AddNewVisual();

    std::string m_CurrentSceneFile;

    ObjectID m_SelectedVisual = ID_InvalidObject;
    std::vector<Visual*> m_AllVisuals;
    ObjectPool<Visual> m_VisualsPool;
};
#define g_Scene Scene::GetInstance()
