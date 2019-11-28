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
        g_Mouse.Tick(ms_RealFrameTimeMs);
        ++ms_SystemFrameNumber;
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

    GfxManager::GetInstance().Initialize();
}

void System::ShutdownGraphic()
{
    GfxManager::GetInstance().ShutDown();
}

void System::Update()
{
    bbeProfileFunction();

    tf::Taskflow tf;

    GfxManager::GetInstance().ScheduleGraphicTasks(tf);

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
    float realFrameTimeMs = (float)m_StopWatch.ElapsedUS() / 1000;
    float fps = 1000.0f / realFrameTimeMs;

    bbeProfile("Idle");

    //::WaitForSingleObject(ms_TimerHandle, INFINITE); // This is unreliable as fuck
    while (std::chrono::high_resolution_clock::now() < m_FrameEndTime) { std::this_thread::yield(); } // busy wait until exactly fps limit

    float cappedFrameTimeMs = (float)m_StopWatch.ElapsedUS() / 1000;
    float cappedFPS = 1000.0f / cappedFrameTimeMs;

    System::ms_RealFrameTimeMs   = realFrameTimeMs;
    System::ms_FPS               = fps;
    System::ms_CappedFrameTimeMs = cappedFrameTimeMs;
    System::ms_CappedFPS         = cappedFPS;
}
