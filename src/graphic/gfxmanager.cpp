#include "graphic/gfxmanager.h"

#include "graphic/gfxadapter.h"
#include "graphic/gfxcontext.h"
#include "graphic/gfxdevice.h"
#include "graphic/guimanager.h"
#include "graphic/gfxrootsignature.h"

#include "graphic/renderpasses/gfxtestrenderpass.h"

namespace GfxManagerSingletons
{
    static GfxDevice    gs_GfxDevice;
    static GfxSwapChain gs_SwapChain;
    static GUIManager   gs_GUIManager;

    static std::vector<std::unique_ptr<GfxRenderPass>> gs_RenderPasses;
}

void GfxManager::Initialize()
{
    bbeProfileFunction();

    m_GfxDevice = &GfxManagerSingletons::gs_GfxDevice;
    m_SwapChain = &GfxManagerSingletons::gs_SwapChain;
    m_GUIManager = &GfxManagerSingletons::gs_GUIManager;

    GfxAdapter::GetInstance().Initialize();
    m_GfxDevice->Initialize();
    m_SwapChain->Initialize(System::APP_WINDOW_WIDTH, System::APP_WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_GUIManager->Initialize();

    GfxManagerSingletons::gs_RenderPasses.push_back(std::make_unique<GfxTestRenderPass>());
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    m_GUIManager->ShutDown();
    m_GfxDevice->WaitForPreviousFrame();
}

void GfxManager::ScheduleGraphicTasks(tf::Subflow& sf)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    tf::Task beginFrameTask = sf.emplace([&]() { BeginFrame(); });
    
    std::vector<tf::Task> allRenderTasks;
    allRenderTasks.reserve(GfxManagerSingletons::gs_RenderPasses.size());

    for (const std::unique_ptr<GfxRenderPass>& renderPass : GfxManagerSingletons::gs_RenderPasses)
    {
        GfxContext& context = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
        tf::Task newRenderTask = sf.emplace([&]()
            {
                renderPass->Render(context);
            });
        newRenderTask.name(renderPass->GetName());
        allRenderTasks.push_back(newRenderTask);
    }
    beginFrameTask.precede(allRenderTasks);

    tf::Task endFrameTask = sf.emplace([&]() { EndFrame(); });
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
    m_GfxDevice->WaitForPreviousFrame();
}
