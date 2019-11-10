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

// Keep these 2 headers on top to prevent the build from exploding with warnings
#include "system/win64includes.h"

#include "system/utils.h"
#include "system/math.h"
#include "system/stopwatch.h"
#include "system/logger.h"
#include "system/keyboard.h"
#include "system/mouse.h"
#include "system/profiler.h"

typedef uint64_t WindowHandle;

class System
{
    DeclareSingletonFunctions(System);

public:

    static inline const std::string APP_NAME = "Vulkan Test";
    static const uint32_t APP_WINDOW_WIDTH   = 1600;
    static const uint32_t APP_WINDOW_HEIGHT  = 900;
    static const uint32_t FPS_LIMIT          = 60;

    // Gfx Debug Layer
    static const bool EnableGfxDebugLayer                             = true;
    static const bool GfxDebugLayerBreakOnWarnings                    = true;
    static const bool GfxDebugLayerBreakOnErrors                      = true;
    static const bool GfxDebugLayerLogVerbose                         = true;
    static const bool GfxDebugLayerEnableGPUValidation                = true;
    static const bool GfxDebugLayerSynchronizedCommandQueueValidation = true;
    static const bool GfxDebugLayerDisableStateTracking               = false;
    static const bool EnableRenderDocCapture                          = false;

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

// super simple lightweight ComPtr implementation
template <typename T>
class AutoComPtr
{
public:
    explicit AutoComPtr(T* ptr) : m_Ptr(ptr) {}

    ~AutoComPtr()
    {
        m_Ptr->Release();
    }

    T* operator->() const { return m_Ptr; }

private:
    T* m_Ptr;
};

template <typename T>
const AutoComPtr<T> MakeAutoComPtr(T* ptr) { return AutoComPtr<T>{ptr}; }
