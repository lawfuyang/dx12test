#pragma once

#include <system/commandmanager.h>

#include <graphic/gfx/gfxdevice.h>
#include <graphic/gfx/gfxswapchain.h>
#include <graphic/gfx/gfxtexturesandbuffers.h>

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

    GfxDevice&         GetGfxDevice()        { return m_GfxDevice; }
    GfxSwapChain&      GetSwapChain()        { return m_SwapChain; }
    GfxConstantBuffer& GetFrameParams()      { return m_FrameParamsCB; }
    GfxTexture&        GetSceneDepthBuffer() { return m_SceneDepthBuffer; }

private:
    void ScheduleRenderPasses(tf::Subflow& sf);
    void ScheduleCommandListsExecution();
    void TransitionBackBufferForPresent();
    void UpdateFrameParamsCB();
    void UpdateIMGUIPropertyGrid();
    void InitSceneDepthBuffer();

    std::mutex m_ContextsLock;
    std::vector<GfxContext> m_AllContexts;

    GfxDevice         m_GfxDevice;
    GfxSwapChain      m_SwapChain;
    GfxConstantBuffer m_FrameParamsCB;
    GfxTexture        m_SceneDepthBuffer;

    CommandManager m_GfxCommandManager;
};
#define g_GfxManager GfxManager::GetInstance()
