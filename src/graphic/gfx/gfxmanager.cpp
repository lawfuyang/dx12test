#include <graphic/gfx/gfxmanager.h>
#include <graphic/pch.h>

#include <system/imguimanager.h>

static bool gs_ShowGfxManagerIMGUIWindow = false;

extern GfxRendererBase* g_GfxForwardLightingPass;
extern GfxRendererBase* g_GfxIMGUIRenderer;
extern GfxRendererBase* g_GfxBodyGravityParticlesUpdate;
extern GfxRendererBase* g_GfxBodyGravityParticlesRender;
extern GfxTexture g_SceneDepthBuffer;

void InitializeGraphic(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    g_GfxManager.Initialize(subFlow);
}

void ShutdownGraphic()
{
    bbeProfileFunction();

    g_GfxManager.ShutDown();
}

void UpdateGraphic(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    g_GfxManager.ScheduleGraphicTasks(subFlow);
}

void GfxManager::Initialize(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    m_GfxCommandManager.Initialize();

    g_IMGUIManager.RegisterTopMenu("Graphic", "GfxManager", &gs_ShowGfxManagerIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUIPropertyGrid(); });

    // independent tasks
    subFlow.emplace([](tf::Subflow& sf) { g_GfxShaderManager.Initialize(sf); });
    subFlow.emplace([] { g_GfxDefaultVertexFormats.Initialize(); });
    subFlow.emplace([] { g_GfxLightsManager.Initialize(); });

    tf::Task PRE_INIT_GATE = subFlow.emplace([this](tf::Subflow& sf) { PreInit(sf); });
    tf::Task POST_INIT_GATE = subFlow.emplace([this] { PostInit(); }).succeed(PRE_INIT_GATE);
    
    // tasks with dependencies
    subFlow.emplace([this] { m_SwapChain.Initialize(); }).succeed(PRE_INIT_GATE).precede(POST_INIT_GATE);
    subFlow.emplace([] { g_GfxPSOManager.Initialize(); }).succeed(PRE_INIT_GATE).precede(POST_INIT_GATE);
    subFlow.emplace([] { g_GfxGPUDescriptorAllocator.Initialize(); }).succeed(PRE_INIT_GATE).precede(POST_INIT_GATE);
    subFlow.emplace([](tf::Subflow& sf) { g_GfxDefaultAssets.Initialize(sf); }).succeed(PRE_INIT_GATE).precede(POST_INIT_GATE);

    for (GfxRendererBase* renderer : GfxRendererBase::ms_AllRenderers)
    {
        subFlow.emplace([renderer] { renderer->Initialize(); }).succeed(PRE_INIT_GATE).precede(POST_INIT_GATE);
    }
}

void GfxManager::PreInit(tf::Subflow& sf)
{
    bbeProfileFunction();

    g_GfxAdapter.Initialize();
    m_GfxDevice.Initialize();
    g_GfxMemoryAllocator.Initialize();
    g_GfxCommandListsManager.Initialize(sf);
}

void GfxManager::PostInit()
{
    bbeProfileFunction();

    g_GfxCommandListsManager.ExecutePendingCommandLists();

    // TODO: use a master GfxFence in GfxManager
    g_GfxCommandListsManager.GetMainQueue().SignalFence();
    g_GfxCommandListsManager.GetMainQueue().StallCPUForFence();

    // reset array of GfxContexts to prepare for next frame
    std::for_each(m_AllContexts.begin(), m_AllContexts.end(), [this](GfxContext* context) { m_ContextsPool.destroy(context); });
    m_AllContexts.clear();
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    g_GfxCommandListsManager.GetMainQueue().StallCPUForFence();

    m_GfxCommandManager.ConsumeAllCommandsST(true);

    // get swapchain out of full screen before exiting
    BOOL fs = false;
    DX12_CALL(m_SwapChain.Dev()->GetFullscreenState(&fs, NULL));
    if (fs)
        m_SwapChain.Dev()->SetFullscreenState(false, NULL);

    g_GfxPSOManager.ShutDown();
    g_GfxMemoryAllocator.ShutDown();
    g_GfxCommandListsManager.ShutDown();

    m_GfxCommandManager.ConsumeAllCommandsST(true);
}

void GfxManager::ScheduleGraphicTasks(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    tf::Task WAIT_PREV_FRAME_GATE = subFlow.placeholder();
    tf::Task BEGIN_FRAME_GATE = subFlow.placeholder().succeed(WAIT_PREV_FRAME_GATE);
    tf::Task RENDERERS_GATE = subFlow.placeholder().succeed(BEGIN_FRAME_GATE);

    // TODO: This will kill performance with gpu debug layer enabled!
    // Implement a proper way of handling previous frame gpu resources
    // This should really be called before execution of the first renderer
    subFlow.emplace([] { g_GfxCommandListsManager.GetMainQueue().StallCPUForFence(); }).precede(WAIT_PREV_FRAME_GATE);

    subFlow.emplace([this](tf::Subflow& sf) { m_GfxCommandManager.ConsumeAllCommandsMT(sf); }).succeed(WAIT_PREV_FRAME_GATE).precede(BEGIN_FRAME_GATE);
    subFlow.emplace([this] { BeginFrame(); }).succeed(WAIT_PREV_FRAME_GATE).precede(BEGIN_FRAME_GATE);

    auto PopulateCommandList = [&, this](GfxRendererBase* renderer)
    {
        tf::Task tf;

        GfxContext& context = GenerateNewContextInternal();
        if (renderer->ShouldPopulateCommandList(context))
        {
            // TODO: different cmd list type based on renderer type (async compute or direct)
            context.Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT, renderer->GetName());
            m_ScheduledCmdLists.push_back(&context.GetCommandList());
            tf = subFlow.emplace([&context, renderer]() { renderer->PopulateCommandList(context); }).name(renderer->GetName());
        }
        else
        {
            // Do nothing if no need to render
            tf = subFlow.placeholder();
        }

        tf.succeed(BEGIN_FRAME_GATE).precede(RENDERERS_GATE);
    };
    PopulateCommandList(g_GfxForwardLightingPass);
    PopulateCommandList(g_GfxIMGUIRenderer);
    PopulateCommandList(g_GfxBodyGravityParticlesUpdate);
    PopulateCommandList(g_GfxBodyGravityParticlesRender);

    subFlow.emplace([this] { EndFrame(); }).succeed(RENDERERS_GATE);
}

void GfxManager::BeginFrame()
{
    m_GfxDevice.CheckStatus();

    g_GfxMemoryAllocator.GarbageCollect();

    // TODO: Remove clearing of BackBuffer when we manage to fill every pixel on screen through various render passes
    GfxContext& context = GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "ClearBackBuffer & DepthBuffer");
    context.ClearRenderTargetView(m_SwapChain.GetCurrentBackBuffer(), bbeVector4{ 0.0f, 0.2f, 0.4f, 1.0f });
    context.ClearDepth(g_SceneDepthBuffer, 1.0f);
    g_GfxCommandListsManager.QueueCommandListToExecute(&context.GetCommandList());
}

void GfxManager::EndFrame()
{
    bbeProfileFunction();

    // execute all Renderers' cmd lists
    std::for_each(m_ScheduledCmdLists.begin(), m_ScheduledCmdLists.end(), [](GfxCommandList* cmdList) { g_GfxCommandListsManager.QueueCommandListToExecute(cmdList); });
    m_ScheduledCmdLists.clear();

    // Before presenting backbuffer, transition to to PRESENT state
    GfxContext& context = GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "TransitionBackBufferForPresent");
    context.TransitionResource(m_SwapChain.GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, true);
    g_GfxCommandListsManager.QueueCommandListToExecute(&context.GetCommandList());

    // Finally, execute all cmd lists for this frame
    g_GfxCommandListsManager.ExecutePendingCommandLists();

    // finally, present back buffer
    m_SwapChain.Present();

    // TODO: use a master GfxFence in GfxManager
    g_GfxCommandListsManager.GetMainQueue().SignalFence();

    // reset array of GfxContexts to prepare for next frame
    std::for_each(m_AllContexts.begin(), m_AllContexts.end(), [this](GfxContext* context) { m_ContextsPool.destroy(context); });
    m_AllContexts.clear();

    ++m_GraphicFrameNumber;
}

GfxContext& GfxManager::GenerateNewContext(D3D12_COMMAND_LIST_TYPE cmdListType, std::string_view name)
{
    GfxContext& newContext = GenerateNewContextInternal();
    newContext.Initialize(cmdListType, name);

    return newContext;
}

GfxContext& GfxManager::GenerateNewContextInternal()
{
    bbeProfileFunction();

    bbeAutoLock(m_ContextsLock);
    GfxContext* ret = m_ContextsPool.construct();
    m_AllContexts.push_back(ret);
    return *ret;
}

void GfxManager::UpdateIMGUIPropertyGrid()
{
    if (!gs_ShowGfxManagerIMGUIWindow)
        return;

    bbeProfileFunction();

    D3D12MA::Allocator& allocator = g_GfxMemoryAllocator.Dev();

    static bool showMemoryStats = false;
    static bool showDetailedStats = false;

    bool* pOpen = nullptr;
    ScopedIMGUIWindow window{ "GfxManager" };
    ImGui::Checkbox("Show Memory Stats", &showMemoryStats);

    if (showMemoryStats)
    {
        D3D12MA::Budget gpuBudget;
        D3D12MA::Budget cpuBudget;
        allocator.GetBudget(&gpuBudget, &cpuBudget);

        ImGui::Text("GPU Budget:");
        ImGui::LabelText("Blocks", "\t%.2f mb", (float)gpuBudget.BlockBytes / 1024 / 1024);
        ImGui::LabelText("Allocations", "\t%.2f mb", (float)gpuBudget.AllocationBytes / 1024 / 1024);
        ImGui::LabelText("Usage", "\t%.2f mb", (float)gpuBudget.UsageBytes / 1024 / 1024);
        ImGui::LabelText("Budget", "\t%.2f mb", (float)gpuBudget.BudgetBytes / 1024 / 1024);

        ImGui::NewLine();

        ImGui::Text("CPU Budget:");
        ImGui::LabelText("Blocks", "\t%.2f mb", (float)cpuBudget.BlockBytes / 1024 / 1024);
        ImGui::LabelText("Allocations", "\t%.2f mb", (float)cpuBudget.AllocationBytes / 1024 / 1024);
        ImGui::LabelText("Usage", "\t%.2f mb", (float)cpuBudget.UsageBytes / 1024 / 1024);
        ImGui::LabelText("Budget", "\t%.2f mb", (float)cpuBudget.BudgetBytes / 1024 / 1024);

        ImGui::NewLine();

        ImGui::Checkbox("Show Detailed Stats", &showDetailedStats);
    }

    if (showDetailedStats)
    {
        WCHAR* statsStringW = NULL;
        allocator.BuildStatsString(&statsStringW, allocator.GetD3D12Options().ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2);
        const char* statsString = StringUtils::WideToUtf8Big(statsStringW);
        allocator.FreeStatsString(statsStringW);

        ScopedIMGUIWindow window{ "Detailed Gfx Stats" };
        ImGui::Text("%s", statsString);
    }
}
