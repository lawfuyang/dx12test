#include "system/profiler.h"

#include "system/logger.h"
#include "system/keyboard.h"

void SystemProfiler::EnableProfiling()
{
    if (!::profiler::isEnabled())
    {
        EASY_PROFILER_ENABLE;

        g_Log.info("Profiling enabled, capturing frame(s) data...");
    }
}

void SystemProfiler::DisableProfilingAndDumpToFile()
{
    if (!::profiler::isEnabled())
    {
        g_Log.error("ms_FramesProfiled must be true!");
        return;
    }

    BBE_SCOPED_UNSET(bool, ms_DumpingBlocks, true);

    const std::string dumpFilePath = StringFormat("..\\bin\\%s.prof", GetTimeStamp().c_str());
    g_Log.info("Dumping profile capture {}", dumpFilePath.c_str());

    const uint32_t numBlocksDumped = ::profiler::dumpBlocksToFile(dumpFilePath.c_str());
    if (numBlocksDumped == 0) g_Log.error("Error dumping profile blocks!");

    EASY_PROFILER_DISABLE;
    ms_FramesProfiled = 0;
    ms_MsProfiled = 0.0f;
}

ProfilerInstance::ProfilerInstance(bool dumpOnExitScope)
    : m_DumpOnExitScope(dumpOnExitScope)
{
    if (dumpOnExitScope || Keyboard::IsKeyPressed(Keyboard::KEY_P))
    {
        SystemProfiler::EnableProfiling();
    }
}

ProfilerInstance::~ProfilerInstance()
{
    static const uint32_t s_MAX_FRAMES = 50;
    static const float s_MAX_MS = 1000.0f;

    if (::profiler::isEnabled())
    {
        if (m_DumpOnExitScope || 
            SystemProfiler::ms_FramesProfiled++ >= s_MAX_FRAMES || 
            (SystemProfiler::ms_MsProfiled += m_StopWatch.ElapsedMS()) >= s_MAX_MS)
        {
            SystemProfiler::DisableProfilingAndDumpToFile();
        }
    }
}
