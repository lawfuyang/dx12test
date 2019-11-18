#include "graphic/gfxmanager.h"

#include "graphic/gfxadapter.h"
#include "graphic/gfxdevice.h"
#include "graphic/guimanager.h"

namespace GfxManagerSingletons
{
    static GfxDevice    gs_MainDevice;
    static GfxSwapChain gs_SwapChain;
    static GUIManager   gs_GUIManager;
}

void GfxManager::Initialize()
{
    bbeProfileFunction();

    m_MainDevice = &GfxManagerSingletons::gs_MainDevice;
    m_SwapChain = &GfxManagerSingletons::gs_SwapChain;
    m_GUIManager = &GfxManagerSingletons::gs_GUIManager;

    GfxAdapter::GetInstance().Initialize();

    m_MainDevice->InitializeForMainDevice();

    m_SwapChain->Initialize(System::APP_WINDOW_WIDTH, System::APP_WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM);

    m_GUIManager->Initialize();
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    m_GUIManager->ShutDown();
    m_MainDevice->WaitForPreviousFrame();
}

void GfxManager::ScheduleGraphicTasks(tf::Subflow& sf)
{
    bbeProfileFunction();

    tf::Task beginFrameTask = sf.emplace([this]() { BeginFrame(); });
    tf::Task mainRenderTask = sf.emplace([this]() { Render(); });
    tf::Task endFrameTask = sf.emplace([this]() { EndFrame(); });

    beginFrameTask.precede(mainRenderTask);
    mainRenderTask.precede(endFrameTask);
}

void GfxManager::BeginFrame()
{
    bbeProfileFunction();

    // check for DXGI_ERRORs on all GfxDevices
    m_MainDevice->CheckStatus();

    m_MainDevice->BeginFrame();
    m_GUIManager->BeginFrame();
}

void GfxManager::Render()
{
    bbeProfileFunction();

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_MainDevice->ClearRenderTargetView(m_SwapChain->GetCurrentBackBuffer(), clearColor);
}

void GfxManager::EndFrame()
{
    bbeProfileFunction();
    m_MainDevice->EndFrame();
    m_GUIManager->EndFrameAndRenderGUI();

    m_SwapChain->Present();
}
