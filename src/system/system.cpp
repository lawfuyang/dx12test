#include "graphic/gfxmanager.h"
#include "graphic/guimanager.h"

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
    g_Log.debug("Entering main loop");

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

    g_Log.debug("Exiting main loop");
}

void System::Initialize()
{
    // uncomment to profile engine init phase
    //const ProfilerInstance profilerInstance{ true }; bbeProfileFunction();

    InitializeGraphic();
}

void System::Shutdown()
{
    // uncomment to profile engine shutdown phase
    //const ProfilerInstance profilerInstance{ true }; bbeProfileFunction();

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
