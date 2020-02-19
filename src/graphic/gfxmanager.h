#pragma once

class GfxDevice;
class GfxSwapChain;
class GUIManager;
class GfxRootSignatureManager;
class GfxConstantBuffer;

class GfxManager
{
    DeclareSingletonFunctions(GfxManager);

public:
    void Initialize(tf::Taskflow&);
    void ShutDown();

    void ScheduleGraphicTasks(tf::Taskflow&);

    void BeginFrame();
    void EndFrame();

    void AddGraphicCommand(const std::function<void()>& newCmd) { m_GfxCommands.push(newCmd); }

    void DumpGfxMemory();

    GfxDevice&         GetGfxDevice()   { return *m_GfxDevice; }
    GfxSwapChain&      GetSwapChain()   { return *m_SwapChain; }
    GfxConstantBuffer& GetFrameParams() { return *m_FrameParamsCB; }

private:
    void ScheduleRenderPasses(tf::Subflow& sf);
    void TransitionBackBufferForPresent();
    void UpdateFrameParamsCB();

    GfxDevice*         m_GfxDevice     = nullptr;
    GfxSwapChain*      m_SwapChain     = nullptr;
    GfxConstantBuffer* m_FrameParamsCB = nullptr;

    boost::lockfree::stack<std::function<void()>> m_GfxCommands{128};
};
#define g_GfxManager GfxManager::GetInstance()
