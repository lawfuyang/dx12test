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
            GfxManagerSingletons::gs_RenderPasses.push_back(std::make_unique<GfxTestRenderPass>());
        });

    deviceInitTask.succeed(adapterInitTask);
    deviceInitTask.precede(cmdListInitTask, descHeapManagerInitTask);
    deviceInitTask.precede(rootSigManagerInitTask, PSOManagerInitTask);
    swapChainInitTask.succeed(deviceInitTask, cmdListInitTask, descHeapManagerInitTask);
    renderPassesInitTask.succeed(deviceInitTask, swapChainInitTask, rootSigManagerInitTask, PSOManagerInitTask);
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    GUIManager::GetInstance().ShutDown();

    // we must complete the previous GPU frame before exiting the app
    m_GfxDevice->WaitForPreviousFrame();

    GfxPSOManager::GetInstance().ShutDown();
}

void GfxManager::ScheduleGraphicTasks(tf::Taskflow& tf)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    tf::Task beginFrameTask = tf.emplace([&]() { BeginFrame(); });
    tf::Task transitionBackBufferTask = tf.emplace([&]() { TransitionBackBufferForPresent(); });

    auto [S, T] = tf.parallel_for(GfxManagerSingletons::gs_RenderPasses.begin(), GfxManagerSingletons::gs_RenderPasses.end(), [&](const std::unique_ptr<GfxRenderPass>& renderPass)
        {
            GfxContext& context = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
            SetD3DDebugName(context.GetCommandList().Dev(), renderPass->GetName());

            renderPass->Render(context);
        });

    beginFrameTask.precede(S);
    transitionBackBufferTask.succeed(T);

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
