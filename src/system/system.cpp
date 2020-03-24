#include "extern/argparse/argparse.h"

#include "system.h"

extern void InitializeGraphic();
extern void UpdateGraphic();
extern void ShutdownGraphic();
extern void InitializeApplicationLayer();
extern void UpdateApplicationLayer();
extern void ShutdownApplicationLayer();

void System::ProcessWindowsMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CLOSE)
    {
        m_Exit = true;
    }
}

void System::Loop()
{
    g_Log.info("Entering main loop");

    do
    {
        bbeProfile("Frame");

        // limited FPS scope
        {
            const FrameRateController frcInstance;

            RunKeyboardCommands();

            UpdateGraphic();
            UpdateApplicationLayer();

            // make sure I/O ticks happen last
            g_Keyboard.Tick();
            g_Mouse.Tick(m_RealFrameTimeMs);
            ++m_SystemFrameNumber;
        }

        g_Profiler.OnFlip(); // This shit costs ~0.4ms on the main CPU thread SingleThreaded!
    } while (!m_Exit);

    g_Log.info("Exiting main loop");
}

void System::RunKeyboardCommands()
{
    g_Profiler.DumpProfilerBlocks(Keyboard::IsKeyPressed(Keyboard::KEY_P));
}

void System::Initialize()
{
    g_Profiler.Initialize();

    bbeConditionalProfile(g_CommandLineOptions.m_ProfileInit, "System::Initialize");

    InitializeGraphic();
    InitializeApplicationLayer();

    g_Profiler.DumpProfilerBlocks(g_CommandLineOptions.m_ProfileInit, true);
}

void System::Shutdown()
{
    {
        bbeConditionalProfile(g_CommandLineOptions.m_ProfileShutdown, "System::Shutdown");

        ShutdownApplicationLayer();
        ShutdownGraphic();
    }

    g_Profiler.DumpProfilerBlocks(g_CommandLineOptions.m_ProfileShutdown, true);
    g_Profiler.ShutDown();
}

FrameRateController::FrameRateController()
{
    static const uint32_t s_FPSLimit = 60;

    g_System.m_CappedPrevFrameMs = g_System.m_RealFrameTimeMs;

    m_FrameEndTime = std::chrono::high_resolution_clock::now() + std::chrono::microseconds{ 1000000 / s_FPSLimit };
    m_200FPSFrameEndTime = std::chrono::high_resolution_clock::now() + std::chrono::microseconds{ 1000000 / 200 };
}

FrameRateController::~FrameRateController()
{
    const float realFrameTimeMs = (float)m_StopWatch.ElapsedUS() / 1000;
    const float fps = 1000.0f / realFrameTimeMs;

    bbeProfile("Idle");

    while (std::chrono::high_resolution_clock::now() < m_200FPSFrameEndTime) { std::this_thread::yield(); } // busy wait until hard cap of 200 fps
    while (std::chrono::high_resolution_clock::now() < m_FrameEndTime) { std::this_thread::yield(); } // busy wait until exactly fps limit

    const float cappedFrameTimeMs = (float)m_StopWatch.ElapsedUS() / 1000;
    const float cappedFPS = 1000.0f / cappedFrameTimeMs;

    g_System.m_RealFrameTimeMs   = realFrameTimeMs;
    g_System.m_FPS               = fps;
    g_System.m_CappedFrameTimeMs = cappedFrameTimeMs;
    g_System.m_CappedFPS         = cappedFPS;
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
