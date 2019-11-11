#include "graphic/guimanager.h"

extern ::HWND g_EngineWindowHandle;

GUIManager::GUIManager()
{

}

GUIManager::~GUIManager()
{

}

void GUIManager::Initialize()
{
    bbeProfileFunction();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
}

void GUIManager::ShutDown()
{
    bbeProfileFunction();

    ImGui::DestroyContext();
}

void GUIManager::HandleWindowsInput(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

}

void GUIManager::BeginFrame()
{
    //ImGui_ImplWin32_NewFrame();
    //ImGui::NewFrame();
}

void GUIManager::EndFrameAndRenderGUI()
{
    //ImGui::Render();
}
