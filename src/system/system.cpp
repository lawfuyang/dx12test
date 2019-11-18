#include "graphic/gfxmanager.h"
#include "graphic/guimanager.h"

void System::ProcessWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CLOSE)
    {
        m_Exit = true;
    }

    GfxManager::GetInstance().GetGUIManager().HandleWindowsInput(hWnd, message, wParam, lParam);
}

void System::Loop()
{
    bbeDebug("Entering main loop");

    do
    {
        bbeProfile("Frame");

        // ProfilerInstance must be before FrameRateController
        const ProfilerInstance profilerInstance;
        const FrameRateController frcInstance;

        Update();

        // make sure I/O ticks happen last
        g_Keyboard.Tick();
        g_Mouse.Tick(ms_LastFrameTimeMs);

    } while (!m_Exit);

    bbeDebug("Exiting main loop");
}

void System::Initialize()
{
    SystemProfiler::InitializeMainThread();

    // uncomment to profile engine init phase
    //const ProfilerInstance profilerInstance{ true };

    bbeDebug("Initializing");

    InitializeGraphic();
}

void System::Shutdown()
{
    // uncomment to profile engine shutdown phase
    //const ProfilerInstance profilerInstance{ true };

    ShutdownGraphic();
}

void System::InitializeGraphic()
{
    bbeProfileFunction();
    bbeDebug("Initializing Graphic");

    GfxManager::GetInstance().Initialize();
}

void System::ShutdownGraphic()
{
    bbeDebug("Shutting down Graphic");

    GfxManager::GetInstance().ShutDown();
}

void System::Update()
{
    bbeProfileFunction();

    tf::Taskflow tf;

    tf::Task graphicTask = tf.emplace([](tf::Subflow& subflow) { GfxManager::GetInstance().ScheduleGraphicTasks(subflow); });
    graphicTask.name("Graphic Tasks");

    m_Executor.run(tf).wait();
}

FrameRateController::FrameRateController()
{
    m_FrameEndTime = std::chrono::high_resolution_clock::now() + std::chrono::microseconds{ 1000000 / System::FPS_LIMIT };

    ::LARGE_INTEGER liTime;
    liTime.QuadPart = -(std::chrono::nanoseconds(10000000 / System::FPS_LIMIT).count() - 10000); // wait 1 ms less due to timer inaccuracy
    ::SetWaitableTimer(ms_TimerHandle, &liTime, 0, 0, 0, FALSE);
}

FrameRateController::~FrameRateController()
{
    UpdateAvgRealFrameTime(m_StopWatch.ElapsedUS());

    bbeProfile("Idle");

    //::WaitForSingleObject(ms_TimerHandle, INFINITE); // This is unreliable as fuck
    while (std::chrono::high_resolution_clock::now() < m_FrameEndTime) { std::this_thread::yield(); } // busy wait until exactly fps limit

    UpdateAvgCappedFrameTime(m_StopWatch.ElapsedUS());

    System::ms_LastFrameTimeMs      = (float)m_StopWatch.ElapsedUS() / 1000;
    System::ms_AvgRealFrameTimeMs   = ms_AvgRealFrameTimeMs;
    System::ms_AvgFPS               = ms_AvgFPS;
    System::ms_AvgCappedFrameTimeMs = ms_AvgCappedFrameTimeMs;
    System::ms_AvgCappedFPS         = ms_AvgCappedFPS;
}

void FrameRateController::UpdateAvgFrameTimeInternal(FpsArray& array, uint8_t& arrCursor, uint64_t elapsedUs, float& avgFrameTime, float& avgFPS)
{
    array[arrCursor++] = elapsedUs;
    arrCursor = arrCursor % NUM_AVG_FRAMES;

    avgFrameTime = (float)(std::accumulate(array, array + NUM_AVG_FRAMES, UINT64_C(0)) / NUM_AVG_FRAMES) / 1000;
    avgFPS = 1000.0f / avgFrameTime;
}
