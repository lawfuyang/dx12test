#pragma once

#include <system/commandmanager.h>

#include <graphic/gfx/gfxdevice.h>
#include <graphic/gfx/gfxswapchain.h>
#include <graphic/gfx/gfxtexturesandbuffers.h>

#include <graphic/view.h>

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

    GfxContext& GenerateNewContext(D3D12_COMMAND_LIST_TYPE, const std::string& name);

    GfxDevice& GetGfxDevice() { return m_GfxDevice; }
    GfxSwapChain& GetSwapChain() { return m_SwapChain; }

    View& GetMainView() { return m_MainView; }

private:
    void ScheduleRenderPasses(tf::Subflow& sf);
    void ScheduleCommandListsExecution();
    void TransitionBackBufferForPresent();
    void UpdateIMGUIPropertyGrid();

    std::mutex m_ContextsLock;
    ObjectPool<GfxContext> m_ContextsPool;
    std::vector<GfxContext*> m_AllContexts;
    std::vector<GfxContext*> m_ScheduledContexts;

    GfxDevice         m_GfxDevice;
    GfxSwapChain      m_SwapChain;

    CommandManager m_GfxCommandManager;

    View m_MainView;
};
#define g_GfxManager GfxManager::GetInstance()
