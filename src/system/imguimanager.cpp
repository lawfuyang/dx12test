#include "system/imguimanager.h"

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
    m_Timer.Tick();
    io.DeltaTime = (float)m_Timer.GetDeltaSeconds();

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

    // TODO: update all IMGUI windows here

    // Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    ImGui::ShowDemoWindow(&g_CommandLineOptions.m_ShowIMGUIDemoWindows);
    static bool show_another_window = false;

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    if (g_CommandLineOptions.m_ShowIMGUIDemoWindows)
    {
        static float f = 0.0f;
        static int counter = 0;
        static ImVec4 color{ 0.45f, 0.55f, 0.60f, 1.00f };

        // Create a window called "Hello, world!" and append into it.
        ImGui::Begin("Hello, world!");

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &g_CommandLineOptions.m_ShowIMGUIDemoWindows);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("color", (float*)&color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (g_CommandLineOptions.m_ShowIMGUIDemoWindows && show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    // This will back up the render data until the next frame.
    SaveDrawData();

    m_DrawDataIdx = 1 - m_DrawDataIdx;
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
    newDrawData.m_Pos = imguiDrawData->DisplayPos;
    newDrawData.m_Size = imguiDrawData->DisplaySize;

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
