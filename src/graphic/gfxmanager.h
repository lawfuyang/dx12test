#pragma once

#include <system/commandmanager.h>

#include <graphic/gfxdevice.h>
#include <graphic/gfxswapchain.h>
#include <graphic/gfxtexturesandbuffers.h>

class GfxManager
{
    DeclareSingletonFunctions(GfxManager);

public:
    void Initialize(tf::Subflow& subFlow);
    void ShutDown();

    void ScheduleGraphicTasks(tf::Subflow& subFlow);

    void BeginFrame();
    void EndFrame();

    template <typename Lambda>
    void AddGraphicCommand(Lambda&& lambda) { m_GfxCommandManager.AddCommand(std::forward<Lambda>(lambda)); }

    GfxDevice&         GetGfxDevice()   { return m_GfxDevice; }
    GfxSwapChain&      GetSwapChain()   { return m_SwapChain; }
    GfxConstantBuffer& GetFrameParams() { return m_FrameParamsCB; }

private:
    void ScheduleRenderPasses(tf::Subflow& sf);
    void ScheduleCommandListsExecution();
    void TransitionBackBufferForPresent();
    void UpdateFrameParamsCB();
    void UpdateIMGUIPropertyGrid();

    GfxDevice         m_GfxDevice;
    GfxSwapChain      m_SwapChain;
    GfxConstantBuffer m_FrameParamsCB;

    CommandManager m_GfxCommandManager;
};
#define g_GfxManager GfxManager::GetInstance()
