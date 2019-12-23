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

    GfxDevice&    GetGfxDevice() { return *m_GfxDevice; }
    GfxSwapChain& GetSwapChain() { return *m_SwapChain; }

private:
    void TransitionBackBufferForPresent();

    GfxDevice*    m_GfxDevice = nullptr;
    GfxSwapChain* m_SwapChain = nullptr;
};
