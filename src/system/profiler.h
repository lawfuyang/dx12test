#pragma once

#define BUILD_WITH_EASY_PROFILER
#include "extern/easy/profiler.h"

class SystemProfiler
{
public:
    static void InitializeMainThread() { EASY_MAIN_THREAD; }
    static void InitializeWorkerThread(const char* name) { EASY_THREAD(name); }
    static void EnableProfiling();

private:
    static void DisableProfilingAndDumpToFile();

    inline static uint32_t ms_FramesProfiled = 0;
    inline static float    ms_MsProfiled     = 0.0f;

    friend class ProfilerInstance;
};

class ProfilerInstance
{
public:
    ProfilerInstance(bool dumpOnExitScope = false);
    ~ProfilerInstance();

private:
    bool m_DumpOnExitScope = false;

    StopWatch m_StopWatch;
};

#define bbeProfile(str)      EASY_BLOCK(str, (profiler::color_t)(GetCompileTimeCRC32(str) | 0XFF000000))
#define bbeProfileFunction() EASY_BLOCK(EASY_FUNC_NAME, (profiler::color_t)(GetCompileTimeCRC32(EASY_FUNC_NAME) | 0XFF000000))
