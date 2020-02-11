#include "graphic/gfxmanager.h"
#include "graphic/guimanager.h"
#include "system.h"

void System::ProcessWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CLOSE)
    {
        m_Exit = true;
    }

    GUIManager::GetInstance().HandleWindowsInput(hWnd, message, wParam, lParam);
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

            tf::Taskflow tf;
            GfxManager::GetInstance().ScheduleGraphicTasks(tf);
            m_Executor.run(tf).wait();

            // make sure I/O ticks happen last
            g_Keyboard.Tick();
            g_Mouse.Tick(ms_RealFrameTimeMs);
            ++ms_SystemFrameNumber;
        }

        g_Profiler.OnFlip();
    } while (!m_Exit);

    g_Log.info("Exiting main loop");
}

void System::RunKeyboardCommands()
{
    g_Profiler.DumpProfilerBlocks(Keyboard::IsKeyPressed(Keyboard::KEY_P));

    if (Keyboard::IsKeyPressed(Keyboard::KEY_J))
    {
        GfxManager::GetInstance().DumpGfxMemory();
    }
}

void System::Initialize()
{
    g_Profiler.Initialize();

    bbeConditionalProfile(g_CommandLineOptions.m_ProfileInit, "System::Initialize");

    tf::Taskflow tf;

    GfxManager::GetInstance().Initialize(tf);
    // TODO: init other System/Engine/Physics systems here in parallel

    m_Executor.run(tf).wait();

    g_Profiler.DumpProfilerBlocks(g_CommandLineOptions.m_ProfileInit, true, true);
}

void System::Shutdown()
{
    {
        bbeConditionalProfile(g_CommandLineOptions.m_ProfileShutdown, "System::Shutdown");

        GfxManager::GetInstance().ShutDown();
    }

    g_Profiler.DumpProfilerBlocks(g_CommandLineOptions.m_ProfileShutdown, true, true);
    g_Profiler.ShutDown();
}

FrameRateController::FrameRateController()
{
    m_FrameEndTime = std::chrono::high_resolution_clock::now() + std::chrono::microseconds{ 1000000 / System::FPS_LIMIT };
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

    System::ms_RealFrameTimeMs   = realFrameTimeMs;
    System::ms_FPS               = fps;
    System::ms_CappedFrameTimeMs = cappedFrameTimeMs;
    System::ms_CappedFPS         = cappedFPS;
}

#define CommandLineCB(argStr, func) { std::hash<std::string_view>{}(argStr), func }
static const std::unordered_map<std::size_t, std::function<void()>> g_CommandLineOptionsCBs =
{
    CommandLineCB("pixcapture", [&]() { g_CommandLineOptions.m_PIXCapture = true; }),
    CommandLineCB("profileinit", [&]() { g_CommandLineOptions.m_ProfileInit = true; }),
    CommandLineCB("profileshutdown", [&]() { g_CommandLineOptions.m_ProfileShutdown = true; })
};
#undef CommandLineCB

void CommandLineOptions::ParseCmdLine(char* cmdLine)
{
    g_Log.info("cmd line args: {}", cmdLine);

    const char* delimiters = " ";
    char* token = std::strtok(cmdLine, delimiters);
    while (token)
    {
        const std::size_t tokenHash = std::hash<std::string>{}(token);
        try
        {
            g_CommandLineOptionsCBs.at(tokenHash)();
        }
        catch (const std::exception & ex)
        {
            g_Log.error("CommandLineOptions error for '{}': {}", token, ex.what());
        }

        token = std::strtok(nullptr, delimiters);
    }
}
