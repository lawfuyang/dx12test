#include "system/profiler.h"

#pragma comment(lib, "ws2_32.lib")

#include "system/logger.h"

void SystemProfiler::Initialize()
{
    g_Log.info("Initializing CPU Profiler");

    //turn on profiling
    MicroProfileOnThreadCreate("Main");
    MicroProfileSetEnableAllGroups(true);
    MicroProfileSetForceMetaCounters(true);

    m_GPULogs.reserve(16);
}

void SystemProfiler::InitializeGPUProfiler(void* pDevice, void* pCommandQueue)
{
    g_Log.info("Initializing GPU Profiler");

    MicroProfileGpuInitD3D12(pDevice, 1, (void**)&pCommandQueue);
    MicroProfileSetCurrentNodeD3D12(0);
}

void SystemProfiler::RegisterGPUQueue(void* pCommandQueue, const char* queueName)
{
    m_GPUQueueToProfilerHandle[pCommandQueue] = MicroProfileInitGpuQueue(queueName);
}

void SystemProfiler::RegisterThread(const char* name)
{
    MicroProfileOnThreadCreate(name);
}

void SystemProfiler::ShutDown()
{
    g_Log.info("Shutting down profiler");

    for (MicroProfileThreadLogGpu* gpuLog : m_GPULogs)
    {
        MicroProfileThreadLogGpuFree(gpuLog);
    }
    m_GPULogs.clear();

    MicroProfileShutdown();
}

void SystemProfiler::OnFlip()
{
    for (MicroProfileThreadLogGpu* gpuLog : m_GPULogs)
    {
        MicroProfileThreadLogGpuReset(gpuLog);
    }

    //g_ProfilerFlipThread.OnFlip();
    MicroProfileFlip(nullptr);
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

void GPUProfilerContext::Initialize(void* commandList, uint32_t id)
{
    if (g_Profiler.m_GPULogs.size() <= id)
    {
        bbeMultiThreadDetector();
        g_Profiler.m_GPULogs.resize(id + 1);

        // alloc new logs if necessary
        // TODO: This is bullshit... while not critical for perf, do it in a better way?
        for (MicroProfileThreadLogGpu*& log : g_Profiler.m_GPULogs)
        {
            if (!log)
            {
                log = MicroProfileThreadLogGpuAlloc();
            }
        }
    }

    m_Log = g_Profiler.m_GPULogs.at(id);

    MicroProfileGpuBegin(commandList, m_Log);
}

void GPUProfilerContext::Submit(void* submissionQueue)
{
    assert(m_Log);

    const uint64_t token = MicroProfileGpuEnd(m_Log);

    const int32_t queueHandle = g_Profiler.m_GPUQueueToProfilerHandle.at(submissionQueue);
    MicroProfileGpuSubmit(queueHandle, token);
}
