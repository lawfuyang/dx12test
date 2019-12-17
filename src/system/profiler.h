#pragma once

#define BUILD_WITH_EASY_PROFILER
#include "extern/easy/profiler.h"

#include "system/stopwatch.h"
#include "system/utils.h"

class SystemProfiler
{
public:
    static void EnableProfiling();
    static void DisableProfilingAndDumpToFile();

    static bool IsDumpingBlocks() { return ms_DumpingBlocks; }

private:
    inline static uint32_t ms_FramesProfiled = 0;
    inline static float    ms_MsProfiled     = 0.0f;
    inline static bool     ms_DumpingBlocks  = false;

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

#define bbeProfile(str)           EASY_BLOCK(str, (profiler::color_t)(GetCompileTimeCRC32(str) | 0XFF000000))
#define bbeProfileFunction()      EASY_BLOCK(EASY_FUNC_NAME, (profiler::color_t)(GetCompileTimeCRC32(EASY_FUNC_NAME) | 0XFF000000))
#define bbeProfileBlockBegin(str) EASY_NONSCOPED_BLOCK(str, profiler::colors::Red);
#define bbeProfileBlockEnd()      EASY_END_BLOCK;
