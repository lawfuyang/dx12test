#include "graphic/gfxmanager.h"

#include "extern/D3D12MemoryAllocator/src/D3D12MemAlloc.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxadapter.h"
#include "graphic/gfxcontext.h"
#include "graphic/gfxdevice.h"
#include "graphic/guimanager.h"
#include "graphic/gfxrootsignature.h"
#include "graphic/gfxshadermanager.h"
#include "graphic/gfxvertexformat.h"
#include "graphic/gfxtexturesandbuffers.h"

#include "graphic/renderpasses/gfxtestrenderpass.h"

namespace GfxManagerSingletons
{
    static GfxDevice gs_GfxDevice;
    static GfxSwapChain gs_SwapChain;

    static GfxTestRenderPass gs_TestRenderPass;
}

void GfxManager::Initialize(tf::Taskflow& tf)
{
    bbeProfileFunction();

    m_GfxDevice = &GfxManagerSingletons::gs_GfxDevice;
    m_SwapChain = &GfxManagerSingletons::gs_SwapChain;
    m_FrameParamsCB = new GfxConstantBuffer;

    tf::Task adapterInitTask        = tf.emplace([&]() { GfxAdapter::GetInstance().Initialize(); });
    tf::Task deviceInitTask         = tf.emplace([&]() { m_GfxDevice->Initialize(); });
    tf::Task cmdListInitTask        = tf.emplace([&]() { m_GfxDevice->GetCommandListsManager().Initialize(); });
    tf::Task swapChainInitTask      = tf.emplace([&]() { m_SwapChain->Initialize(); });
    tf::Task GUIManagerInitTask     = tf.emplace([&]() { GUIManager::GetInstance().Initialize(); });
    tf::Task rootSigManagerInitTask = tf.emplace([&]() { GfxRootSignatureManager::GetInstance().Initialize(); });
    tf::Task PSOManagerInitTask     = tf.emplace([&]() { GfxPSOManager::GetInstance().Initialize(); });
    tf::Task shaderManagerInitTask  = tf.emplace([&]() { GfxShaderManager::GetInstance().Initialize(); });
    tf::Task vertexFormatsInitTask  = tf.emplace([&]() { GfxDefaultVertexFormats::Initialize(); });
    tf::Task frameParamsCBInitTask  = tf.emplace([&]() { m_FrameParamsCB->Initialize(m_GfxDevice->GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "FrameParams"), sizeof(AutoGenerated::FrameParams)); });

    tf::Task renderPassesInitTask = tf.emplace([&]()
        {
            bbeProfile("Render Passes Construction");

            // TODO: Multi-thread this task
            GfxContext& initContext = m_GfxDevice->GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "Render Passes Init");

            GfxManagerSingletons::gs_TestRenderPass.Initialize(initContext);

            // execute all cmd lists and wait for everything to complete 
            m_GfxDevice->EndFrame();
            m_GfxDevice->WaitForPreviousFrame();
        });

    deviceInitTask.succeed(adapterInitTask);
    deviceInitTask.precede(cmdListInitTask, rootSigManagerInitTask, PSOManagerInitTask);
    frameParamsCBInitTask.succeed(cmdListInitTask, rootSigManagerInitTask);
    swapChainInitTask.succeed(deviceInitTask, cmdListInitTask);
    renderPassesInitTask.succeed(swapChainInitTask, rootSigManagerInitTask, PSOManagerInitTask);
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    GUIManager::GetInstance().ShutDown();

    // get swapchain out of full screen before exiting
    BOOL fs = false;
    DX12_CALL(m_SwapChain->Dev()->GetFullscreenState(&fs, NULL));
    if (fs)
        m_SwapChain->Dev()->SetFullscreenState(false, NULL);

    GfxPSOManager::GetInstance().ShutDown();

    GfxManagerSingletons::gs_TestRenderPass.ShutDown();
    
    m_FrameParamsCB->Release();
    delete m_FrameParamsCB;

    // we must complete the previous GPU frame before exiting the app
    m_GfxDevice->WaitForPreviousFrame();
    m_GfxDevice->ShutDown();
}

void GfxManager::ScheduleGraphicTasks(tf::Taskflow& tf)
{
    bbeProfileFunction();

    // consume all gfx commmands multi-threaded
    std::vector<std::function<void()>> gfxCommandsToConsume;
    m_GfxCommands.consume_all([&](const std::function<void()>& cmd) { gfxCommandsToConsume.push_back(cmd); });
    tf::Task gfxCommandsConsumptionTasks = tf.emplace([gfxCommandsToConsume](tf::Subflow& subFlow)
        {
            subFlow.parallel_for(gfxCommandsToConsume.begin(), gfxCommandsToConsume.end(), [](const std::function<void()>& cmd) { bbeProfile("GfxCommand_MT"); cmd(); });
        });

    tf::Task beginFrameTask = tf.emplace([&]() { BeginFrame(); });
    tf::Task transitionBackBufferTask = tf.emplace([&]() { TransitionBackBufferForPresent(); });
    tf::Task endFrameTask = tf.emplace([&]() { EndFrame(); });
    tf::Task renderPassesRenderTasks = tf.emplace([&](tf::Subflow& sf) { ScheduleRenderPasses(sf); });

    gfxCommandsConsumptionTasks.precede(beginFrameTask);
    beginFrameTask.precede(renderPassesRenderTasks);
    transitionBackBufferTask.succeed(renderPassesRenderTasks);
    endFrameTask.succeed(transitionBackBufferTask);
}

void GfxManager::ScheduleRenderPasses(tf::Subflow& sf)
{
    bbeProfileFunction();

    tf::Task testRenderPass = sf.emplace([&]() { GfxManagerSingletons::gs_TestRenderPass.Render(m_GfxDevice->GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "TestRenderPass")); });
}

void GfxManager::BeginFrame()
{
    bbeProfileFunction();

    // check for DXGI_ERRORs on all GfxDevices
    m_GfxDevice->CheckStatus();

    UpdateFrameParamsCB();
}

void GfxManager::EndFrame()
{
    bbeProfileFunction();

    m_GfxDevice->EndFrame();
    m_SwapChain->Present();

    // TODO: refactor this. use fences to sync
    m_GfxDevice->WaitForPreviousFrame();
}

void GfxManager::DumpGfxMemory()
{
    WCHAR* statsString = NULL;

    D3D12MA::Allocator* allocator = m_GfxDevice->GetD3D12MemoryAllocator();

    allocator->BuildStatsString(&statsString, allocator->GetD3D12Options().ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2);

    const std::string dumpFilePath = StringFormat("..\\bin\\GfxMemoryDump_%s.json", GetTimeStamp().c_str());
    const bool IsReadMode = false;
    CFileWrapper outFile{ dumpFilePath, IsReadMode };
    fprintf(outFile, "%ls", statsString);

    m_GfxDevice->GetD3D12MemoryAllocator()->FreeStatsString(statsString);
}

void GfxManager::TransitionBackBufferForPresent()
{
    bbeProfileFunction();

    GfxContext& context = m_GfxDevice->GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "TransitionBackBufferForPresent");
    SetD3DDebugName(context.GetCommandList().Dev(), "TransitionBackBufferForPresent");

    m_SwapChain->TransitionBackBufferForPresent(context);
}

void GfxManager::UpdateFrameParamsCB()
{
    AutoGenerated::FrameParams params;
    params.m_FrameNumber = System::GetSystemFrameNumber();
    params.m_CurrentFrameTime = System::GetCappedFrameTimeMs();
    params.m_PreviousFrameTime = System::GetCappedPrevFrameTimeMs();

    m_FrameParamsCB->Update(&params);
}
