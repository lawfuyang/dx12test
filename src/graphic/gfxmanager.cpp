#include "graphic/gfxmanager.h"

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
    static std::vector<std::unique_ptr<GfxRenderPass>> gs_RenderPasses;
}

void GfxManager::Initialize()
{
    bbeProfileFunction();

    m_GfxDevice = &GfxManagerSingletons::gs_GfxDevice;
    m_SwapChain = &GfxManagerSingletons::gs_SwapChain;

    tf::Taskflow tf;

    tf::Task adapterInitTask           = tf.emplace([&]() { GfxAdapter::GetInstance().Initialize(); });
    tf::Task deviceInitTask            = tf.emplace([&]() { m_GfxDevice->Initialize(); });
    tf::Task cmdListInitTask           = tf.emplace([&]() { m_GfxDevice->GetCommandListsManager().Initialize(); });
    tf::Task descHeapManagerInitTask   = tf.emplace([&]() { m_GfxDevice->GetDescriptorHeapManager().Initialize(); });
    tf::Task swapChainInitTask         = tf.emplace([&]() { m_SwapChain->Initialize(System::APP_WINDOW_WIDTH, System::APP_WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM); });
    tf::Task GUIManagerInitTask        = tf.emplace([&]() { GUIManager::GetInstance().Initialize(); });
    tf::Task rootSigManagerInitTask    = tf.emplace([&]() { GfxRootSignatureManager::GetInstance().Initialize(); });
    tf::Task PSOManagerInitTask        = tf.emplace([&]() { GfxPSOManager::GetInstance().Initialize(); });
    tf::Task ShaderManagerInitTask     = tf.emplace([&]() { GfxShaderManager::GetInstance().Initialize(); });
    tf::Task VertexInputLayoutInitTask = tf.emplace([&]() { GfxVertexInputLayoutManager::GetInstance().Initialize(); });

    deviceInitTask.succeed(adapterInitTask);
    deviceInitTask.precede({ cmdListInitTask, descHeapManagerInitTask });
    swapChainInitTask.succeed({ deviceInitTask, cmdListInitTask, descHeapManagerInitTask });
    rootSigManagerInitTask.succeed(deviceInitTask);
    PSOManagerInitTask.succeed(deviceInitTask);

    System::GetInstance().GetTasksExecutor().run(tf).wait();

    GfxManagerSingletons::gs_RenderPasses.reserve(4);
    GfxManagerSingletons::gs_RenderPasses.push_back(std::make_unique<GfxTestRenderPass>());
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    GUIManager::GetInstance().ShutDown();

    // we must complete the previous GPU frame before exiting the app
    m_GfxDevice->WaitForPreviousFrame();

    const bool DeleteCacheFile = true; // TODO: find out why the fuck ID3D12Device1::CreatePipelineLibrary is crashing when there's an existing cache file
    GfxPSOManager::GetInstance().ShutDown(DeleteCacheFile);
}

void GfxManager::ScheduleGraphicTasks(tf::Taskflow& tf)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    tf::Task beginFrameTask = tf.emplace([&]() { BeginFrame(); });

    std::vector<tf::Task> allRenderTasks;
    allRenderTasks.reserve(4);
    for (const std::unique_ptr<GfxRenderPass>& renderPass : GfxManagerSingletons::gs_RenderPasses)
    {
        GfxContext& context = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
        SetD3DDebugName(context.GetCommandList().Dev(), renderPass->GetName());
        allRenderTasks.push_back(tf.emplace([&]() { renderPass->Render(context); }));
    }
    beginFrameTask.precede(allRenderTasks);

    tf::Task transitionBackBufferTask = tf.emplace([&]() { TransitionBackBufferForPresent(); });
    transitionBackBufferTask.succeed(allRenderTasks);

    tf::Task endFrameTask = tf.emplace([&]() { EndFrame(); });
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

void GfxManager::TransitionBackBufferForPresent()
{
    bbeProfileFunction();

    GfxContext& context = m_GfxDevice->GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
    SetD3DDebugName(context.GetCommandList().Dev(), "TransitionBackBufferForPresent");

    context.ClearRenderTargetView(m_SwapChain->GetCurrentBackBuffer(), XMFLOAT4{ 0.0f, 0.2f, 0.4f, 1.0f });
    m_SwapChain->TransitionBackBufferForPresent(context);
}
