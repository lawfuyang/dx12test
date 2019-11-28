#pragma once

class GfxDevice;
class GfxSwapChain;
class GUIManager;
class GfxRootSignatureManager;

class GfxManager
{
    DeclareSingletonFunctions(GfxManager);

public:
    void Initialize();
    void ShutDown();

    void ScheduleGraphicTasks(tf::Taskflow&);

    void BeginFrame();
    void EndFrame();

    GfxDevice& GetGfxDevice() { return *m_GfxDevice; }
    GfxSwapChain& GetSwapChain() { return *m_SwapChain; }
    GUIManager& GetGUIManager() { return *m_GUIManager; }

private:
    GfxDevice* m_GfxDevice = nullptr;
    GfxSwapChain* m_SwapChain = nullptr;
    GUIManager* m_GUIManager = nullptr;
};
