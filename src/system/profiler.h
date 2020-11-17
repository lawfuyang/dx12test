#pragma once

struct MicroProfileThreadLogGpu;
class GfxContext;
class GfxCommandList;

class SystemProfiler
{
public:
    DeclareSingletonFunctions(SystemProfiler);

    void Initialize();
    void InitializeGPUProfiler(void* pDevice, void* pCommandQueue);
    void RegisterGPUQueue(void* pCommandQueue, const char* queueName);
    void ShutDown();
    void OnFlip();
    void DumpProfilerBlocks(bool condition, bool immediately = false);
    int GetGPUQueueHandle(void* queue) const { return m_GPUQueueToProfilerHandle.at(queue); }
    void SubmitAllGPULogsToQueue(ID3D12CommandQueue*);

    MicroProfileThreadLogGpu* GetGPULogForCurrentThread(const GfxCommandList&);

private:
    std::unordered_map<void*, int> m_GPUQueueToProfilerHandle;

    struct GPULogContext
    {
        bool m_Begun = false;
        MicroProfileThreadLogGpu* m_Log = nullptr;
    };

    std::mutex m_GPULogsMutex;
    std::unordered_map<std::thread::id, GPULogContext> m_PerThreadGPULogs;
};
#define g_Profiler SystemProfiler::GetInstance()

#define bbeAutoLock(lck) \
    bbeProfileLock(lck); \
    const std::lock_guard bbeUniqueVariable(ScopedLock) {lck};

#if defined(BBE_USE_PROFILER)
    #define bbeDefineProfilerToken(var, group, name, color) MICROPROFILE_DEFINE(var, group, name, color)
    #define bbeProfile(str)                                 MICROPROFILE_SCOPEI("", str, GetCompileTimeCRC32(str))
    #define bbeProfileToken(token)                          MICROPROFILE_SCOPE(token)
    #define bbeProfileFunction()                            MICROPROFILE_SCOPEI("", __FUNCTION__, GetCompileTimeCRC32(__FUNCTION__))
    #define bbeConditionalProfile(condition, name)          MICROPROFILE_CONDITIONAL_SCOPEI(condition, name, name, GetCompileTimeCRC32(name))
    #define bbeProfileBlockEnd()                            MICROPROFILE_LEAVE()
    #define bbeProfileLock(lck)                             MICROPROFILE_SCOPEI("Locks", bbeTOSTRING(lck), 0xFF0000);
    #define bbePIXEvent(gfxContext)                         const ScopedPixEvent bbeUniqueVariable(pixEvent) { gfxContext.GetCommandList().Dev() }

    #define bbeProfileGPU(gfxContext, name) \
        bbePIXEvent(gfxContext); \
        MICROPROFILE_SCOPEGPUI_L(g_Profiler.GetGPULogForCurrentThread(gfxContext.GetCommandList()), name, GetCompileTimeCRC32(name));

    #define bbeProfileGPUFunction(gfxContext) \
        bbePIXEvent(gfxContext); \
        MICROPROFILE_SCOPEGPUI_L(g_Profiler.GetGPULogForCurrentThread(gfxContext.GetCommandList()), __FUNCTION__, GetCompileTimeCRC32(__FUNCTION__));
#else
    #define bbeDefineProfilerToken(var, group, name, color) __noop
    #define bbeProfile(str)                                 __noop
    #define bbeProfileToken(token)                          __noop
    #define bbeProfileFunction()                            __noop
    #define bbeConditionalProfile(condition, name)          __noop
    #define bbeProfileBlockEnd()                            __noop
    #define bbeProfileLock(lck)                             __noop
    #define bbePIXEvent(gfxContext)                         __noop
    #define bbeProfileGPU(gfxContext, name)                 __noop
    #define bbeProfileGPUFunction(gfxContext)               __noop
#endif
