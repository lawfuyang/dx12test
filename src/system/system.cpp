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
    g_Log.info("Entering main loop");

    do
    {
        bbeProfile("Frame");

        // ProfilerInstance must be before FrameRateController
        const ProfilerInstance profilerInstance{ /*ms_SystemFrameNumber == 0*/ };
        const FrameRateController frcInstance;

        if (Keyboard::IsKeyPressed(Keyboard::KEY_P))
        {
            SystemProfiler::EnableProfiling();
        }

        if (Keyboard::IsKeyPressed(Keyboard::KEY_J))
        {
            GfxManager::GetInstance().DumpGfxMemory();
        }

        tf::Taskflow tf;

        GfxManager::GetInstance().ScheduleGraphicTasks(tf);

        m_Executor.run(tf).wait();

        // make sure I/O ticks happen last
        g_Keyboard.Tick();
        g_Mouse.Tick(ms_RealFrameTimeMs);
        ++ms_SystemFrameNumber;
    } while (!m_Exit);

    g_Log.info("Exiting main loop");
}

void System::Initialize()
{
    // uncomment to profile engine init phase
    //const ProfilerInstance profilerInstance{ true }; bbeProfileFunction();

    tf::Taskflow tf;

    GfxManager::GetInstance().Initialize(tf);
    // TODO: init other System/Engine/Physics systems here in parallel

    m_Executor.run(tf).wait();
}

void System::Shutdown()
{
    // uncomment to profile engine shutdown phase
    //const ProfilerInstance profilerInstance{ true }; bbeProfileFunction();

    GfxManager::GetInstance().ShutDown();
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
