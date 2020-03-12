#pragma once

class EngineWindowThread
{
public:
    static void Initialize()
    {
        ms_EngineWindowThread = std::thread{ [&]() { Run(); } };
    }

    static void Shutdown()
    {
        if (g_System.GetEngineWindowHandle())
            DestroyWindow(g_System.GetEngineWindowHandle());

        ms_EngineWindowThread.join();
    }

private:
    static ::LRESULT CALLBACK WndProc(::HWND hWnd, ::UINT message, ::WPARAM wParam, ::LPARAM lParam)
    {
        auto Get_X_LPARAM = [&](::LPARAM lParam) { return ((int32_t)(int64_t)LOWORD(lParam)); };
        auto Get_Y_LPARAM = [&](::LPARAM lParam) { return ((int32_t)(int64_t)HIWORD(lParam)); };

        switch (message)
        {
        case WM_CLOSE:
            ms_Exit = true;
            break;
        case WM_MOUSEMOVE:
            RECT rect;
            GetClientRect(g_System.GetEngineWindowHandle(), &rect);
            g_Mouse.ProcessMouseMove((uint32_t)wParam, Get_X_LPARAM(lParam) - rect.left, Get_Y_LPARAM(lParam) - rect.top);
            // NO BREAK HERE!
        case WM_KEYUP:
            g_Keyboard.ProcessKeyUp((uint32_t)wParam);
            break;
        case WM_KEYDOWN:
            g_Keyboard.ProcessKeyDown((uint32_t)wParam);
            break;
        case WM_LBUTTONDOWN:
            g_Mouse.UpdateButton(Mouse::Left, true);
            break;
        case WM_LBUTTONUP:
            g_Mouse.UpdateButton(Mouse::Left, false);
            break;
        case WM_RBUTTONDOWN:
            g_Mouse.UpdateButton(Mouse::Right, true);
            break;
        case WM_RBUTTONUP:
            g_Mouse.UpdateButton(Mouse::Right, false);
            break;
        case WM_MOUSEWHEEL:
            g_Mouse.ProcessMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            break;
        default:
            break;
        }

        g_System.ProcessWindowsMessage(hWnd, message, wParam, lParam);
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    static uint32_t GetTaskBarHeight()
    {
        ::RECT rect = {};
        ::HWND taskBar = ::FindWindowA("Shell_traywnd", NULL);
        if (taskBar && ::GetWindowRect(taskBar, &rect)) 
        {
            return rect.bottom - rect.top;
        }
        return 0;
    }

    static void Run()
    {
        static const char* s_AppName = "DX12 Test";

        ::HINSTANCE hInstance = 0;

        ::WNDCLASS wc = { 0 };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.hbrBackground = (::HBRUSH)(COLOR_BACKGROUND);
        wc.lpszClassName = s_AppName;

        if (FAILED(RegisterClass(&wc)))
        {
            g_Log.error("ApplicationWin : Failed to create window: {}", GetLastErrorAsString().c_str());
            assert(false);
        }

        const ::DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;
        ::RECT rect{ 0, 0, (LONG)g_CommandLineOptions.m_WindowWidth, (LONG)g_CommandLineOptions.m_WindowHeight };
        ::AdjustWindowRect(&rect, style, false);

        ::RECT desktopRect = {};
        ::GetWindowRect(::GetDesktopWindow(), &desktopRect);

        const uint32_t nativeWidth = desktopRect.right;
        const uint32_t nativeHeight = desktopRect.bottom - GetTaskBarHeight();
        const uint32_t windowTopLeftX = (nativeWidth - g_CommandLineOptions.m_WindowWidth) / 2;
        const uint32_t windowTopLeftY = (nativeHeight - g_CommandLineOptions.m_WindowHeight) / 2;

        ::HWND engineWindowHandle = CreateWindow(wc.lpszClassName,
                                                 s_AppName,
                                                 style,
                                                 windowTopLeftX, windowTopLeftY,
                                                 rect.right - rect.left,
                                                 rect.bottom - rect.top,
                                                 0, 0, hInstance, NULL);

        if (engineWindowHandle == 0)
        {
            g_Log.error("ApplicationWin : Failed to create window: {}", GetLastErrorAsString().c_str());
            assert(false);
        }

        g_System.SetEngineWindowHandle(engineWindowHandle);

        ::ShowCursor(true);
        ::SetCursor(LoadCursor(NULL, IDC_ARROW));

        ::MSG msg;
        ::BOOL bRet;
        while ((bRet = GetMessageW(&msg, NULL, 0, 0)) != 0)
        {
            if (bRet == -1)
            {
                g_Log.error("Can't get new message: {}", GetLastErrorAsString().c_str());
                _exit(0);
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (ms_Exit)
            {
                break;
            }
        }
    }

    inline static bool ms_Exit = false;
    inline static std::thread ms_EngineWindowThread;
};

int APIENTRY WinMain(::HINSTANCE hInstance,
                     ::HINSTANCE hPrevInstance,
                     ::LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    // first, Init the logger
    Logger::GetInstance().Initialize("../bin/output.txt");

    g_Log.info("Commandline args: {}", lpCmdLine);
    g_CommandLineOptions.Parse();

    EngineWindowThread::Initialize();

    g_System.Initialize();
    g_System.Loop();
    g_System.Shutdown();

    EngineWindowThread::Shutdown();
}
