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

    const uint32_t FPSLimit = 200;

    // busy wait until hard cap of 200 FPS
    const uint64_t FPSFrameEndTime = Timer::MilliSecondsToTicks(1000.0 / FPSLimit);

    while (timer.GetElapsedTicks() < FPSFrameEndTime)
    {
        std::this_thread::yield();
    }
}

void System::Loop()
{
    g_Log.info("Entering main loop");

    do
    {
        m_FrameTimer.Reset();

        RunKeyboardCommands();

        tf::Taskflow tf;

        tf::Task systemConsumptionTasks = ADD_SF_TASK(tf, m_SystemCommandManager.ConsumeAllCommandsMT(sf));

        InplaceArray<tf::Task, 4> allSystemTasks;
        allSystemTasks.push_back(ADD_SF_TASK(tf, UpdateGraphic(sf)));
        allSystemTasks.push_back(ADD_SF_TASK(tf, UpdateApplicationLayer(sf)));
        allSystemTasks.push_back(ADD_TF_TASK(tf, g_IMGUIManager.Update()));

        for (tf::Task task : allSystemTasks)
        {
            systemConsumptionTasks.precede(task);
        }

        m_Executor.run(tf).wait();

        // make sure I/O ticks happen last
        g_Keyboard.Tick();
        g_Mouse.Tick();
        ++m_SystemFrameNumber;

        g_Profiler.OnFlip();

        BusyWaitUntilFPSLimit(m_FrameTimer);

        const double elapsedMS = m_FrameTimer.GetElapsedMicroSeconds();
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

    while (!m_BGAsyncThreadExit)
    {
        bool hasCommands = true;
        do
        {
            hasCommands = m_BGAsyncCommandManager.ConsumeOneCommand();
        } while (hasCommands);

        ::Sleep(1);
    }
}

void System::Initialize()
{
    g_Profiler.Initialize();

    {
        bbeProfileFunction();

        m_BGAsyncCommandManager.Initialize();
        m_BGAsyncThread = std::thread([&]() { BGAsyncThreadLoop(); });

        m_SystemCommandManager.Initialize();

        tf::Taskflow tf;

        ADD_TF_TASK(tf, g_IMGUIManager.Initialize());
        ADD_SF_TASK(tf, InitializeGraphic(sf));
        ADD_SF_TASK(tf, InitializeApplicationLayer(sf));

        m_Executor.run(tf).wait();
    }

    g_Profiler.DumpProfilerBlocks(g_CommandLineOptions.m_ProfileInit, true);
}

void System::Shutdown()
{
    {
        bbeConditionalProfile(g_CommandLineOptions.m_ProfileShutdown, "System::Shutdown");

        m_SystemCommandManager.ConsumeAllCommandsST();

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

    parser.add_argument("--pixcapture", "pixcapture");
    parser.add_argument("--profileinit", "profileinit");
    parser.add_argument("--profileshutdown", "profileshutdown");
    parser.add_argument("--resolution", "resolution");
    parser.add_argument("--gfxmemallocalwayscommitedmemory", "gfxmemallocalwayscommitedmemory");
    parser.add_argument("--enablegfxdebuglayer", "enablegfxdebuglayer");

    try
    {
        parser.parse(__argc, (const char**)__argv);
    }
    catch (const std::exception & ex)
    {
        g_Log.error("CommandLineOptions::Parse error: {}", ex.what());
    }

    m_PIXCapture                      = parser.exists("pixcapture");
    m_ProfileInit                     = parser.exists("profileinit");
    m_ProfileShutdown                 = parser.exists("profileshutdown");
    m_GfxMemAllocAlwaysCommitedMemory = parser.exists("gfxmemallocalwayscommitedmemory");
    m_EnableGfxDebugLayer             = parser.exists("enablegfxdebuglayer");

    if (parser.exists("resolution"))
    {
        const std::vector<uint32_t> resolution = parser.getv<uint32_t>("resolution");
        m_WindowWidth = resolution[0];
        m_WindowHeight = resolution[1];
    }

    const std::vector<std::string> unusedArgs = parser.getv<std::string>("");
    for (const std::string& str : unusedArgs)
    {
        g_Log.warn("Unused commandline arg: {}", str);
    }
}
