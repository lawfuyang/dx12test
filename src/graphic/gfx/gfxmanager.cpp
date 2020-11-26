#include <graphic/gfx/gfxmanager.h>
#include <graphic/pch.h>

#include <system/imguimanager.h>

static bool gs_ShowGfxManagerIMGUIWindow = false;

extern GfxRendererBase* g_GfxForwardLightingPass;
extern GfxRendererBase* g_GfxIMGUIRenderer;
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
    ADD_SF_TASK(subFlow, g_GfxShaderManager.Initialize(sf));
    ADD_TF_TASK(subFlow, g_GfxDefaultVertexFormats.Initialize());
    
    // tasks with dependencies
    tf::Task cmdListInitTask              = ADD_TF_TASK(subFlow, g_GfxCommandListsManager.Initialize());
    tf::Task swapChainInitTask            = ADD_TF_TASK(subFlow, m_SwapChain.Initialize());
    tf::Task PSOManagerInitTask           = ADD_TF_TASK(subFlow, g_GfxPSOManager.Initialize());
    tf::Task dynamicDescHeapAllocatorInit = ADD_TF_TASK(subFlow, g_GfxGPUDescriptorAllocator.Initialize());
    tf::Task lightsManagerInit            = ADD_TF_TASK(subFlow, g_GfxLightsManager.Initialize());

    tf::Task adapterAndDeviceInit = subFlow.emplace([&](tf::Subflow& sf)
        {
            g_GfxAdapter.Initialize();
            m_GfxDevice.Initialize();
        }).name("adapterAndDeviceInit");

    tf::Task miscGfxInitTask = subFlow.emplace([&](tf::Subflow& sf)
        {
            bbeProfile("General Gfx Init");

            tf::Task allTasks[] =
            {
                ADD_SF_TASK(sf, g_GfxDefaultAssets.Initialize(sf)),
                ADD_TF_TASK(sf, g_GfxForwardLightingPass->Initialize()),
                ADD_TF_TASK(sf, g_GfxIMGUIRenderer->Initialize()),
            };

            // HUGE assumption that all of the above gfx tasks queued their command lists to be executed
            //tf::Task flushAndWaitTask = ADD_TF_TASK(subFlow, m_GfxDevice.Flush(true /* andWait */));
            tf::Task flushAndWaitTask = sf.emplace([this] 
                {
                    g_GfxCommandListsManager.ExecutePendingCommandLists();

                    // TODO: check that every other queue except the main Direct queue is empty
                    g_GfxCommandListsManager.GetMainQueue().SignalFence();
                    g_GfxCommandListsManager.GetMainQueue().StallCPUForFence();

                    // reset array of GfxContexts to prepare for next frame
                    std::for_each(m_AllContexts.begin(), m_AllContexts.end(), [this](GfxContext* context) { m_ContextsPool.destroy(context); });
                    m_AllContexts.clear();
                });
            for (tf::Task task : allTasks)
            {
                flushAndWaitTask.succeed(task);
            }
        }).name("generalGfxInitTask");

    adapterAndDeviceInit.precede(cmdListInitTask, PSOManagerInitTask, miscGfxInitTask, swapChainInitTask, dynamicDescHeapAllocatorInit);
    miscGfxInitTask.succeed(cmdListInitTask);
    swapChainInitTask.succeed(cmdListInitTask);
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    m_GfxCommandManager.ConsumeAllCommandsST(true);

    // get swapchain out of full screen before exiting
    BOOL fs = false;
    DX12_CALL(m_SwapChain.Dev()->GetFullscreenState(&fs, NULL));
    if (fs)
        m_SwapChain.Dev()->SetFullscreenState(false, NULL);

    g_GfxPSOManager.ShutDown();
    g_GfxForwardLightingPass->ShutDown();
    g_GfxIMGUIRenderer->ShutDown();
    g_GfxDefaultAssets.ShutDown();
    g_GfxResourceManager.ShutDown();

    m_GfxCommandManager.ConsumeAllCommandsST(true);

    m_GfxDevice.ShutDown();
}

void GfxManager::ScheduleGraphicTasks(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    tf::Task BEGIN_FRAME_GATE = subFlow.placeholder();
    tf::Task RENDERERS_GATE = subFlow.placeholder().succeed(BEGIN_FRAME_GATE);

    subFlow.emplace([this](tf::Subflow& sf) { m_GfxCommandManager.ConsumeAllCommandsMT(sf); }).precede(BEGIN_FRAME_GATE);
    subFlow.emplace([this] { BeginFrame(); }).precede(BEGIN_FRAME_GATE);

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

    subFlow.emplace([this] { EndFrame(); }).succeed(RENDERERS_GATE);
}

void GfxManager::BeginFrame()
{
    m_GfxDevice.CheckStatus();

    g_GfxGPUDescriptorAllocator.GarbageCollect();

    // TODO: Get rid of this
    g_GfxCommandListsManager.GetMainQueue().StallCPUForFence();

    // TODO: Remove clearing of BackBuffer when we manage to fill every pixel on screen through various render passes
    GfxContext& context = GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "TransitionBackBufferForPresent");
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

    // reset array of GfxContexts to prepare for next frame
    std::for_each(m_AllContexts.begin(), m_AllContexts.end(), [this](GfxContext* context) { m_ContextsPool.destroy(context); });
    m_AllContexts.clear();

    ++m_GraphicFrameNumber;
}

GfxContext& GfxManager::GenerateNewContext(D3D12_COMMAND_LIST_TYPE cmdListType, const std::string& name)
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
        const std::string statsString = StringUtils::WideToUtf8(statsStringW);
        allocator.FreeStatsString(statsStringW);

        ScopedIMGUIWindow window{ "Detailed Gfx Stats" };
        ImGui::Text("%s", statsString.c_str());
    }
}
