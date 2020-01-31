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

#include "graphic/renderpasses/gfxtestrenderpass.h"

namespace GfxManagerSingletons
{
    static GfxDevice gs_GfxDevice;
    static GfxSwapChain gs_SwapChain;

    static GfxRenderPass* gs_TestRenderPass = nullptr;
}

void GfxManager::Initialize(tf::Taskflow& tf)
{
    bbeProfileFunction();

    m_GfxDevice = &GfxManagerSingletons::gs_GfxDevice;
    m_SwapChain = &GfxManagerSingletons::gs_SwapChain;

    tf::Task adapterInitTask           = tf.emplace([&]() { GfxAdapter::GetInstance().Initialize(); });
    tf::Task deviceInitTask            = tf.emplace([&]() { m_GfxDevice->Initialize(); });
    tf::Task cmdListInitTask           = tf.emplace([&]() { m_GfxDevice->GetCommandListsManager().Initialize(); });
    tf::Task descHeapManagerInitTask   = tf.emplace([&]() { m_GfxDevice->GetDescriptorHeapManager().Initialize(); });
    tf::Task swapChainInitTask         = tf.emplace([&]() { m_SwapChain->Initialize(); });
    tf::Task GUIManagerInitTask        = tf.emplace([&]() { GUIManager::GetInstance().Initialize(); });
    tf::Task rootSigManagerInitTask    = tf.emplace([&]() { GfxRootSignatureManager::GetInstance().Initialize(); });
    tf::Task PSOManagerInitTask        = tf.emplace([&]() { GfxPSOManager::GetInstance().Initialize(); });
    tf::Task shaderManagerInitTask     = tf.emplace([&]() { GfxShaderManager::GetInstance().Initialize(); });
    tf::Task vertexInputLayoutInitTask = tf.emplace([&]() { GfxVertexInputLayoutManager::GetInstance().Initialize(); });

    tf::Task renderPassesInitTask = tf.emplace([&]()
        {
            bbeProfile("Render Passes Construction");

            GfxContext& initContext = m_GfxDevice->GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT);

            GfxManagerSingletons::gs_TestRenderPass = new GfxTestRenderPass(initContext);

            // execute all cmd lists and wait for everything to complete 
            m_GfxDevice->EndFrame();
            m_GfxDevice->WaitForPreviousFrame();
        });

    deviceInitTask.succeed(adapterInitTask);
    deviceInitTask.precede(cmdListInitTask, descHeapManagerInitTask, rootSigManagerInitTask, PSOManagerInitTask);
    swapChainInitTask.succeed(deviceInitTask, cmdListInitTask, descHeapManagerInitTask);
    renderPassesInitTask.succeed(deviceInitTask, swapChainInitTask, rootSigManagerInitTask, PSOManagerInitTask);
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

    // we must complete the previous GPU frame before exiting the app
    m_GfxDevice->WaitForPreviousFrame();

    GfxPSOManager::GetInstance().ShutDown();

    delete(GfxManagerSingletons::gs_TestRenderPass);

    m_GfxDevice->ShutDown();
}

void GfxManager::ScheduleGraphicTasks(tf::Taskflow& tf)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    // consume all gfx commmands multi-threaded
    std::vector<std::function<void()>> gfxCommandsToConsume;
    m_GfxCommands.consume_all([&](const std::function<void()>& cmd) { gfxCommandsToConsume.push_back(cmd); });
    tf::Task gfxCommandsConsumptionTasks = tf.emplace([gfxCommandsToConsume](tf::Subflow subFlow)
        {
            subFlow.parallel_for(gfxCommandsToConsume.begin(), gfxCommandsToConsume.end(), [](const std::function<void()>& cmd) { bbeProfile("GfxCommand_MT"); cmd(); });
        });

    tf::Task beginFrameTask = tf.emplace([&]() { BeginFrame(); });
    tf::Task transitionBackBufferTask = tf.emplace([&]() { TransitionBackBufferForPresent(); });
    tf::Task endFrameTask = tf.emplace([&]() { EndFrame(); });

    // TODO: Implement proper automatic dynamic scheduling for render passes
    tf::Task renderPassesRenderTasks = tf.emplace([&]()
        {
            GfxManagerSingletons::gs_TestRenderPass->Render(gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT));
        });

    gfxCommandsConsumptionTasks.precede(beginFrameTask);
    beginFrameTask.precede(renderPassesRenderTasks);
    transitionBackBufferTask.succeed(renderPassesRenderTasks);
    endFrameTask.succeed(transitionBackBufferTask);
}

void GfxManager::BeginFrame()
{
    bbeProfileFunction();

    // check for DXGI_ERRORs on all GfxDevices
    m_GfxDevice->CheckStatus();
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

    GfxContext& context = m_GfxDevice->GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
    SetD3DDebugName(context.GetCommandList().Dev(), "TransitionBackBufferForPresent");

    context.ClearRenderTargetView(m_SwapChain->GetCurrentBackBuffer(), XMFLOAT4{ 0.0f, 0.2f, 0.4f, 1.0f });
    m_SwapChain->TransitionBackBufferForPresent(context);
}
