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
    MicroProfileSetForceMetaCounters(true);

    m_ThreadGPULogContexts.reserve(g_TasksExecutor.num_workers());
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
    m_GPUQueueToProfilerHandle[pCommandQueue] = MicroProfileInitGpuQueue(queueName);
}

void SystemProfiler::RegisterThread(const char* name)
{
    MicroProfileOnThreadCreate(name);
}

void SystemProfiler::ShutDown()
{
    g_Log.info("Shutting down profiler");

    for (auto& handles : m_GPUQueueToProfilerHandle)
    {
        MicroProfileFreeGpuQueue(handles.second);
    }

    MicroProfileShutdown();
}

void SystemProfiler::OnFlip()
{
    for (const auto& elem : m_ThreadGPULogContexts)
    {
        const GPUProfileLogContext& context = elem.second;
        MicroProfileThreadLogGpuReset(context.m_GPULog);
    }

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

thread_local bool g_ThisThreadGPULogInit = false;

void GPUProfilerContext::Initialize(void* commandList)
{
    assert(commandList);

    const int thisThreadID = g_TasksExecutor.this_worker_id();
    if (!g_ThisThreadGPULogInit)
    {
        g_Profiler.m_ThreadGPULogContexts[thisThreadID].m_GPULog = MicroProfileThreadLogGpuAlloc();
        g_ThisThreadGPULogInit = true;
    }

    m_LogContext = &g_Profiler.m_ThreadGPULogContexts[thisThreadID];
}

void GPUProfilerContext::Submit(void* submissionQueue)
{
    assert(submissionQueue);
    assert(m_LogContext);

    const int32_t queueHandle = g_Profiler.m_GPUQueueToProfilerHandle.at(submissionQueue);

    for (MicroProfileToken& token : m_Tokens)
    {
        MICROPROFILE_GPU_SUBMIT(queueHandle, token);
    }
    m_Tokens.clear();
}

ScopedGPUProfileHandler::ScopedGPUProfileHandler(MicroProfileToken& token, GfxContext& gfxContext)
    : m_GfxContext(gfxContext)
    , m_Token(token)
{
    void* cmdList = m_GfxContext.GetCommandList().Dev();
    MicroProfileThreadLogGpu* log = m_GfxContext.GetGPUProfilerContext().GetLog();

    MICROPROFILE_GPU_BEGIN(cmdList, log);
    MICROPROFILE_GPU_ENTER_TOKEN_L(log, m_Token);
}

ScopedGPUProfileHandler::~ScopedGPUProfileHandler()
{
    GPUProfilerContext& context = m_GfxContext.GetGPUProfilerContext();

    MicroProfileThreadLogGpu* log = context.GetLog();

    MICROPROFILE_GPU_LEAVE_L(log);
    m_Token = MICROPROFILE_GPU_END(log);

    context.m_Tokens.push_back(m_Token);
}
