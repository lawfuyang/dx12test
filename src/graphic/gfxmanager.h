#pragma once

class GfxDevice;
class GfxSwapChain;
class GUIManager;

class GfxManager
{
    DeclareSingletonFunctions(GfxManager);

public:
    void Initialize();
    void ShutDown();

    void ScheduleGraphicTasks(tf::Subflow&);

    void BeginFrame();
    void Render();
    void EndFrame();

    GfxDevice& GetMainGfxDevice() { return *m_MainDevice; }

    GUIManager& GetGUIManager() { return *m_GUIManager; }

private:
    GfxDevice* m_MainDevice = nullptr;
    GfxSwapChain* m_SwapChain = nullptr;
    GUIManager* m_GUIManager = nullptr;
};
