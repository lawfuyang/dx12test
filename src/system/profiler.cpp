#include <system/profiler.h>

#include <system/logger.h>

#include <graphic/gfxcontext.h>
#include <graphic/gfxcommandlist.h>

void SystemProfiler::Initialize()
{
    g_Log.info("Initializing CPU Profiler");

    //turn on profiling
    MicroProfileOnThreadCreate("Main");
    MicroProfileSetEnableAllGroups(true);

    m_ThreadGPULogs.reserve(g_TasksExecutor.num_workers());
    m_GPUQueueToProfilerHandle.reserve(D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE + 1);
}

void SystemProfiler::InitializeGPUProfiler(void* pDevice, void* pCommandQueue)
{
    g_Log.info("Initializing GPU Profiler");

    MicroProfileGpuInitD3D12(pDevice, 1, (void**)&pCommandQueue);
    MicroProfileSetCurrentNodeD3D12(0);
}

void SystemProfiler::RegisterGPUQueue(void* pCommandQueue, const char* queueName)
{
    m_GPUQueueToProfilerHandle[pCommandQueue] = MICROPROFILE_GPU_INIT_QUEUE(queueName);
}

void SystemProfiler::ShutDown()
{
    g_Log.info("Shutting down profiler");

    for (auto& handles : m_GPUQueueToProfilerHandle)
    {
        MICROPROFILE_GPU_FREE_QUEUE(handles.second);
    }

    MicroProfileShutdown();
}

void SystemProfiler::OnFlip()
{
    MicroProfileFlip(nullptr);
}

void SystemProfiler::ResetGPULogs()
{
    for (const std::pair<int, MicroProfileThreadLogGpu*>& elem : m_ThreadGPULogs)
    {
        MICROPROFILE_THREADLOGGPURESET(elem.second);
    }
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
        // TODO: support auto dumping on CPU/GPU spike
        MicroProfileDumpFile(dumpFilePath.c_str(), nullptr, 0.0f, 0.0f);
    }
}

thread_local bool g_ThisThreadGPULogInit = false;

MicroProfileThreadLogGpu* SystemProfiler::GetLogForCurrentThread()
{
    const int thisThreadID = g_TasksExecutor.this_worker_id();
    if (!g_ThisThreadGPULogInit)
    {
        g_Profiler.m_ThreadGPULogs[thisThreadID] = MicroProfileThreadLogGpuAlloc();
        g_ThisThreadGPULogInit = true;
    }

    return m_ThreadGPULogs.at(thisThreadID);
}
