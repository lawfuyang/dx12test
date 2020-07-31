#pragma once

class Visual;

static const uint32_t C_VISUALS_ARRAY_SZ = BBE_KB(1);
using VisualsArray = InplaceArray<Visual*, C_VISUALS_ARRAY_SZ>;

class Scene
{
    DeclareSingletonFunctions(Scene);

public:
    void Initialize();
    void Update();
    void ShutDown();

    Visual* GetVisualFromID(ObjectID id);

    void OpenSceneWindow();
    void SaveSceneWindow();
    void SaveSceneAsWindow();
    void AddNewVisual();
    void UpdateSelectedVisualPropertiesWindow();
    void UpdateSceneVisualsWindow();

    std::string m_CurrentSceneFile;
    Visual* m_SelectedVisual = nullptr;

    VisualsArray m_AllVisuals;
    ObjectPool<Visual> m_VisualsPool;
};
#define g_Scene Scene::GetInstance()
