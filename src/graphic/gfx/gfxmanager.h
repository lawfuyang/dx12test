#pragma once

#include <system/commandmanager.h>

#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxdevice.h>
#include <graphic/gfx/gfxswapchain.h>

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

    template <typename Lambda>
    void AddDoubleDeferredGraphicCommand(Lambda&& lambda)
    {
        auto deferredCmd = [this, lambda]() { AddGraphicCommand(lambda); };
        AddGraphicCommand(deferredCmd);
    }

    GfxContext& GenerateNewContext(D3D12_COMMAND_LIST_TYPE, const std::string& name);

    GfxDevice& GetGfxDevice() { return m_GfxDevice; }
    GfxSwapChain& GetSwapChain() { return m_SwapChain; }

    View& GetMainView() { return m_MainView; }

    uint32_t GetGraphicFrameNumber() const { return m_GraphicFrameNumber; }

private:
    GfxContext& GenerateNewContextInternal();
    void UpdateIMGUIPropertyGrid();

    static const uint32_t NbMaxContexts = 128;

    std::mutex m_ContextsLock;
    ObjectPool<GfxContext> m_ContextsPool;
    InplaceArray<GfxContext*, NbMaxContexts> m_AllContexts;
    InplaceArray<GfxCommandList*, NbMaxContexts> m_ScheduledCmdLists;

    GfxDevice         m_GfxDevice;
    GfxSwapChain      m_SwapChain;

    CommandManager m_GfxCommandManager;

    View m_MainView;

    uint32_t m_GraphicFrameNumber = 0;
};
#define g_GfxManager GfxManager::GetInstance()
