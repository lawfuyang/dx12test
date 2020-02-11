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

void SystemProfiler::ShutDown()
{
    g_Log.info("Shutting down profiler");

    MicroProfileShutdown();
}

void SystemProfiler::Flip()
{
    MicroProfileFlip(nullptr);
}

void SystemProfiler::DumpProfilerBlocks()
{
    const std::string dumpFilePath = StringFormat("..\\bin\\%s.csv", GetTimeStamp().c_str());
    g_Log.info("Dumping profile capture {}", dumpFilePath.c_str());

    MicroProfileDumpFile(nullptr, dumpFilePath.c_str(), 0.0f, 0.0f);
}
