#pragma once

#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
#include <ctime>
#include <algorithm>
#include <array>
#include <numeric>
#include <wrl.h>

#include "system/utils.h"
#include "system/math.h"
#include "system/stopwatch.h"
#include "system/logger.h"
#include "system/keyboard.h"
#include "system/mouse.h"
#include "system/profiler.h"
#include "system/locks.h"

#include "extern/taskflow/taskflow.hpp"

typedef uint64_t WindowHandle;
using Microsoft::WRL::ComPtr;

class System
{
    DeclareSingletonFunctions(System);

public:

    static inline const std::string APP_NAME = "DX12 Test";
    static const uint32_t APP_WINDOW_WIDTH   = 1600;
    static const uint32_t APP_WINDOW_HEIGHT  = 900;
    static const uint32_t FPS_LIMIT          = 60;

    // Gfx Debug Layer
    static const bool EnableGfxDebugLayer                                 = true;
    static const bool GfxDebugLayerBreakOnWarnings                        = EnableGfxDebugLayer && true;
    static const bool GfxDebugLayerBreakOnErrors                          = EnableGfxDebugLayer && true;
    static const bool GfxDebugLayerLogVerbose                             = EnableGfxDebugLayer && true;
    static const bool GfxDebugLayerEnableGPUValidation                    = EnableGfxDebugLayer && true;
    static const bool GfxDebugLayerSynchronizedCommandQueueValidation     = EnableGfxDebugLayer && true;
    static const bool GfxDebugLayerDisableStateTracking                   = EnableGfxDebugLayer && false;
    static const bool GfxDebugLayerEnableConservativeResorceStateTracking = EnableGfxDebugLayer && true;

    void Initialize();
    void Shutdown();
    void ProcessWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Loop();

    static float GetRealFrameTimeMs()   { return ms_AvgRealFrameTimeMs; }
    static float GetRealFPS()           { return ms_AvgFPS; }
    static float GetCappedFrameTimeMs() { return ms_AvgCappedFrameTimeMs; }
    static float GetCappedFPS()         { return ms_AvgCappedFPS; }

private:
    void InitializeGraphic();
    void ShutdownGraphic();
    void Update();

    bool m_Exit             = false;
    uint64_t m_LastUpdateMS = 0;

    StopWatch m_Clock;
    inline static float ms_LastFrameTimeMs      = 0.0f;
    inline static float ms_AvgRealFrameTimeMs   = 0.0f;
    inline static float ms_AvgFPS               = 0.0f;
    inline static float ms_AvgCappedFrameTimeMs = 0.0f;
    inline static float ms_AvgCappedFPS         = 0.0f;

    tf::Executor m_Executor;

    friend class FrameRateController;
};

class FrameRateController
{
public:
    FrameRateController();
    ~FrameRateController();

private:
    static constexpr uint32_t NUM_AVG_FRAMES = 10;
    using FpsArray = uint64_t[NUM_AVG_FRAMES];

    static void UpdateAvgFrameTimeInternal(FpsArray& array, uint8_t& arrCursor, uint64_t elapsedUs, float& avgFrameTime, float& avgFPS);

    static void UpdateAvgRealFrameTime(uint64_t elapsedUS)
    {
        UpdateAvgFrameTimeInternal(ms_AvgRealFrameTimeUsArray, ms_AvgRealFrameIdxCursor, elapsedUS, ms_AvgRealFrameTimeMs, ms_AvgFPS);
    }

    static void UpdateAvgCappedFrameTime(uint64_t elapsedUS)
    {
        UpdateAvgFrameTimeInternal(ms_AvgCappedFrameTimeUsArray, ms_AvgCappedFrameIdxCursor, elapsedUS, ms_AvgCappedFrameTimeMs, ms_AvgCappedFPS);
    }

    StopWatch m_StopWatch;
    inline static ::HANDLE ms_TimerHandle = ::CreateWaitableTimer(0, TRUE, "FrameRateController Timer");

    std::chrono::high_resolution_clock::time_point m_FrameEndTime;

    inline static float ms_AvgRealFrameTimeMs   = 0.0f;
    inline static float ms_AvgFPS               = 0.0f;
    inline static float ms_AvgCappedFrameTimeMs = 0.0f;
    inline static float ms_AvgCappedFPS         = 0.0f;

    inline static FpsArray ms_AvgRealFrameTimeUsArray   = {};
    inline static FpsArray ms_AvgCappedFrameTimeUsArray = {};
    inline static uint8_t ms_AvgRealFrameIdxCursor      = 0;
    inline static uint8_t ms_AvgCappedFrameIdxCursor    = 0;
};

// General purpose ParallelFor with iterators input
template <typename BeginIter, typename EndIter, typename Lambda>
static void ParallelFor(BeginIter begin, EndIter end, Lambda&& lambda)
{
    bbeProfileFunction();

    tf::Taskflow tf;
    tf.parallel_for(begin, end, lambda);
    System::GetInstance().m_Executor.run(tf).wait();
}

// General purpose ParallelFor with containers input
template <typename ContainerType, typename Lambda>
static void ParallelFor(const ContainerType& container, Lambda&& lambda)
{
    ParallelFor(container.cbegin(), container.cend(), std::forward<Lambda>(lambda));
}

// General purpose ParallelFor with const containers input
template <typename ContainerType, typename Lambda>
static void ParallelFor(ContainerType& container, Lambda&& lambda)
{
    ParallelFor(const_cast<const ContainerType&>(container), std::forward<Lambda>(lambda));
}

// General purpose ParallelFor with C-Style array input
template <typename ArrayType, uint32_t ArraySize, typename Lambda>
static void ParallelFor(ArrayType(&arr)[ArraySize], Lambda&& lambda)
{
    ParallelFor(arr, arr + ArraySize, std::forward<Lambda>(lambda));
}
