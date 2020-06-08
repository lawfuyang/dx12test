#include <graphic/gfx/gfxmanager.h>

#include <graphic/dx12utils.h>
#include <graphic/gfx/gfxadapter.h>
#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxshadermanager.h>
#include <graphic/gfx/gfxvertexformat.h>
#include <graphic/gfx/gfxdefaultassets.h>
#include <graphic/gfx/gfxtexturesandbuffers.h>

#include <system/imguimanager.h>

#include <graphic/renderers/gfxzprepassrenderer.h>
#include <graphic/renderers/gfxtestrenderpass.h>
#include <graphic/renderers/gfximguirenderer.h>

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

    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUIPropertyGrid(); });

    // independent tasks
    ADD_SF_TASK(subFlow, g_GfxShaderManager.Initialize(sf));
    ADD_TF_TASK(subFlow, g_GfxDefaultVertexFormats.Initialize());
    
    // tasks with dependencies
    tf::Task cmdListInitTask              = ADD_TF_TASK(subFlow, m_GfxDevice.GetCommandListsManager().Initialize());
    tf::Task swapChainInitTask            = ADD_TF_TASK(subFlow, m_SwapChain.Initialize());
    tf::Task PSOManagerInitTask           = ADD_TF_TASK(subFlow, g_GfxPSOManager.Initialize());
    tf::Task defaultsAssetsPreInit        = ADD_SF_TASK(subFlow, g_GfxDefaultAssets.PreInitialize(sf));
    tf::Task dynamicDescHeapAllocatorInit = ADD_TF_TASK(subFlow, g_GfxGPUDescriptorAllocator.Initialize());

    tf::Task adapterAndDeviceInit = subFlow.emplace([&](tf::Subflow& sf)
        {
            g_GfxAdapter.Initialize();
            m_GfxDevice.Initialize();
        }).name("adapterAndDeviceInit");

    tf::Task miscGfxInitTask = subFlow.emplace([&](tf::Subflow& subFlow)
        {
            bbeProfile("General Gfx Init");

            InplaceArray<tf::Task, 6> allTasks;

            allTasks.push_back(ADD_SF_TASK(subFlow, g_GfxDefaultAssets.Initialize(sf)));
            allTasks.push_back(ADD_TF_TASK(subFlow, g_GfxTestRenderPass.Initialize()));
            allTasks.push_back(ADD_TF_TASK(subFlow, g_GfxIMGUIRenderer.Initialize()));
            allTasks.push_back(ADD_TF_TASK(subFlow, g_ZPrePassRenderer.Initialize()));
            allTasks.push_back(ADD_TF_TASK(subFlow, InitSceneDepthBuffer()));

            // HUGE assumption that all of the above gfx tasks queued their command lists to be executed
            tf::Task flushAndWaitTask = ADD_TF_TASK(subFlow, m_GfxDevice.Flush(true /* andWait */));
            for (tf::Task task : allTasks)
            {
                flushAndWaitTask.succeed(task);
            }
        }).name("generalGfxInitTask");

    adapterAndDeviceInit.precede(cmdListInitTask, PSOManagerInitTask, miscGfxInitTask, swapChainInitTask, dynamicDescHeapAllocatorInit);
    miscGfxInitTask.succeed(cmdListInitTask, defaultsAssetsPreInit);
    swapChainInitTask.succeed(cmdListInitTask);

    m_AllContexts.reserve(128);
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    m_GfxDevice.IncrementAndSignalFence();
    m_GfxDevice.WaitForFence();

    // finish all commands before shutting down
    m_GfxCommandManager.ConsumeAllCommandsST();

    // get swapchain out of full screen before exiting
    BOOL fs = false;
    DX12_CALL(m_SwapChain.Dev()->GetFullscreenState(&fs, NULL));
    if (fs)
        m_SwapChain.Dev()->SetFullscreenState(false, NULL);

    m_SceneDepthBuffer.Release();
    g_GfxPSOManager.ShutDown();
    g_ZPrePassRenderer.ShutDown();
    g_GfxTestRenderPass.ShutDown();
    g_GfxIMGUIRenderer.ShutDown();
    g_GfxDefaultAssets.ShutDown();

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

    ADD_TF_TASK(subFlow, g_ZPrePassRenderer.PopulateCommandList());
    ADD_TF_TASK(subFlow, g_GfxTestRenderPass.PopulateCommandList());
    ADD_TF_TASK(subFlow, g_GfxIMGUIRenderer.PopulateCommandList());
}

void GfxManager::ScheduleCommandListsExecution()
{
    bbeProfileFunction();

    // helper lambda
    auto QueueRenderPass = [&](GfxRendererBase* pass)
    {
        m_GfxDevice.GetCommandListsManager().QueueCommandListToExecute(pass->GetGfxContext()->GetCommandList(), pass->GetGfxContext()->GetCommandList().GetType());
    };

    // queue all render passes
    QueueRenderPass(&g_ZPrePassRenderer);
    QueueRenderPass(&g_GfxTestRenderPass);
    QueueRenderPass(&g_GfxIMGUIRenderer);

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
        clearBackBufferContext.ClearDepth(m_SceneDepthBuffer, 1.0f);
        m_GfxDevice.GetCommandListsManager().QueueCommandListToExecute(clearBackBufferContext.GetCommandList(), clearBackBufferContext.GetCommandList().GetType());

        m_GfxDevice.Flush();
    }
}

void GfxManager::EndFrame()
{
    bbeProfileFunction();

    m_GfxDevice.EndFrame();
    m_SwapChain.Present();
    m_GfxDevice.IncrementAndSignalFence();

    m_AllContexts.clear();
}

GfxContext& GfxManager::GenerateNewContext(D3D12_COMMAND_LIST_TYPE cmdListType, const std::string& name)
{
    bbeProfileFunction();

    //g_Log.info("*** GfxManager::GenerateNewContext: {}", name);

    GfxContext* newContext;
    {
        bbeAutoLock(m_ContextsLock);
        newContext = &m_AllContexts.emplace_back(GfxContext{});
    }
    newContext->Initialize(cmdListType, name);

    return *newContext;
}

void GfxManager::TransitionBackBufferForPresent()
{
    bbeProfileFunction();

    GfxContext& context = GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "TransitionBackBufferForPresent");
    SetD3DDebugName(context.GetCommandList().Dev(), "TransitionBackBufferForPresent");

    m_SwapChain.TransitionBackBufferForPresent(context);

    m_GfxDevice.GetCommandListsManager().QueueCommandListToExecute(context.GetCommandList(), context.GetCommandList().GetType());
}

void GfxManager::UpdateIMGUIPropertyGrid()
{
    if (!g_IMGUIManager.m_ShowGfxManagerWindow)
        return;

    bbeProfileFunction();

    D3D12MA::Allocator& allocator = m_GfxDevice.GetD3D12MemoryAllocator();

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
        const std::string statsString = MakeStrFromWStr(statsStringW);
        allocator.FreeStatsString(statsStringW);

        ScopedIMGUIWindow window{ "Detailed Gfx Stats" };
        ImGui::Text("%s", statsString.c_str());
    }
}

void GfxManager::InitSceneDepthBuffer()
{
    GfxContext& initContext = GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxManager::InitDepthBuffer");

    GfxTexture::InitParams depthBufferInitParams;
    depthBufferInitParams.m_Format = DXGI_FORMAT_D32_FLOAT;
    depthBufferInitParams.m_Width = g_CommandLineOptions.m_WindowWidth;
    depthBufferInitParams.m_Height = g_CommandLineOptions.m_WindowHeight;
    depthBufferInitParams.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    depthBufferInitParams.m_InitData = nullptr;
    depthBufferInitParams.m_ResourceName = "GfxManager Depth Buffer";
    depthBufferInitParams.m_InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    depthBufferInitParams.m_ViewType = GfxTexture::DSV;
    depthBufferInitParams.m_ClearValue.Format = depthBufferInitParams.m_Format;
    depthBufferInitParams.m_ClearValue.DepthStencil.Depth = 1.0f;
    depthBufferInitParams.m_ClearValue.DepthStencil.Stencil = 0;

    m_SceneDepthBuffer.Initialize(initContext, depthBufferInitParams);
}
