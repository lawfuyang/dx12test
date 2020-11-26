#include <system/profiler.h>

#include <system/logger.h>

#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxcommandlist.h>

void SystemProfiler::Initialize()
{
    g_Log.info("Initializing CPU Profiler");

    //turn on profiling
    MicroProfileOnThreadCreate("Main");
    MicroProfileSetEnableAllGroups(true);

    m_GPUQueueToProfilerHandle.reserve(D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE + 1);
    m_PerThreadGPULogs.reserve(std::thread::hardware_concurrency());
}

void SystemProfiler::RegisterGPUQueue(void* pDevice, void* pCommandQueue, const char* queueName)
{
    MicroProfileGpuInitD3D12(pDevice, 1, (void**)&pCommandQueue);
    m_GPUQueueToProfilerHandle[pCommandQueue] = MICROPROFILE_GPU_INIT_QUEUE(queueName);

    // If we support multiple adapters, need to change this
    MicroProfileSetCurrentNodeD3D12(0);
}

void SystemProfiler::ShutDown()
{
    g_Log.info("Shutting down profiler");

    for (auto& handles : m_GPUQueueToProfilerHandle)
    {
        MICROPROFILE_GPU_FREE_QUEUE(handles.second);
    }

    for (const auto& gpuLog : m_PerThreadGPULogs)
    {
        MicroProfileThreadLogGpuFree(gpuLog.second.m_Log);
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

void SystemProfiler::SubmitAllGPULogsToQueue(void* queue)
{
    for (auto& gpuLog : m_PerThreadGPULogs)
    {
        const uint64_t token = MICROPROFILE_GPU_END(gpuLog.second.m_Log);
        MICROPROFILE_GPU_SUBMIT(m_GPUQueueToProfilerHandle.at(queue), token);

        gpuLog.second.m_Begun = false;
    }
}

thread_local MicroProfileThreadLogGpu* tl_GPULog = nullptr;

MicroProfileThreadLogGpu* SystemProfiler::GetGPULogForCurrentThread(const GfxCommandList& cmdList)
{
    const std::thread::id thisThreadID = std::this_thread::get_id();

    if (!tl_GPULog)
    {
        tl_GPULog = MicroProfileThreadLogGpuAlloc();

        bbeAutoLock(m_GPULogsMutex);
        m_PerThreadGPULogs[thisThreadID].m_Log = tl_GPULog;
    }

    if (!m_PerThreadGPULogs[thisThreadID].m_Begun)
    {
        MICROPROFILE_GPU_BEGIN(cmdList.Dev(), tl_GPULog);
        m_PerThreadGPULogs[thisThreadID].m_Begun = true;
    }

    return tl_GPULog;
}
