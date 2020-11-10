#include <graphic/gfx/gfxmanager.h>

#include <graphic/dx12utils.h>
#include <graphic/gfxresourcemanager.h>
#include <graphic/gfx/gfxadapter.h>
#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxshadermanager.h>
#include <graphic/gfx/gfxvertexformat.h>
#include <graphic/gfx/gfxdefaultassets.h>
#include <graphic/gfx/gfxtexturesandbuffers.h>
#include <graphic/gfx/gfxlightsmanager.h>
#include <graphic/renderers/gfxrendererbase.h>

#include <system/imguimanager.h>

static bool gs_ShowGfxManagerIMGUIWindow = false;

extern GfxRendererBase* g_GfxForwardLightingPass;
extern GfxRendererBase* g_GfxIMGUIRenderer;
extern GfxTexture gs_SceneDepthBuffer;

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

    tf::Task miscGfxInitTask = subFlow.emplace([&](tf::Subflow& subFlow)
        {
            bbeProfile("General Gfx Init");

            tf::Task allTasks[] =
            {
                ADD_SF_TASK(subFlow, g_GfxDefaultAssets.Initialize(sf)),
                ADD_TF_TASK(subFlow, g_GfxForwardLightingPass->Initialize()),
                ADD_TF_TASK(subFlow, g_GfxIMGUIRenderer->Initialize()),
            };

            // HUGE assumption that all of the above gfx tasks queued their command lists to be executed
            tf::Task flushAndWaitTask = ADD_TF_TASK(subFlow, m_GfxDevice.Flush(true /* andWait */));
            for (tf::Task task : allTasks)
            {
                flushAndWaitTask.succeed(task);
            }
        }).name("generalGfxInitTask");

    adapterAndDeviceInit.precede(cmdListInitTask, PSOManagerInitTask, miscGfxInitTask, swapChainInitTask, dynamicDescHeapAllocatorInit);
    miscGfxInitTask.succeed(cmdListInitTask);
    swapChainInitTask.succeed(cmdListInitTask);

    m_AllContexts.reserve(128);
    m_ScheduledContexts.reserve(128);
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    m_GfxDevice.IncrementAndSignalFence();
    m_GfxDevice.WaitForFence();

    // finish all commands before shutting down
    bool hasCommands = true;
    do
    {
        hasCommands = m_GfxCommandManager.ConsumeOneCommand();
    } while (hasCommands);

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

    // we must complete the previous GPU frame before exiting the app
    m_GfxDevice.IncrementAndSignalFence();
    m_GfxDevice.WaitForFence();
    m_GfxDevice.ShutDown();
}

void GfxManager::ScheduleGraphicTasks(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    // consume all gfx commmands multi-threaded
    tf::Task gfxCommandsConsumptionTasks = ADD_SF_TASK(subFlow, m_GfxCommandManager.ConsumeAllCommandsMT(sf));
    tf::Task beginFrameTask              = ADD_TF_TASK(subFlow, BeginFrame());
    tf::Task endFrameTask                = ADD_TF_TASK(subFlow, EndFrame());
    tf::Task renderersScheduleTask       = ADD_SF_TASK(subFlow, ScheduleRenderPasses(sf));
    tf::Task cmdListsExecTask            = ADD_TF_TASK(subFlow, ScheduleCommandListsExecution());

    gfxCommandsConsumptionTasks.precede(beginFrameTask);
    beginFrameTask.precede(renderersScheduleTask);
    cmdListsExecTask.succeed(renderersScheduleTask);
    endFrameTask.succeed(cmdListsExecTask);
}

void GfxManager::ScheduleRenderPasses(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    auto ScheduleRenderPass = [](GfxRendererBase* renderer)
    {
        // TODO: different cmd list type based on renderer type (async compute or direct)
        GfxContext& context = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, renderer->GetName());
        renderer->PopulateCommandList(context);
        g_GfxManager.m_ScheduledContexts.push_back(&context);
    };

    // Manual scheduling
    ADD_TF_TASK(subFlow, ScheduleRenderPass(g_GfxForwardLightingPass));
    ADD_TF_TASK(subFlow, ScheduleRenderPass(g_GfxIMGUIRenderer));
}

void GfxManager::ScheduleCommandListsExecution()
{
    bbeProfileFunction();

    // schedule all render passes that were manually scheduled in "ScheduleRenderPasses"
    for (GfxContext* context : m_ScheduledContexts)
    {
        g_GfxCommandListsManager.QueueCommandListToExecute(context->GetCommandList(), context->GetCommandList().GetType());
    }

    // immediately clear array of scheduled renderers after queueing them for execution
    m_ScheduledContexts.clear();

    // No more draw calls directly to the Back Buffer beyond this point!
    TransitionBackBufferForPresent();

    // execute all command lists
    m_GfxDevice.Flush();
}

void GfxManager::BeginFrame()
{
    bbeProfileFunction();

    // wait for GPU to be done with previous frame
    m_GfxDevice.WaitForFence();

    // check for DXGI_ERRORs on all GfxDevices
    m_GfxDevice.CheckStatus();

    g_GfxGPUDescriptorAllocator.CleanupUsedHeaps();

    // TODO: Remove clearing of BackBuffer when we manage to fill every pixel on screen through various render passes
    {
        GfxContext& clearBackBufferContext = GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "ClearBackBuffer");
        clearBackBufferContext.ClearRenderTargetView(g_GfxManager.GetSwapChain().GetCurrentBackBuffer(), bbeVector4{ 0.0f, 0.2f, 0.4f, 1.0f });
        clearBackBufferContext.ClearDepth(gs_SceneDepthBuffer, 1.0f);
        g_GfxCommandListsManager.QueueCommandListToExecute(clearBackBufferContext.GetCommandList(), clearBackBufferContext.GetCommandList().GetType());

        m_GfxDevice.Flush();
    }
}

void GfxManager::EndFrame()
{
    bbeProfileFunction();
    bbeMultiThreadDetector();

    m_GfxDevice.EndFrame();
    m_SwapChain.Present();
    m_GfxDevice.IncrementAndSignalFence();

    assert(m_ScheduledContexts.empty());
    for (GfxContext* context : m_AllContexts)
    {
        m_ContextsPool.destroy(context);
    }
    m_AllContexts.clear();
}

GfxContext& GfxManager::GenerateNewContext(D3D12_COMMAND_LIST_TYPE cmdListType, const std::string& name)
{
    bbeProfileFunction();

    //g_Log.info("*** GfxManager::GenerateNewContext: {}", name);

    GfxContext* newContext = [this]()
    {
        bbeAutoLock(m_ContextsLock);
        GfxContext* ret = m_ContextsPool.construct();
        m_AllContexts.push_back(ret);
        return ret;
    }();
    newContext->Initialize(cmdListType, name);

    return *newContext;
}

void GfxManager::TransitionBackBufferForPresent()
{
    bbeProfileFunction();

    GfxContext& context = GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "TransitionBackBufferForPresent");
    SetD3DDebugName(context.GetCommandList().Dev(), "TransitionBackBufferForPresent");

    m_SwapChain.TransitionBackBufferForPresent(context);

    g_GfxCommandListsManager.QueueCommandListToExecute(context.GetCommandList(), context.GetCommandList().GetType());
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
