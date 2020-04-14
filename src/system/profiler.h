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
    void RegisterThread(const char* name);
    void ShutDown();
    void OnFlip();
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
    InplaceArray<MicroProfileToken, 4> m_Tokens;

    friend struct ScopedGPUProfileHandler;
};

struct ScopedGPUProfileHandler
{
    ScopedGPUProfileHandler(MicroProfileToken& token, GfxContext& gfxContext);
    ~ScopedGPUProfileHandler();

    GfxContext& m_GfxContext;
    MicroProfileToken& m_Token;
};

#define bbeDefineProfilerToken(var, group, name, color) MICROPROFILE_DEFINE(var, group, name, color)
#define bbeProfile(str)                                 MICROPROFILE_SCOPEI("", str, GetCompileTimeCRC32(str))
#define bbeProfileToken(token)                          MICROPROFILE_SCOPE(token)
#define bbeProfileFunction()                            MICROPROFILE_SCOPEI("", __FUNCTION__, GetCompileTimeCRC32(__FUNCTION__))
#define bbeConditionalProfile(condition, name)          MICROPROFILE_CONDITIONAL_SCOPEI(condition, name, name, GetCompileTimeCRC32(name))
#define bbeProfileBlockBegin(token)                     MICROPROFILE_ENTER(token)
#define bbeProfileBlockEnd()                            MICROPROFILE_LEAVE()
#define bbePIXEvent(gfxContext)                         const ScopedPixEvent bbeUniqueVariable(pixEvent) { gfxContext.GetCommandList().Dev() }

#define bbeDefineGPUProfilerToken(name) static MicroProfileToken gs_GPUProfilerToken_##name = MicroProfileGetToken("GPU", bbeTOSTRING(name), GetCompileTimeCRC32(bbeTOSTRING(name)), MicroProfileTokenTypeGpu)
#define bbeGPUProfileToken(name) gs_GPUProfilerToken_##name

#define bbeProfileGPU(gfxContext, token)                                                     \
    bbePIXEvent(gfxContext);                                                                 \
    const ScopedGPUProfileHandler bbeUniqueVariable(scopedGPUProfiler) { token, gfxContext };
