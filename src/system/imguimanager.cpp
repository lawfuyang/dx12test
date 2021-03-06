#include <system/imguimanager.h>

#define g_IMGUIFileDialog igfd::ImGuiFileDialog::Instance()

void IMGUIManager::Initialize()
{
    bbeProfileFunction();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;         // Enable Keyboard Controls
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;    // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendPlatformName = "imgui_impl_win32";
    io.BackendRendererName = "imgui_impl_dx12";
    io.ImeWindowHandle = g_System.GetEngineWindowHandle();

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_Tab]         = VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow]   = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow]  = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow]     = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow]   = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp]      = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown]    = VK_NEXT;
    io.KeyMap[ImGuiKey_Home]        = VK_HOME;
    io.KeyMap[ImGuiKey_End]         = VK_END;
    io.KeyMap[ImGuiKey_Insert]      = VK_INSERT;
    io.KeyMap[ImGuiKey_Delete]      = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace]   = VK_BACK;
    io.KeyMap[ImGuiKey_Space]       = VK_SPACE;
    io.KeyMap[ImGuiKey_Enter]       = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape]      = VK_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
    io.KeyMap[ImGuiKey_A]           = 'A';
    io.KeyMap[ImGuiKey_C]           = 'C';
    io.KeyMap[ImGuiKey_V]           = 'V';
    io.KeyMap[ImGuiKey_X]           = 'X';
    io.KeyMap[ImGuiKey_Y]           = 'Y';
    io.KeyMap[ImGuiKey_Z]           = 'Z';

    ImGui::StyleColorsDark();

    RegisterTopMenu("Others", "Show IMGUI Demo Window", &m_ShowDemoWindow);
}

void IMGUIManager::ShutDown()
{
    bbeProfileFunction();

    ImGui::DestroyContext();
}

void IMGUIManager::ProcessWindowsMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() == NULL)
        return;

    ImGuiIO& io = ImGui::GetIO();
    switch (msg)
    {
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        {
            int button = 0;
            if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
            if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
            if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
            if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
            if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
                ::SetCapture(g_System.GetEngineWindowHandle());
            io.MouseDown[button] = true;
            break;
        }

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            int button = 0;
            if (msg == WM_LBUTTONUP) { button = 0; }
            if (msg == WM_RBUTTONUP) { button = 1; }
            if (msg == WM_MBUTTONUP) { button = 2; }
            if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
            io.MouseDown[button] = false;
            if (!ImGui::IsAnyMouseDown() && ::GetCapture() == g_System.GetEngineWindowHandle())
                ::ReleaseCapture();
            break;
        }

        case WM_MOUSEWHEEL:
            io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
            break;

        case WM_MOUSEHWHEEL:
            io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wParam < 256)
                io.KeysDown[wParam] = 1;
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (wParam < 256)
                io.KeysDown[wParam] = 0;
            break;

        case WM_CHAR:
            // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
            io.AddInputCharacter((unsigned int)wParam);
            break;

        //case WM_DEVICECHANGE:
        //{
        //    const uint32_t DBT_DEVNODES_CHANGED = 0x0007;

        //    if ((uint32_t)wParam == DBT_DEVNODES_CHANGED)
        //        g_WantUpdateHasGamepad = true;
        //    break;
        //}
    }
}

void IMGUIManager::Update()
{
    bbeProfileFunction();

    ImGuiIO& io = ImGui::GetIO();
    assert(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    RECT rect;
    ::GetClientRect(g_System.GetEngineWindowHandle(), &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    // Setup time step
    io.DeltaTime = (float)Timer::TicksToSeconds(m_Timer.Tick());

    // Read keyboard modifiers inputs
    io.KeyCtrl = g_Keyboard.IsKeyPressed(Keyboard::KEY_CTRL);
    io.KeyShift = g_Keyboard.IsKeyPressed(Keyboard::KEY_SHIFT);
    io.KeyAlt = g_Keyboard.IsKeyPressed(Keyboard::KEY_ALT);
    io.KeySuper = false;

    // Set mouse position
    io.MousePos = ImVec2{ -FLT_MAX, -FLT_MAX };
    POINT pos;
    if (HWND active_window = ::GetForegroundWindow())
    {
        if (active_window == g_System.GetEngineWindowHandle() || ::IsChild(active_window, g_System.GetEngineWindowHandle()))
        {
            if (::GetCursorPos(&pos) && ::ScreenToClient(g_System.GetEngineWindowHandle(), &pos))
            {
                io.MousePos = ImVec2((float)pos.x, (float)pos.y);
            }
        }
    }

    ImGui::NewFrame();

    // Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (m_ShowDemoWindow)
    {
        ImGui::ShowDemoWindow();
    }

    auto MenuItemBoolToggle = [](const char* label, bool& b) { if (ImGui::MenuItem(label, nullptr, b)) { b = !b; } };

    if (ImGui::BeginMainMenuBar())
    {
        for (auto& menus : m_TopMenusCBs)
        {
            const char* menuCategory = menus.first.c_str();
            if (ImGui::BeginMenu(menuCategory))
            {
                std::vector<std::pair<std::string, bool*>>& menuButtons = menus.second;
                for (auto& button : menuButtons)
                {
                    const char* buttonName = button.first.c_str();

                    if (bool* menuToggle = button.second)
                    {
                        MenuItemBoolToggle(buttonName, *menuToggle);

                        if (*menuToggle && m_TriggerCBs.count(menuToggle))
                        {
                            m_TriggerCBs[menuToggle]();
                            *menuToggle = false;
                        }
                    }
                }

                ImGui::EndMenu();
            }
        }

        ImGui::Text("\t\tFrame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
        ImGui::SameLine();
        ImGui::Text("\tFPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::EndMainMenuBar();
    }

    // swap and update the rest
    InplaceArray<std::function<void()>, 128> windowUpdateCBs;
    {
        bbeAutoLock(m_WindowCBsLock);
        std::copy(m_UpdateCBs.begin(), m_UpdateCBs.end(), std::back_inserter(windowUpdateCBs));
    }

    for (const std::function<void()>& cb : windowUpdateCBs)
    {
        bbeProfile("Update IMGUI Window");
        cb();
    }

    if (m_ActiveFileDialog.second)
    {
        // display
        if (g_IMGUIFileDialog->FileDialog(m_ActiveFileDialog.first))
        {
            // action if OK
            if (g_IMGUIFileDialog->IsOk)
            {
                m_ActiveFileDialog.second(g_IMGUIFileDialog->GetFilePathName());
            }

            // close
            g_IMGUIFileDialog->CloseDialog(m_ActiveFileDialog.first);

            m_ActiveFileDialog.first.clear();
            m_ActiveFileDialog.second = nullptr;
        }
    }

    // This will back up the render data until the next frame.
    SaveDrawData();

    m_DrawDataIdx = 1 - m_DrawDataIdx;
}

void IMGUIManager::RegisterWindowUpdateCB(const std::function<void()>& cb)
{
    bbeAutoLock(m_WindowCBsLock);
    m_UpdateCBs.push_back(cb);
}

void IMGUIManager::RegisterGeneralButtonCB(const std::function<void()>& cb, bool* triggerBool)
{
    assert(triggerBool);

    bbeAutoLock(m_WindowCBsLock);
    m_TriggerCBs[triggerBool] = cb;
}

void IMGUIManager::RegisterTopMenu(const std::string& mainCategory, const std::string& buttonName, bool* windowToggle)
{
    bbeAutoLock(m_WindowCBsLock);
    m_TopMenusCBs[mainCategory].push_back(std::make_pair(buttonName, windowToggle));
}

void IMGUIManager::RegisterFileDialog(const std::string& vName, const char* vFilters, const FileDialogResultFinalizer& finalizer)
{
    // only support 1 active file dialog
    if (!m_ActiveFileDialog.first.empty())
        return;

    g_IMGUIFileDialog->OpenDialog(vName, vName.c_str(), vFilters, "..\\bin\\assets\\");

    m_ActiveFileDialog.first = vName;
    m_ActiveFileDialog.second = finalizer;
}

void IMGUIManager::SaveDrawData()
{
    bbeProfileFunction();

    ImGui::Render();

    ImDrawData* imguiDrawData = ImGui::GetDrawData();
    assert(imguiDrawData);

    IMGUIDrawData newDrawData;
    newDrawData.m_VtxCount = imguiDrawData->TotalVtxCount;
    newDrawData.m_IdxCount = imguiDrawData->TotalIdxCount;
    newDrawData.m_Pos = bbeVector2{ imguiDrawData->DisplayPos.x, imguiDrawData->DisplayPos.y };
    newDrawData.m_Size = bbeVector2{ imguiDrawData->DisplaySize.x, imguiDrawData->DisplaySize.y };

    newDrawData.m_DrawList.resize(imguiDrawData->CmdListsCount);
    for (int n = 0; n < imguiDrawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = imguiDrawData->CmdLists[n];

        // Copy VB and IB
        newDrawData.m_DrawList[n].m_VB.resize(cmd_list->VtxBuffer.size());
        memcpy(newDrawData.m_DrawList[n].m_VB.data(), &cmd_list->VtxBuffer[0], cmd_list->VtxBuffer.size() * sizeof(ImDrawVert));
        newDrawData.m_DrawList[n].m_IB.resize(cmd_list->IdxBuffer.size());
        memcpy(newDrawData.m_DrawList[n].m_IB.data(), &cmd_list->IdxBuffer[0], cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx));

        // Copy cmdlist params
        newDrawData.m_DrawList[n].m_DrawCmd.resize(cmd_list->CmdBuffer.size());
        memcpy(newDrawData.m_DrawList[n].m_DrawCmd.data(), &cmd_list->CmdBuffer[0], cmd_list->CmdBuffer.size() * sizeof(ImDrawCmd));
    }

    m_DrawData[1 - m_DrawDataIdx] = std::move(newDrawData);
}

ScopedIMGUIWindow::ScopedIMGUIWindow(const char* windowName)
{
    bool* pOpen = nullptr;
    ImGui::Begin(windowName, pOpen, ImGuiWindowFlags_AlwaysAutoResize);
}

ScopedIMGUIWindow::~ScopedIMGUIWindow()
{
    ImGui::End();
}
