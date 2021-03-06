#include <system/system.h>

#include <system/imguimanager.h>

extern void InitializeGraphic(tf::Subflow& subFlow);
extern void UpdateGraphic(tf::Subflow& subFlow);
extern void ShutdownGraphic();
extern void InitializeApplicationLayer(tf::Subflow& subFlow);
extern void UpdateApplicationLayer(tf::Subflow& subFlow);
extern void ShutdownApplicationLayer();

void System::ProcessWindowsMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto Get_X_LPARAM = [&](::LPARAM lParam) { return ((int32_t)(int64_t)LOWORD(lParam)); };
    auto Get_Y_LPARAM = [&](::LPARAM lParam) { return ((int32_t)(int64_t)HIWORD(lParam)); };

    switch (message)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        m_Exit = true;
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
        g_Mouse.UpdateButton(g_Mouse.Left, true);
        break;

    case WM_LBUTTONUP:
        g_Mouse.UpdateButton(g_Mouse.Left, false);
        break;

    case WM_RBUTTONDOWN:
        g_Mouse.UpdateButton(g_Mouse.Right, true);
        break;

    case WM_RBUTTONUP:
        g_Mouse.UpdateButton(g_Mouse.Right, false);
        break;

    case WM_MOUSEWHEEL:
        g_Mouse.ProcessMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        break;
    }

    g_IMGUIManager.ProcessWindowsMessage(hWnd, message, wParam, lParam);
}

static void BusyWaitUntilFPSLimit(Timer& timer)
{
    bbeProfileFunction();

    const uint64_t ticksToFPSLimit = Timer::MilliSecondsToTicks(1000.0 / g_CommandLineOptions.m_FPSLimit);

    // TODO: Find a way to NOT spin wait to fps cap
#if 0
    const uint64_t ticksSoFar = timer.GetElapsedTicks();

    // beyond FPS limit... return
    if (ticksSoFar >= ticksToFPSLimit)
        return;

    // ticks left
    LARGE_INTEGER liDueTime{};
    liDueTime.QuadPart = -(LONGLONG)(ticksToFPSLimit - ticksSoFar);

    static ::HANDLE FPSTimer = ::CreateWaitableTimer(nullptr, true, nullptr);
    bool result = ::SetWaitableTimer(FPSTimer, &liDueTime, 0, NULL, NULL, 0);
    assert(result);

    // wait until fps cap
    WaitForSingleObject(FPSTimer, INFINITE);
#endif

    while (timer.GetElapsedTicks() < ticksToFPSLimit) { std::this_thread::yield(); }
}

void System::Loop()
{
    g_Log.info("Entering main loop");

    do
    {
        Timer frameTimer;

        RunKeyboardCommands();

        // run System commnads first
        tf::Taskflow tf;
        tf.emplace([this](tf::Subflow& subFlow) { m_SystemCommandManager.ConsumeAllCommandsMT(subFlow); });
        m_Executor.run(tf).wait();
        tf.clear();

        tf.emplace([](tf::Subflow& subFlow) { UpdateGraphic(subFlow); });
        tf.emplace([](tf::Subflow& subFlow) { UpdateApplicationLayer(subFlow); });
        tf.emplace([]() { g_IMGUIManager.Update(); });
        m_Executor.run(tf).wait();

        // make sure I/O ticks happen last
        g_Keyboard.Tick();
        g_Mouse.Tick();
        ++m_SystemFrameNumber;

        g_Profiler.OnFlip();

        BusyWaitUntilFPSLimit(frameTimer);

        const double elapsedMS = frameTimer.GetElapsedMilliSeconds();
        g_System.m_FrameTimeMs = elapsedMS;
        g_System.m_FPS = 1000.0 / elapsedMS;
    } while (!m_Exit);

    g_Log.info("Exiting main loop");
}

void System::RunKeyboardCommands()
{
    g_Profiler.DumpProfilerBlocks(g_Keyboard.IsKeyPressed(Keyboard::KEY_P));
}

void System::BGAsyncThreadLoop()
{
    MicroProfileOnThreadCreate("BG Async Thread");

    do
    {
        const bool ConsumeAllCmdsRecursive = true;
        m_BGAsyncCommandManager.ConsumeAllCommandsST(ConsumeAllCmdsRecursive);

        ::Sleep(1);
    }while (!m_BGAsyncThreadExit);
}

void System::Initialize()
{
    g_Profiler.Initialize();

    {
        bbeProfileFunction();

        m_BGAsyncCommandManager.Initialize();
        m_BGAsyncThread = std::thread([&]() { BGAsyncThreadLoop(); });

        m_SystemCommandManager.Initialize();

        m_Executor.silent_async([] { g_IMGUIManager.Initialize(); });

        tf::Taskflow tf;
        tf.emplace([](tf::Subflow& subFlow) { InitializeGraphic(subFlow); });
        tf.emplace([](tf::Subflow& subFlow) { InitializeApplicationLayer(subFlow); });

        m_Executor.run(tf);
        m_Executor.wait_for_all();
    }

    g_Profiler.DumpProfilerBlocks(g_CommandLineOptions.m_ProfileInit, true);
}

void System::Shutdown()
{
    {
        bbeConditionalProfile(g_CommandLineOptions.m_ProfileShutdown, "System::Shutdown");

        const bool ConsumeAllCmdsRecursive = true;
        m_SystemCommandManager.ConsumeAllCommandsST(ConsumeAllCmdsRecursive);

        g_IMGUIManager.ShutDown();
        ShutdownApplicationLayer();
        ShutdownGraphic();

        {
            bbeProfile("Wait BG Async Thread Done");

            m_BGAsyncThreadExit = true;
            m_BGAsyncThread.join();
        }
    }

    g_Profiler.DumpProfilerBlocks(g_CommandLineOptions.m_ProfileShutdown, true);
    g_Profiler.ShutDown();
}

void CommandLineOptions::Parse()
{
    ArgumentParser parser("Argument Parser");

    parser.add_argument("--fpslimit", "fpslimit");
    parser.add_argument("--pixcapture", "pixcapture");
    parser.add_argument("--profileinit", "profileinit");
    parser.add_argument("--profileshutdown", "profileshutdown");
    parser.add_argument("--resolution", "resolution");
    parser.add_argument("--gfxdebuglayer", "gfxdebuglayer");

    try
    {
        parser.parse(__argc, (const char**)__argv);
    }
    catch (const std::exception & ex)
    {
        g_Log.error("CommandLineOptions::Parse error: {}", ex.what());
    }

    m_PIXCapture      = parser.exists("pixcapture");
    m_ProfileInit     = parser.exists("profileinit");
    m_ProfileShutdown = parser.exists("profileshutdown");

    if (parser.exists("fpslimit"))
    {
        m_FPSLimit = parser.get<uint32_t>("fpslimit");
    }

    if (parser.exists("resolution"))
    {
        const std::vector<uint32_t> resolution = parser.getv<uint32_t>("resolution");
        m_WindowWidth = resolution[0];
        m_WindowHeight = resolution[1];
    }

    if (parser.exists("gfxdebuglayer"))
    {
        m_GfxDebugLayer.m_Enabled = true;

        const std::vector<std::string> args = parser.getv<std::string>("gfxdebuglayer");
        for (const std::string& str : args)
        {
            if (str.find("breakonwarnings") != std::string::npos) { m_GfxDebugLayer.m_BreakOnWarnings = true; }
            else if (str.find("breakonerrors") != std::string::npos) { m_GfxDebugLayer.m_BreakOnErrors = true; }
            else if (str.find("gpuvalidation") != std::string::npos) { m_GfxDebugLayer.m_EnableGPUValidation = true; }
            else if (str.find("gpuresourcevalidation") != std::string::npos) { m_GfxDebugLayer.m_EnableGPUResourceValidation = true; }
            else if (str.find("resourcestatetracking") != std::string::npos) { m_GfxDebugLayer.m_EnableConservativeResourceStateTracking = true; }
            else if (str.find("behaviorchangingaids") != std::string::npos) { m_GfxDebugLayer.m_BehaviorChangingAids = true; }
        }
    }

    const std::vector<std::string> unusedArgs = parser.getv<std::string>("");
    for (const std::string& str : unusedArgs)
    {
        g_Log.warn("Unused commandline arg: {}", str);
    }
}
