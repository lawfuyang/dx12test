#include <graphic/gfxmanager.h>

#include <graphic/dx12utils.h>
#include <graphic/gfxadapter.h>
#include <graphic/gfxcontext.h>
#include <graphic/gfxrootsignature.h>
#include <graphic/gfxshadermanager.h>
#include <graphic/gfxvertexformat.h>
#include <graphic/gfxdefaultassets.h>
#include <graphic/gfxtexturesandbuffers.h>

#include <graphic/renderpasses/gfxtestrenderpass.h>
#include <graphic/renderpasses/gfximguirenderer.h>

#include <tmp/shaders/FrameParams.h>

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

    if (Keyboard::IsKeyPressed(Keyboard::KEY_J))
    {
        g_GfxManager.DumpGfxMemory();
    }
}

void GfxManager::Initialize(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    // independent tasks
    ADD_SF_TASK(subFlow, g_GfxShaderManager.Initialize(sf));
    ADD_TF_TASK(subFlow, g_GfxDefaultVertexFormats.Initialize());
    
    // tasks with dependencies
    tf::Task cmdListInitTask            = ADD_TF_TASK(subFlow, m_GfxDevice.GetCommandListsManager().Initialize());
    tf::Task swapChainInitTask          = ADD_TF_TASK(subFlow, m_SwapChain.Initialize());
    tf::Task rootSigManagerInitTask     = ADD_TF_TASK(subFlow, g_GfxRootSignatureManager.Initialize());
    tf::Task PSOManagerInitTask         = ADD_TF_TASK(subFlow, g_GfxPSOManager.Initialize());
    tf::Task defaultsAssetsPreInit      = ADD_SF_TASK(subFlow, g_GfxDefaultAssets.PreInitialize(sf));

    tf::Task adapterAndDeviceInit = subFlow.emplace([&](tf::Subflow& sf)
        {
            g_GfxAdapter.Initialize();
            m_GfxDevice.Initialize();
        }).name("adapterAndDeviceInit");

    tf::Task miscGfxInitTask = subFlow.emplace([&](tf::Subflow& subFlow)
        {
            bbeProfile("General Gfx Init");

            InplaceArray<tf::Task, 4> allTasks;

            allTasks.push_back(ADD_SF_TASK(subFlow, g_GfxDefaultAssets.Initialize(sf)));
            allTasks.push_back(ADD_TF_TASK(subFlow, m_FrameParamsCB.Initialize<AutoGenerated::FrameParams>()));
            allTasks.push_back(ADD_TF_TASK(subFlow, g_GfxTestRenderPass.Initialize()));
            allTasks.push_back(ADD_TF_TASK(subFlow, g_GfxIMGUIRenderer.Initialize()));

            tf::Task flushAndWaitTask = ADD_TF_TASK(subFlow, m_GfxDevice.Flush(true /* andWait */));
            for (tf::Task task : allTasks)
            {
                flushAndWaitTask.succeed(task);
            }
        }).name("generalGfxInitTask");

    adapterAndDeviceInit.precede(cmdListInitTask, rootSigManagerInitTask, PSOManagerInitTask, miscGfxInitTask, swapChainInitTask);
    miscGfxInitTask.succeed(cmdListInitTask, rootSigManagerInitTask, defaultsAssetsPreInit);
    miscGfxInitTask.succeed(cmdListInitTask, rootSigManagerInitTask);
    swapChainInitTask.succeed(cmdListInitTask);
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    // get swapchain out of full screen before exiting
    BOOL fs = false;
    DX12_CALL(m_SwapChain.Dev()->GetFullscreenState(&fs, NULL));
    if (fs)
        m_SwapChain.Dev()->SetFullscreenState(false, NULL);

    g_GfxPSOManager.ShutDown();
    g_GfxTestRenderPass.ShutDown();
    g_GfxIMGUIRenderer.ShutDown();
    m_FrameParamsCB.Release();
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
    tf::Task gfxCommandsConsumptionTasks = ADD_SF_TASK(subFlow, m_GfxCommandManager.ConsumeCommandsMT(sf));
    tf::Task beginFrameTask              = ADD_TF_TASK(subFlow, BeginFrame());
    tf::Task transitionBackBufferTask    = ADD_TF_TASK(subFlow, TransitionBackBufferForPresent());
    tf::Task endFrameTask                = ADD_TF_TASK(subFlow, EndFrame());
    tf::Task renderPassesRenderTasks     = ADD_SF_TASK(subFlow, ScheduleRenderPasses(sf));

    gfxCommandsConsumptionTasks.precede(beginFrameTask);
    beginFrameTask.precede(renderPassesRenderTasks);
    transitionBackBufferTask.succeed(renderPassesRenderTasks);
    endFrameTask.succeed(transitionBackBufferTask);
}

void GfxManager::ScheduleRenderPasses(tf::Subflow& subFlow)
{
    bbeProfileFunction();

    tf::Task testRenderPass  = ADD_TF_TASK(subFlow, g_GfxTestRenderPass.Render(m_GfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "TestRenderPass")));
    tf::Task IMGUIRenderPass = ADD_TF_TASK(subFlow, g_GfxIMGUIRenderer.Render(m_GfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "IMGUIRenderPass")));

    IMGUIRenderPass.succeed(testRenderPass);
}

void GfxManager::BeginFrame()
{
    bbeProfileFunction();

    // wait for GPU to be done with previous frame
    m_GfxDevice.WaitForFence();

    // check for DXGI_ERRORs on all GfxDevices
    m_GfxDevice.CheckStatus();

    UpdateFrameParamsCB();

    GfxContext& context = m_GfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "ClearBackBuffer");
    SetD3DDebugName(context.GetCommandList().Dev(), "ClearBackBuffer");

    // TODO: Remove this when we manage to fill every pixel on screen through various render passes
    context.ClearRenderTargetView(g_GfxManager.GetSwapChain().GetCurrentBackBuffer(), bbeVector4{ 0.0f, 0.2f, 0.4f, 1.0f });

    m_GfxDevice.Flush();
}

void GfxManager::EndFrame()
{
    bbeProfileFunction();

    m_GfxDevice.Flush();
    m_SwapChain.Present();
    m_GfxDevice.IncrementAndSignalFence();
}

void GfxManager::DumpGfxMemory()
{
    WCHAR* statsString = NULL;

    D3D12MA::Allocator& allocator = m_GfxDevice.GetD3D12MemoryAllocator();

    allocator.BuildStatsString(&statsString, allocator.GetD3D12Options().ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2);

    const std::string dumpFilePath = StringFormat("..\\bin\\GfxMemoryDump_%s.json", GetTimeStamp().c_str());
    const bool IsReadMode = false;
    CFileWrapper outFile{ dumpFilePath, IsReadMode };
    fprintf(outFile, "%ls", statsString);

    allocator.FreeStatsString(statsString);
}

void GfxManager::TransitionBackBufferForPresent()
{
    bbeProfileFunction();

    GfxContext& context = m_GfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "TransitionBackBufferForPresent");
    SetD3DDebugName(context.GetCommandList().Dev(), "TransitionBackBufferForPresent");

    m_SwapChain.TransitionBackBufferForPresent(context);

    m_GfxDevice.Flush();
}

void GfxManager::UpdateFrameParamsCB()
{
    AutoGenerated::FrameParams params;
    params.m_FrameNumber = g_System.GetSystemFrameNumber();
    params.m_CurrentFrameTime = (float)g_System.GetFrameTimeMs();

    m_FrameParamsCB.Update(&params);
}
