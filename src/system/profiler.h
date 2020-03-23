#pragma once

#define MICROPROFILE_ENABLED 1
#define MICROPROFILE_GPU_TIMERS_D3D12 1
#define MICROPROFILE_WEBSERVER_MAXFRAMES 100
#include "extern/microprofile/microprofile.h"

#include "system/utils.h"

struct MicroProfileThreadLogGpu;

class SystemProfiler
{
public:
    DeclareSingletonFunctions(SystemProfiler);

    void Initialize();
    void InitializeGPUProfiler(void* pDevice, void* pCommandQueue);
    void RegisterGPUQueue(void* pCommandQueue, const char* queueName);
    void RegisterThread(const char* name);
    void ShutDown();
    void OnFlip();
    void DumpProfilerBlocks(bool condition, bool immediately = false);

private:
    MicroProfileThreadLogGpu* m_GPULogs[MICROPROFILE_MAX_THREADS] = {};
    std::unordered_map<void*, int> m_GPUQueueToProfilerHandle;

    friend class GPUProfilerContext;
};
#define g_Profiler SystemProfiler::GetInstance()

class GPUProfilerContext
{
public:
    void Initialize(void* commandList, uint32_t id);
    void Submit(void* submissionQueue);

    MicroProfileThreadLogGpu* GetLog() { return m_Log; }

private:
    uint32_t m_ID = ~0;
    MicroProfileThreadLogGpu* m_Log = nullptr;

    friend class GPUProfilerScopeHandler;
};

#define bbeDefineProfilerToken(var, group, name, color) MICROPROFILE_DEFINE(var, group, name, color)
#define bbeProfile(str)                                 MICROPROFILE_SCOPEI("", str, GetCompileTimeCRC32(str))
#define bbeProfileToken(token)                          MICROPROFILE_SCOPE(token)
#define bbeProfileFunction()                            MICROPROFILE_SCOPEI("", __FUNCTION__, GetCompileTimeCRC32(__FUNCTION__))
#define bbeConditionalProfile(condition, name)          MICROPROFILE_CONDITIONAL_SCOPEI(condition, name, name, GetCompileTimeCRC32(name))
#define bbeProfileBlockBegin(token)                     MICROPROFILE_ENTER(token)
#define bbeProfileBlockEnd()                            MICROPROFILE_LEAVE()

// TODO: refactor MICROPROFILE gpu stuff
//#define bbeProfileGPU(gfxContext, str)                  MICROPROFILE_SCOPEGPUI_L(gfxContext.GetGPUProfilerContext().GetLog(), str, GetCompileTimeCRC32(str))
//#define bbeProfileGPUFunction(gfxContext)               MICROPROFILE_SCOPEGPUI_L(gfxContext.GetGPUProfilerContext().GetLog(), __FUNCTION__, GetCompileTimeCRC32(__FUNCTION__))

#define bbeProfileGPU(gfxContext, str)    ((void)0)
#define bbeProfileGPUFunction(gfxContext) ((void)0)