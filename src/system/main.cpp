
class EngineWindowThread
{
public:
    static void Initialize()
    {
        ms_EngineWindowThread = std::thread{ [&]() { Run(); } };
    }

    static void Shutdown()
    {
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
            ::DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            break;
        }

        g_System.ProcessWindowsMessage(hWnd, message, wParam, lParam);
        return ::DefWindowProc(hWnd, message, wParam, lParam);
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
            g_Log.error("ApplicationWin : Failed to create window: {}", GetLastErrorAsString());
            assert(false);
        }

        const ::DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;
        ::RECT rect{ 0, 0, (LONG)g_CommandLineOptions.m_WindowWidth, (LONG)g_CommandLineOptions.m_WindowHeight };
        ::AdjustWindowRect(&rect, style, false);

        const int posX = (GetSystemMetrics(SM_CXSCREEN) - g_CommandLineOptions.m_WindowWidth) / 2;
        const int posY = (GetSystemMetrics(SM_CYSCREEN) - g_CommandLineOptions.m_WindowHeight) / 2;

        ::HWND engineWindowHandle = CreateWindow(wc.lpszClassName,
                                                 s_AppName,
                                                 style,
                                                 posX, posY,
                                                 rect.right - rect.left,
                                                 rect.bottom - rect.top,
                                                 0, 0, hInstance, NULL);

        if (engineWindowHandle == 0)
        {
            g_Log.error("ApplicationWin : Failed to create window: {}", GetLastErrorAsString());
            assert(false);
        }

        g_System.SetEngineWindowHandle(engineWindowHandle);

        ::ShowCursor(true);
        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));

        ::MSG msg;
        ::BOOL bRet;
        while ((bRet = ::GetMessage(&msg, NULL, 0, 0)) != 0)
        {
            if (bRet == -1)
            {
                g_Log.error("Can't get new message: {}", GetLastErrorAsString());
                assert(false);
            }
            else
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
        }
    }

    inline static std::thread ms_EngineWindowThread;
};

int APIENTRY WinMain(::HINSTANCE hInstance,
                     ::HINSTANCE hPrevInstance,
                     ::LPTSTR    lpCmdLine,
                     int         nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    // first, Init the logger
    Logger::GetInstance().Initialize("../bin/output.txt");

    g_Log.info("Commandline args: {}", lpCmdLine);
    g_CommandLineOptions.Parse();

    EngineWindowThread::Initialize();

    {
        Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT{ RO_INIT_MULTITHREADED };

        g_System.Initialize();
        g_System.Loop();
        g_System.Shutdown();

        EngineWindowThread::Shutdown();
    }

    // check for dxgi debug errors
    // todo: fix the leaks
#if 0
    if (g_CommandLineOptions.m_GfxDebugLayer.m_Enabled)
    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
#endif

    return 0;
}
