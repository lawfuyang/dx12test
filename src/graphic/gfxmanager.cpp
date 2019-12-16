#include "graphic/gfxmanager.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxadapter.h"
#include "graphic/gfxcontext.h"
#include "graphic/gfxdevice.h"
#include "graphic/guimanager.h"
#include "graphic/gfxrootsignature.h"
#include "graphic/gfxshadermanager.h"

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

    m_GfxDevice            = &GfxManagerSingletons::gs_GfxDevice;
    m_SwapChain            = &GfxManagerSingletons::gs_SwapChain;

    GfxAdapter::GetInstance().Initialize();
    m_GfxDevice->Initialize();

    tf::Taskflow tf;

    tf.emplace([&]() { m_SwapChain->Initialize(System::APP_WINDOW_WIDTH, System::APP_WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM); });
    tf.emplace([&]() { GUIManager::GetInstance().Initialize(); });
    tf.emplace([&]() { GfxRootSignatureManager::GetInstance().Initialize(); });
    tf.emplace([&]() { GfxPSOManager::GetInstance().Initialize(); });
    tf.emplace([&]() { GfxShaderManager::GetInstance().Initialize(); });

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

    const bool DeleteCacheFile = false;
    GfxPSOManager::GetInstance().ShutDown(DeleteCacheFile);
}

void GfxManager::ScheduleGraphicTasks(tf::Taskflow& tf)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    tf::Task beginFrameTask = tf.emplace([&]() { BeginFrame(); });

    tf::Task allRenderTasks = tf.emplace([&](tf::Subflow& sf)
        {
            for (const std::unique_ptr<GfxRenderPass>& renderPass : GfxManagerSingletons::gs_RenderPasses)
            {
                GfxContext& context = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
                SetD3DDebugName(context.GetCommandList()->Dev(), renderPass->GetName());

                tf::Task newRenderTask = sf.emplace([&]()
                    {
                        renderPass->Render(context);
                    });
                newRenderTask.name(renderPass->GetName());
            }
        }
    );
    allRenderTasks.succeed(beginFrameTask);

    tf::Task endFrameTask = tf.emplace([&]() { EndFrame(); });
    endFrameTask.succeed(allRenderTasks);
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
