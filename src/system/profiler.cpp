#include "system/profiler.h"

#pragma comment(lib, "ws2_32.lib")

#include "system/logger.h"

class ProfilerFlipThread
{
public:
    DeclareSingletonFunctions(ProfilerFlipThread);

    void Loop()
    {
        g_Log.info("Initializing ProfilerFlipThread");

        m_FlipTriggerEvent = ::CreateEventA(nullptr, true, false, "ProfilerFlipTriggerEvent");

        while (1)
        {
            ::WaitForSingleObject(m_FlipTriggerEvent, INFINITE);

            if (!m_Done)
            {
                MicroProfileFlip(nullptr);
            }
            else
            {
                break;
            }
        }
    }

    void OnFlip()
    {
        ::SetEvent(m_FlipTriggerEvent);
        ::ResetEvent(m_FlipTriggerEvent);
    }

    void ShutDown()
    {
        m_Done = true;
        OnFlip();
    }

private:
    bool     m_Done             = false;
    ::HANDLE m_FlipTriggerEvent = nullptr;
};

void SystemProfiler::Initialize()
{
    g_Log.info("Initializing profiler");

    //turn on profiling
    MicroProfileOnThreadCreate("Main");
    MicroProfileSetEnableAllGroups(true);
    MicroProfileSetForceMetaCounters(true);

    m_FlipThread = std::thread{ []() { ProfilerFlipThread::GetInstance().Loop(); } };
}

void SystemProfiler::InitializeGPUProfiler(void* pDevice, void* pCommandQueue)
{
    MicroProfileGpuInitD3D12(pDevice, 1, (void**)&pCommandQueue);
    MicroProfileSetCurrentNodeD3D12(0);
}

void SystemProfiler::ShutDown()
{
    g_Log.info("Shutting down profiler");

    MicroProfileGpuShutdown();
    MicroProfileShutdown();

    ProfilerFlipThread::GetInstance().ShutDown();
    m_FlipThread.join();
}

void SystemProfiler::OnFlip()
{
    ProfilerFlipThread::GetInstance().OnFlip();
}

void SystemProfiler::DumpProfilerBlocks(bool condition, bool immediately)
{
    if (!condition)
        return;

    const std::string dumpFilePath = StringFormat("..\\bin\\Profiler_Results_%s.html", GetTimeStamp().c_str());
    g_Log.info("Dumping profile capture {}", dumpFilePath.c_str());

    if (immediately)
    {
        MicroProfileDumpFileImmediately(dumpFilePath.c_str(), nullptr, nullptr);
    }
    else
    {
        MicroProfileDumpFile(dumpFilePath.c_str(), nullptr, 0.0f, 0.0f);
    }
}
