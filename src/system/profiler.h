#pragma once

#define MICROPROFILE_ENABLED 1
#define MICROPROFILE_GPU_TIMERS_D3D12 1
#define MICROPROFILE_WEBSERVER_MAXFRAMES 100
#include "extern/microprofile/microprofile.h"

#include "system/utils.h"

struct MicroProfileThreadLogGpu;
class GfxContext;

class SystemProfiler
{
public:
    DeclareSingletonFunctions(SystemProfiler);

    struct GPUProfileLogContext
    {
        MicroProfileThreadLogGpu* m_GPULog = nullptr;
        bool m_Recording = false;
    };

    void Initialize();
    void InitializeGPUProfiler(void* pDevice, void* pCommandQueue);
    void RegisterGPUQueue(void* pCommandQueue, const char* queueName);
    void ShutDown();
    void OnFlip();
    void ResetGPULogs();
    void DumpProfilerBlocks(bool condition, bool immediately = false);

private:
    std::unordered_map<int, GPUProfileLogContext> m_ThreadGPULogContexts;
    std::unordered_map<void*, int> m_GPUQueueToProfilerHandle;

    friend class GPUProfilerContext;
};
#define g_Profiler SystemProfiler::GetInstance()

class GPUProfilerContext
{
public:
    void Initialize(void* commandList);
    void Submit(void* submissionQueue);

    MicroProfileThreadLogGpu* GetLog() { return m_LogContext->m_GPULog; }

private:
    SystemProfiler::GPUProfileLogContext* m_LogContext = nullptr;
};

#define bbeDefineProfilerToken(var, group, name, color) MICROPROFILE_DEFINE(var, group, name, color)
#define bbeDefineGPUProfilerToken(name)                 MICROPROFILE_DEFINE_GPU(name, bbeTOSTRING(name), GetCompileTimeCRC32(bbeTOSTRING(name)))
#define bbeProfile(str)                                 MICROPROFILE_SCOPEI("", str, GetCompileTimeCRC32(str))
#define bbeProfileToken(token)                          MICROPROFILE_SCOPE(token)
#define bbeProfileFunction()                            MICROPROFILE_SCOPEI("", __FUNCTION__, GetCompileTimeCRC32(__FUNCTION__))
#define bbeConditionalProfile(condition, name)          MICROPROFILE_CONDITIONAL_SCOPEI(condition, name, name, GetCompileTimeCRC32(name))
#define bbeProfileBlockBegin(token)                     MICROPROFILE_ENTER(token)
#define bbeProfileBlockEnd()                            MICROPROFILE_LEAVE()
#define bbePIXEvent(gfxContext)                         const ScopedPixEvent bbeUniqueVariable(pixEvent) { gfxContext.GetCommandList().Dev() }

#define bbeProfileGPU(gfxContext, token)                                         \
    bbePIXEvent(gfxContext);                                                     \
    MICROPROFILE_SCOPEGPU_L(gfxContext.GetGPUProfilerContext().GetLog(), token);
    