#pragma once

class Scene
{
    DeclareSingletonFunctions(Scene);

public:
    void Initialize();
    void Update();

private:
    void OpenSceneWindow();
    void SaveSceneWindow();
};
#define g_Scene Scene::GetInstance()
