#include <system/profiler.h>

#include <system/logger.h>

#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxcommandlist.h>

struct GPULog
{
    MicroProfileThreadLogGpu* m_GPULog = nullptr;
    D3D12_COMMAND_LIST_TYPE m_Type = (D3D12_COMMAND_LIST_TYPE)0xDEADBEEF;
    bool m_Recording = false;
};
thread_local GPULog tl_GPULog;

void SystemProfiler::Initialize()
{
    g_Log.info("Initializing CPU Profiler");

    //turn on profiling
    MicroProfileOnThreadCreate("Main");
    MicroProfileSetEnableAllGroups(true);
}

void SystemProfiler::RegisterGPUQueue(void* pDevice, void* pCommandQueue, const char* queueName)
{
#if defined(BBE_USE_GPU_PROFILER)
    assert(pDevice);
    assert(pCommandQueue);
    assert(queueName);

    MicroProfileGpuInitD3D12(pDevice, 1, &pCommandQueue);
    m_GPUProfileHandles.push_back({ pCommandQueue, MICROPROFILE_GPU_INIT_QUEUE(queueName) });

    // If we support multiple adapters, need to change this
    MicroProfileSetCurrentNodeD3D12(0);
#endif
}

void SystemProfiler::ShutDown()
{
    g_Log.info("Shutting down profiler");

    for (const GPUQueueAndHandle& elem : m_GPUProfileHandles)
    {
        MICROPROFILE_GPU_FREE_QUEUE(elem.m_Handle);
    }

    for (MicroProfileThreadLogGpu* gpuLog : m_AllGPULogs)
    {
        MicroProfileThreadLogGpuFree(gpuLog);
    }

    MicroProfileShutdown();
}

void SystemProfiler::OnFlip()
{
    MicroProfileFlip(nullptr);
}

void SystemProfiler::DumpProfilerBlocks(bool condition, bool immediately)
{
    if (!condition)
        return;

    const std::string dumpFilePath = StringFormat("..\\bin\\Profiler_Results_%s.html", GetTimeStamp());
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

int SystemProfiler::GetHandleForQueue(void* pCommandQueue) const
{
    // Linear search... because we never will have more than 2+ queues
    auto it = std::find_if(m_GPUProfileHandles.begin(), m_GPUProfileHandles.end(), [pCommandQueue](const GPUQueueAndHandle& elem) { return elem.m_Queue == pCommandQueue; });
    assert(it != m_GPUProfileHandles.end());

    return it->m_Handle;
}

void SystemProfiler::SubmitGPULog()
{
    assert(tl_GPULog.m_GPULog);
    assert(tl_GPULog.m_Type != 0xDEADBEEF);
    assert(tl_GPULog.m_Recording);

    void* pCommandQueue = g_GfxCommandListsManager.GetCommandQueue(tl_GPULog.m_Type).Dev();
    int queueHandle = GetHandleForQueue(pCommandQueue);
    MICROPROFILE_GPU_SUBMIT(queueHandle, MICROPROFILE_GPU_END(tl_GPULog.m_GPULog));

    tl_GPULog.m_Type = (D3D12_COMMAND_LIST_TYPE)0xDEADBEEF;
    tl_GPULog.m_Recording = false;
}

void SystemProfiler::ResetAllGPULogs()
{
    assert(!tl_GPULog.m_Recording);

    for (MicroProfileThreadLogGpu* gpuLog : m_AllGPULogs)
    {
        MICROPROFILE_THREADLOGGPURESET(gpuLog);
    }
}

void SystemProfiler::BeginGPURecording(const GfxCommandList& cmdList)
{
    // This branch should only go in the very first call to a GPU profiling macro
    if (!tl_GPULog.m_GPULog)
    {
        tl_GPULog.m_GPULog = MicroProfileThreadLogGpuAlloc();

        static std::mutex GPULogsMutex;
        bbeAutoLock(GPULogsMutex);
        m_AllGPULogs.push_back(tl_GPULog.m_GPULog);
    }
    assert(tl_GPULog.m_Type == 0xDEADBEEF);
    assert(!tl_GPULog.m_Recording);

    tl_GPULog.m_Type = cmdList.GetType();
    tl_GPULog.m_Recording = true;
    MICROPROFILE_GPU_BEGIN(cmdList.Dev(), tl_GPULog.m_GPULog);
}

MicroProfileThreadLogGpu* SystemProfiler::GetGPULogForThread()
{
    return tl_GPULog.m_GPULog;
}
