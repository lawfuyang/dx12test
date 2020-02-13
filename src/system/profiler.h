#pragma once

#define MICROPROFILE_ENABLED 1
#define MICROPROFILE_GPU_TIMERS_D3D12 1
#define MICROPROFILE_WEBSERVER_MAXFRAMES 100
#include "extern/microprofile/microprofile.h"

#include "system/utils.h"

class SystemProfiler
{
public:
    DeclareSingletonFunctions(SystemProfiler);

    void Initialize();
    void InitializeGPUProfiler(void* pDevice, void* pCommandQueue);
    void ShutDown();
    void OnFlip();
    void DumpProfilerBlocks(bool condition, bool immediately = false, bool ignoreCD = false);
};
#define g_Profiler SystemProfiler::GetInstance()

#define bbeProfile(str)                        MICROPROFILE_SCOPEI("", str, (GetCompileTimeCRC32(str) | 0XFF000000))
#define bbeProfileFunction()                   MICROPROFILE_SCOPEI("", __FUNCTION__, (GetCompileTimeCRC32(__FUNCTION__) | 0XFF000000))
#define bbeConditionalProfile(condition, name) MICROPROFILE_CONDITIONAL_SCOPEI(condition, name, name, (GetCompileTimeCRC32(name) | 0XFF000000))
#define bbeProfileBlockBegin(str)              MICROPROFILE_ENTERI("", str, (GetCompileTimeCRC32(str) | 0XFF000000))
#define bbeProfileBlockEnd()                   MicroProfileLeave()
