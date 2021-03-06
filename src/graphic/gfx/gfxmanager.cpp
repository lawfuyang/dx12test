#include <graphic/gfx/gfxmanager.h>
#include <graphic/pch.h>

#include <system/imguimanager.h>

static bool gs_ShowGfxManagerIMGUIWindow = false;

extern GfxRendererBase* g_GfxForwardLightingPass;
extern GfxRendererBase* g_GfxIMGUIRenderer;
extern GfxRendererBase* g_GfxBodyGravityParticlesUpdate;
extern GfxRendererBase* g_GfxBodyGravityParticlesRender;
extern GfxTexture g_SceneDepthBuffer;

static GfxFence gs_FrameFence;

void InitializeGraphic(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    g_TasksExecutor.silent_async([] { g_GfxShaderManager.Initialize(); });
    g_TasksExecutor.silent_async([] { g_GfxDefaultVertexFormats.Initialize(); });
    g_TasksExecutor.silent_async([] { g_GfxLightsManager.Initialize(); });

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

    g_IMGUIManager.RegisterTopMenu("Graphic", "GfxManager", &gs_ShowGfxManagerIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUIPropertyGrid(); });

    tf::Task preInitGate = subFlow.emplace([this](tf::Subflow& sf) { PreInit(sf); });
    tf::Task mainInitGate = subFlow.emplace([this](tf::Subflow& sf) { MainInit(sf); }).succeed(preInitGate);
    subFlow.emplace([this] { PostInit(); }).succeed(mainInitGate);
}

void GfxManager::PreInit(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    g_GfxAdapter.Initialize();
    m_GfxDevice.Initialize();

    subFlow.emplace([] { gs_FrameFence.Initialize(); });
    subFlow.emplace([this] { m_GfxCommandManager.Initialize(); });
    subFlow.emplace([] { g_GfxPSOManager.Initialize(); });
    subFlow.emplace([] { g_GfxGPUDescriptorAllocator.Initialize(); });
    subFlow.emplace([] { g_GfxMemoryAllocator.Initialize(); });
    subFlow.emplace([](tf::Subflow& sf) { g_GfxCommandListsManager.Initialize(sf); });
}

void GfxManager::MainInit(tf::Subflow& subFlow)
{
    subFlow.emplace([this] { m_SwapChain.Initialize(); });
    subFlow.emplace([](tf::Subflow& sf) { g_GfxDefaultAssets.Initialize(sf); });

    for (GfxRendererBase* renderer : GfxRendererBase::ms_AllRenderers)
    {
        subFlow.emplace([renderer] { renderer->Initialize(); });
    }
}

void GfxManager::PostInit()
{
    bbeProfileFunction();

    g_GfxCommandListsManager.ExecutePendingCommandLists();

    // signal frame fence and stall cpu until all gpu work is done
    gs_FrameFence.IncrementAndSignal(g_GfxCommandListsManager.GetMainQueue().Dev());
    gs_FrameFence.WaitForSignalFromGPU();
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    gs_FrameFence.WaitForSignalFromGPU();

    m_GfxCommandManager.ConsumeAllCommandsST(true);

    // get swapchain out of full screen before exiting
    BOOL fs = false;
    DX12_CALL(m_SwapChain.Dev()->GetFullscreenState(&fs, NULL));
    if (fs)
        m_SwapChain.Dev()->SetFullscreenState(false, NULL);

    g_GfxPSOManager.ShutDown();
    g_GfxMemoryAllocator.ShutDown();
    g_GfxCommandListsManager.ShutDown();
    gs_FrameFence.Shutdown();
    m_SwapChain.Shutdown();
    m_GfxDevice.Shutdown();
    g_GfxAdapter.Shutdown();

    m_GfxCommandManager.ConsumeAllCommandsST(true);
}

void GfxManager::ScheduleGraphicTasks(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    // TODO: This will kill performance with D3D12_GPU_BASED_VALIDATION_STATE_TRACKING enabled!
    // Implement proper fence-based free-ing of gpu resources
    gs_FrameFence.WaitForSignalFromGPU();

    tf::Task beginFrameGate = subFlow.emplace([this] { BeginFrame(); });
    subFlow.emplace([this](tf::Subflow& sf) { m_GfxCommandManager.ConsumeAllCommandsMT(sf); }).succeed(beginFrameGate);
    tf::Task endFrameGate = subFlow.emplace([this] { EndFrame(); });

    struct ExecutionContext
    {
        GfxRendererBase* m_Renderer = nullptr;
        bool m_DoSkeletonScheduling = true;
    };

    // Functor for Renderers' cmd list population
    FixedSizeFlatMap<D3D12_COMMAND_LIST_TYPE, ExecutionContext, 3> lastExecutionContext;
    auto ScheduleRenderer = [&](GfxRendererBase* renderer, bool doSkeletonScheduling = true)
    {
        // reset each renderers' dependencies and event
        renderer->m_Event.Reset();
        renderer->m_Dependencies.clear();

        GfxContext& context = GenerateLightweightGfxContext();

        // Do nothing if no need to render
        if (!renderer->ShouldPopulateCommandList(context))
            return;

        // Rough skeleton dependency scheduling based off last renderer of same cmd list type
        D3D12_COMMAND_LIST_TYPE cmdListType = renderer->GetCommandListType(context);
        if (lastExecutionContext[cmdListType].m_Renderer && lastExecutionContext[cmdListType].m_DoSkeletonScheduling)
            renderer->m_Dependencies.push_back(lastExecutionContext[cmdListType].m_Renderer);
        lastExecutionContext[cmdListType] = { renderer, doSkeletonScheduling };

        subFlow.emplace([&, renderer, cmdListType]()
            {
                context.Initialize(cmdListType, renderer->GetName());
                
                renderer->AddDependencies();
                renderer->PopulateCommandList(context);
                
                // Execute cmd list only after prior dependent renderers are done
                for (GfxRendererBase* dependentRenderer : renderer->m_Dependencies)
                {
                    bbeProfile("Wait For Dependency");

                    // TODO: yield thread to other tasks, not stall. This will be terrible when we have alot more renderers
                    dependentRenderer->m_Event.Wait();
                }

                g_GfxCommandListsManager.ExecuteCommandListImmediate(&context.GetCommandList());

                // signal event after cmd list submission
                renderer->m_Event.Signal();

            }).succeed(beginFrameGate).precede(endFrameGate);
    };

    ScheduleRenderer(g_GfxBodyGravityParticlesUpdate, false);
    ScheduleRenderer(g_GfxForwardLightingPass);
    ScheduleRenderer(g_GfxBodyGravityParticlesRender);
    ScheduleRenderer(g_GfxIMGUIRenderer);
}

void GfxManager::BeginFrame()
{
    bbeProfileFunction();

    m_GfxDevice.CheckStatus();

    g_GfxMemoryAllocator.GarbageCollect();

    // TODO: Remove clearing of BackBuffer when we manage to fill every pixel on screen through various render passes
    GfxContext& context = GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "ClearBackBuffer & DepthBuffer");

    // If Swap Chain is used for the first time after init, it's in the [Common/Present] state
    // If not, it was transitioned into the [Common/Present] in "EndFrame" function
    context.BeginTrackResourceState(m_SwapChain.GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);

    // Clearing the back buffer will transition it to the "RenderTarget" state
    context.ClearRenderTargetView(m_SwapChain.GetCurrentBackBuffer(), bbeVector4{ 0.0f, 0.2f, 0.4f, 1.0f });
    context.ClearDepth(g_SceneDepthBuffer, 1.0f);
    g_GfxCommandListsManager.ExecuteCommandListImmediate(&context.GetCommandList());
}

void GfxManager::EndFrame()
{
    bbeProfileFunction();

    // Before presenting backbuffer, transition to to PRESENT state
    {
        GfxCommandList* cmdList = g_GfxCommandListsManager.GetMainQueue().AllocateCommandList("TransitionBackBufferTo_PRESENT");
        const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_SwapChain.GetCurrentBackBuffer().GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        cmdList->Dev()->ResourceBarrier(1, &barrier);
        g_GfxCommandListsManager.ExecuteCommandListImmediate(cmdList);
    }

    // Present back buffer
    m_SwapChain.Present();

    // signal frame fence after presenting
    gs_FrameFence.IncrementAndSignal(g_GfxCommandListsManager.GetMainQueue().Dev());

    // reset array of GfxContexts to prepare for next frame
    std::for_each(m_AllContexts.begin(), m_AllContexts.end(), [this](GfxContext* context) { m_ContextsPool.destroy(context); });
    m_AllContexts.clear();

    ++m_GraphicFrameNumber;
}

GfxContext& GfxManager::GenerateNewContext(D3D12_COMMAND_LIST_TYPE cmdListType, std::string_view name)
{
    GfxContext& newContext = GenerateLightweightGfxContext();
    newContext.Initialize(cmdListType, name);

    return newContext;
}

GfxContext& GfxManager::GenerateLightweightGfxContext()
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

        ScopedIMGUIWindow statsWindow{ "Detailed Gfx Stats" };
        ImGui::Text("%s", statsString);
    }
}
