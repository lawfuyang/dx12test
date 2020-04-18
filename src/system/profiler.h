#pragma once

#define MICROPROFILE_ENABLED 1
#define MICROPROFILE_GPU_TIMERS_D3D12 1
#define MICROPROFILE_WEBSERVER_MAXFRAMES 100
#include "extern/microprofile/microprofile.h"

struct MicroProfileThreadLogGpu;
class GfxContext;

class SystemProfiler
{
public:
    DeclareSingletonFunctions(SystemProfiler);

    void Initialize();
    void InitializeGPUProfiler(void* pDevice, void* pCommandQueue);
    void RegisterGPUQueue(void* pCommandQueue, const char* queueName);
    void ShutDown();
    void OnFlip();
    void ResetGPULogs();
    void DumpProfilerBlocks(bool condition, bool immediately = false);

    MicroProfileThreadLogGpu* GetLogForCurrentThread();
    int GetGPUQueueHandle(void* queue) const { return m_GPUQueueToProfilerHandle.at(queue); }

private:
    std::unordered_map<int, MicroProfileThreadLogGpu*> m_ThreadGPULogs;
    std::unordered_map<void*, int> m_GPUQueueToProfilerHandle;

    friend class GPUProfilerContext;
};
#define g_Profiler SystemProfiler::GetInstance()

#define bbeDefineProfilerToken(var, group, name, color) MICROPROFILE_DEFINE(var, group, name, color)
#define bbeProfile(str)                                 MICROPROFILE_SCOPEI("", str, GetCompileTimeCRC32(str))
#define bbeProfileToken(token)                          MICROPROFILE_SCOPE(token)
#define bbeProfileFunction()                            MICROPROFILE_SCOPEI("", __FUNCTION__, GetCompileTimeCRC32(__FUNCTION__))
#define bbeConditionalProfile(condition, name)          MICROPROFILE_CONDITIONAL_SCOPEI(condition, name, name, GetCompileTimeCRC32(name))
#define bbeProfileBlockEnd()                            MICROPROFILE_LEAVE()
#define bbePIXEvent(gfxContext)                         const ScopedPixEvent bbeUniqueVariable(pixEvent) { gfxContext.GetCommandList().Dev() }

#define bbeProfileGPU(gfxContext, name)                                                                   \
    bbePIXEvent(gfxContext);                                                                              \
    MICROPROFILE_SCOPEGPUI_L(g_Profiler.GetLogForCurrentThread(), name, GetCompileTimeCRC32(name));

#define bbeProfileGPUFunction(gfxContext)                                                                                 \
    bbePIXEvent(gfxContext);                                                                                              \
    MICROPROFILE_SCOPEGPUI_L(g_Profiler.GetLogForCurrentThread(), __FUNCTION__, GetCompileTimeCRC32(__FUNCTION__));
