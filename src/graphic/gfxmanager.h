#pragma once

#include "graphic/gfxdevice.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxtexturesandbuffers.h"

class GfxManager
{
    DeclareSingletonFunctions(GfxManager);

public:
    void Initialize();
    void ShutDown();

    void ScheduleGraphicTasks();

    void BeginFrame();
    void EndFrame();

    void AddGraphicCommand(const std::function<void()>& newCmd) { bbeAutoLock(m_GfxCommandsLock); m_GfxCommands.push_back(newCmd); }

    void DumpGfxMemory();

    GfxDevice&         GetGfxDevice()   { return m_GfxDevice; }
    GfxSwapChain&      GetSwapChain()   { return m_SwapChain; }
    GfxConstantBuffer& GetFrameParams() { return m_FrameParamsCB; }

private:
    void ScheduleRenderPasses();
    void TransitionBackBufferForPresent();
    void UpdateFrameParamsCB();

    GfxDevice         m_GfxDevice;
    GfxSwapChain      m_SwapChain;
    GfxConstantBuffer m_FrameParamsCB;

    AdaptiveLock m_GfxCommandsLock{ "m_GfxCommands" };
    std::vector<std::function<void()>> m_GfxCommands;
};
#define g_GfxManager GfxManager::GetInstance()
