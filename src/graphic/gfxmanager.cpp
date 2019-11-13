#include "graphic/gfxmanager.h"

#include "graphic/gfxadapter.h"
#include "graphic/gfxdevice.h"
#include "graphic/gfxrenderer.h"
#include "graphic/guimanager.h"

void GfxManager::Initialize()
{
    bbeProfileFunction();

    GfxAdapter::GetInstance().Initialize();

    m_MainDevice.InitializeForMainDevice();

    m_SwapChain.Initialize(System::APP_WINDOW_WIDTH, System::APP_WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM);

    GfxRenderer::GetInstance().Initialize();

    GUIManager::GetInstance().Initialize();
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    GUIManager::GetInstance().ShutDown();
}

void GfxManager::BeginFrame()
{
    GUIManager::GetInstance().BeginFrame();
}

void GfxManager::EndFrame()
{
    GUIManager::GetInstance().EndFrameAndRenderGUI();
}
