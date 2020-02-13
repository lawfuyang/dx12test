#include "system/profiler.h"

#pragma comment(lib, "ws2_32.lib")

#include "system/logger.h"

void SystemProfiler::Initialize()
{
    g_Log.info("Initializing profiler");

    //turn on profiling
    MicroProfileOnThreadCreate("Main");
    MicroProfileSetEnableAllGroups(true);
    MicroProfileSetForceMetaCounters(true);
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
}

void SystemProfiler::OnFlip()
{
    MicroProfileFlip(nullptr);
}

void SystemProfiler::DumpProfilerBlocks(bool condition, bool immediately, bool ignoreCD)
{
    if (!condition)
        return;

    static StopWatch s_StopWatch;

    if (ignoreCD || s_StopWatch.ElapsedMS() > 100)
    {
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

        s_StopWatch.Restart();
    }
}
