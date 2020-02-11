#pragma once

#define MICROPROFILE_ENABLED 1
#define MICROPROFILE_MAX_FRAME_HISTORY 60
#define MICROPROFILE_GPU_TIMERS_D3D12 1
#include "extern/microprofile/microprofile.h"

#include "system/utils.h"

class SystemProfiler
{
public:
    DeclareSingletonFunctions(SystemProfiler);

    void Initialize();
    void ShutDown();
    void Flip();
    void DumpProfilerBlocks();
};
#define g_Profiler SystemProfiler::GetInstance()

//#define bbeProfile(str)           EASY_BLOCK(str, (profiler::color_t)(GetCompileTimeCRC32(str) | 0XFF000000))
//#define bbeProfileFunction()      EASY_BLOCK(EASY_FUNC_NAME, (profiler::color_t)(GetCompileTimeCRC32(EASY_FUNC_NAME) | 0XFF000000))
//#define bbeProfileBlockBegin(str) EASY_NONSCOPED_BLOCK(str, profiler::colors::Red);
//#define bbeProfileBlockEnd()      EASY_END_BLOCK;

#define bbeProfile(str)           MICROPROFILE_SCOPEI("CPU", str, (GetCompileTimeCRC32(str) | 0XFF000000))
#define bbeProfileFunction()      MICROPROFILE_SCOPEI("CPU", __FUNCTION__, (GetCompileTimeCRC32(__FUNCTION__) | 0XFF000000))
#define bbeProfileBlockBegin(str) ((void)0)
#define bbeProfileBlockEnd()      ((void)0)
