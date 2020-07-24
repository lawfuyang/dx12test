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
};
#define g_Scene Scene::GetInstance()
