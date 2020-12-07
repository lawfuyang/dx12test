#pragma once

struct MicroProfileThreadLogGpu;
class GfxContext;
class GfxCommandList;

class SystemProfiler
{
public:
    DeclareSingletonFunctions(SystemProfiler);

    void Initialize();
    void RegisterGPUQueue(void* pDevice, void* pCommandQueue, const char* queueName);
    void ShutDown();
    void OnFlip();
    void DumpProfilerBlocks(bool condition, bool immediately = false);
    void SubmitGPULog();
    void ResetAllGPULogs();
    void BeginGPURecording(const GfxCommandList& cmdList);

    MicroProfileThreadLogGpu* GetGPULogForThread();

private:
    int GetHandleForQueue(void*) const;

    struct GPUQueueAndHandle
    {
        void* m_Queue;
        int m_Handle;
    };

    InplaceArray<GPUQueueAndHandle, 2> m_GPUProfileHandles;
    InplaceArray<MicroProfileThreadLogGpu*, 16> m_AllGPULogs;
};
#define g_Profiler SystemProfiler::GetInstance()

#define bbePIXEvent(gfxContext) const ScopedPixEvent bbeUniqueVariable(pixEvent) { gfxContext.GetCommandList().Dev() }

#if defined(BBE_USE_PROFILER)
    #define bbeAutoLock(lck) const AutoScopeCaller bbeUniqueVariable(ScopedLock){ [&](){ bbeProfileLock(lck); lck.lock(); }, [&](){ lck.unlock(); } };

    #define bbeDefineProfilerToken(var, group, name, color) MICROPROFILE_DEFINE(var, group, name, color)
    #define bbeProfile(str)                                 MICROPROFILE_SCOPEI("", str, GetCompileTimeCRC32(str))
    #define bbeProfileToken(token)                          MICROPROFILE_SCOPE(token)
    #define bbeProfileFunction()                            MICROPROFILE_SCOPEI("", __FUNCTION__, GetCompileTimeCRC32(__FUNCTION__))
    #define bbeConditionalProfile(condition, name)          MICROPROFILE_CONDITIONAL_SCOPEI(condition, name, name, GetCompileTimeCRC32(name))
    #define bbeProfileBlockEnd()                            MICROPROFILE_LEAVE()
    #define bbeProfileLock(lck)                             MICROPROFILE_SCOPEI("Locks", bbeTOSTRING(lck), 0xFF0000);
#else
    #define bbeAutoLock(lck)                                std::lock_guard bbeUniqueVariable(ScopedLock){lck};
    #define bbeDefineProfilerToken(var, group, name, color) __noop
    #define bbeProfile(str)                                 __noop
    #define bbeProfileToken(token)                          __noop
    #define bbeProfileFunction()                            __noop
    #define bbeConditionalProfile(condition, name)          __noop
    #define bbeProfileBlockEnd()                            __noop
    #define bbeProfileLock(lck)                             __noop
#endif


#if defined(BBE_USE_GPU_PROFILER)
    #define bbeProfileGPU(gfxContext, name)                                                             \
            bbePIXEvent(gfxContext);                                                                    \
            bbeOnExitScope([] { g_Profiler.SubmitGPULog(); });                                          \
            g_Profiler.BeginGPURecording(gfxContext.GetCommandList());                                  \
            MICROPROFILE_SCOPEGPUI_L(g_Profiler.GetGPULogForThread(), name, GetCompileTimeCRC32(name));

    #define bbeProfileGPUFunction(gfxContext)                                                                           \
            bbePIXEvent(gfxContext);                                                                                    \
            bbeOnExitScope([] { g_Profiler.SubmitGPULog(); });                                                          \
            g_Profiler.BeginGPURecording(gfxContext.GetCommandList());                                                  \
            MICROPROFILE_SCOPEGPUI_L(g_Profiler.GetGPULogForThread(), __FUNCTION__, GetCompileTimeCRC32(__FUNCTION__));
#else
    #define bbeProfileGPU(gfxContext, name)   bbePIXEvent(gfxContext);
    #define bbeProfileGPUFunction(gfxContext) bbePIXEvent(gfxContext);
#endif