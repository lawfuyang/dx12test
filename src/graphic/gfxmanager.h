#pragma once

#include "graphic/gfxdevice.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxtexturesandbuffers.h"

class GfxManager
{
    DeclareSingletonFunctions(GfxManager);

public:
    void Initialize(tf::Subflow& subFlow);
    void ShutDown();

    void ScheduleGraphicTasks(tf::Subflow& subFlow);

    void BeginFrame();
    void EndFrame();

    void AddGraphicCommand(const std::function<void()>& newCmd) { bbeAutoLock(m_GfxCommandsLock); m_PendingGfxCommands.push_back(newCmd); }

    void DumpGfxMemory();

    GfxDevice&         GetGfxDevice()   { return m_GfxDevice; }
    GfxSwapChain&      GetSwapChain()   { return m_SwapChain; }
    GfxConstantBuffer& GetFrameParams() { return m_FrameParamsCB; }

private:
    void ScheduleRenderPasses(tf::Subflow& sf);
    void TransitionBackBufferForPresent();
    void UpdateFrameParamsCB();

    GfxDevice         m_GfxDevice;
    GfxSwapChain      m_SwapChain;
    GfxConstantBuffer m_FrameParamsCB;

    std::mutex m_GfxCommandsLock;
    std::vector<std::function<void()>> m_PendingGfxCommands;
    std::vector<std::function<void()>> m_ExecutingGfxCommands;
};
#define g_GfxManager GfxManager::GetInstance()
