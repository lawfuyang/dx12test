#include "graphic/precomp.h"
#include "graphic/gfxmanager.h"

#include "graphic/gfxrenderer.h"
#include "graphic/guimanager.h"

GfxManager::GfxManager()
{

}
GfxManager::~GfxManager()
{

}

void GfxManager::Initialize()
{
    bbeProfileFunction();

    GUIManager::GetInstance().Initialize();

    m_MainDevice = std::make_unique<GfxDevice>();
    m_MainDevice->InitializeForMainDevice();

    GfxRenderer::GetInstance().Initialize();
}

void GfxManager::ShutDown()
{
    bbeProfileFunction();

    GfxRenderer::GetInstance().ShutDown();

    m_MainDevice->ShutDown();
    m_MainDevice.release();

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
