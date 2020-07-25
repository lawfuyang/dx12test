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
    void UpdateSelectedVisualPropertiesWindow();

    std::string m_CurrentSceneFile;

    Visual* m_SelectedVisual = nullptr;
    InplaceArray<Visual*, BBE_KB(1)> m_AllVisuals;
    ObjectPool<Visual> m_VisualsPool;
};
#define g_Scene Scene::GetInstance()
