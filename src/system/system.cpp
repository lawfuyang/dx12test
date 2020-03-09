#include "system.h"

extern void InitializeGraphic(tf::Taskflow&);
extern void UpdateGraphic(tf::Taskflow&);
extern void ShutdownGraphic();
extern void InitializeApplicationLayer(tf::Taskflow&);
extern void UpdateApplicationLayer(tf::Taskflow&);
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

            tf::Taskflow tf;
            UpdateGraphic(tf);
            UpdateApplicationLayer(tf);
            m_Executor.run(tf).wait();

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
    InitializeThreadIDs();

    g_Profiler.Initialize();

    bbeConditionalProfile(g_CommandLineOptions.m_ProfileInit, "System::Initialize");

    tf::Taskflow tf;

    InitializeGraphic(tf);
    InitializeApplicationLayer(tf);

    m_Executor.run(tf).wait();

    g_Profiler.DumpProfilerBlocks(g_CommandLineOptions.m_ProfileInit, true);
}

// nice helper to get thread ID in uint32_t instead of some stupid custom type
static uint32_t GetSTDThreadID()
{
    return *reinterpret_cast<uint32_t*>(&std::this_thread::get_id());
}

uint32_t System::GetCurrentThreadID() const
{
    return m_STDThreadIDToIndexMap.at(GetSTDThreadID());
}

void System::InitializeThreadIDs()
{
    // dummy array because 'taskflow' lib's parallel_for is quite limited...
    bool* dummyArr = (bool*)alloca(sizeof(bool) * m_Executor.num_workers());

    // 'taskflow' lib does not allow me to readily access its internal thread IDs easily... need to do this lame step
    tf::Taskflow tf;
    tf.parallel_for(dummyArr, dummyArr + m_Executor.num_workers(), [&](bool)
        {
            const uint32_t workerThreadIdx = m_Executor.this_worker_id();
            const uint32_t STDThreadID = GetSTDThreadID();

            static AdaptiveLock s_Lock;
            bbeAutoLock(s_Lock);
            m_STDThreadIDToIndexMap[STDThreadID] = workerThreadIdx + 1; //  +1, because main thread will be 0
        });
    m_Executor.run(tf).wait();

    // for main thread
    m_STDThreadIDToIndexMap[GetSTDThreadID()] = 0;
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

static const std::unordered_map<std::string, std::function<void()>> g_CommandLineOptionsCBs =
{
    {"pixcapture", [&]() { g_CommandLineOptions.m_PIXCapture = true; }},
    {"profileinit", [&]() { g_CommandLineOptions.m_ProfileInit = true; }},
    {"profileshutdown", [&]() { g_CommandLineOptions.m_ProfileShutdown = true; }}
};

void CommandLineOptions::ParseCmdLine(char* cmdLine)
{
    g_Log.info("cmd line args: {}", cmdLine);

    const char* delimiters = " ";
    char* token = std::strtok(cmdLine, delimiters);
    while (token)
    {
        try
        {
            g_CommandLineOptionsCBs.at(token)();
        }
        catch (const std::exception & ex)
        {
            g_Log.error("CommandLineOptions error for '{}': {}", token, ex.what());
        }

        token = std::strtok(nullptr, delimiters);
    }
}
