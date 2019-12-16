#pragma once

#include <cinttypes>
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
#include <mutex>
#include <functional>
#include <wrl.h>

// uncomment to disable all asserts
// #define NDEBUG
#include <cassert>

#include "system/utils.h"
#include "system/math.h"
#include "system/stopwatch.h"
#include "system/logger.h"
#include "system/keyboard.h"
#include "system/mouse.h"
#include "system/profiler.h"
#include "system/locks.h"

#include "extern/cpp-taskflow/taskflow/taskflow.hpp"
#include "extern/boost/container/small_vector.hpp"
#include "extern/boost/lockfree/stack.hpp"
#include "extern/boost/lockfree/queue.hpp"
#include "extern/boost/pool/object_pool.hpp"
#include "extern/boost/container_hash/hash.hpp"

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

    void Initialize();
    void Shutdown();
    void ProcessWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Loop();

    static float GetRealFrameTimeMs()   { return ms_RealFrameTimeMs; }
    static float GetRealFPS()           { return ms_FPS; }
    static float GetCappedFrameTimeMs() { return ms_CappedFrameTimeMs; }
    static float GetCappedFPS()         { return ms_CappedFPS; }

    static uint32_t GetSystemFrameNumber() { return ms_SystemFrameNumber; }

    tf::Executor& GetTasksExecutor() { return m_Executor; }

private:
    void InitializeGraphic();
    void ShutdownGraphic();
    void Update();

    bool m_Exit             = false;
    uint64_t m_LastUpdateMS = 0;

    StopWatch m_Clock;
    inline static float ms_RealFrameTimeMs   = 0.0f;
    inline static float ms_FPS               = 0.0f;
    inline static float ms_CappedFrameTimeMs = 0.0f;
    inline static float ms_CappedFPS         = 0.0f;

    tf::Executor m_Executor;

    inline static uint32_t ms_SystemFrameNumber = 0;

    friend class FrameRateController;
};

class FrameRateController
{
public:
    FrameRateController();
    ~FrameRateController();

private:
    StopWatch m_StopWatch;

    std::chrono::high_resolution_clock::time_point m_FrameEndTime;
    std::chrono::high_resolution_clock::time_point m_200FPSFrameEndTime;
};
