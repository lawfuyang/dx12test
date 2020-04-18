#pragma once

// Helper class for animation and simulation timing.
class Timer
{
public:
    Timer() { Reset(); }

    void Reset() 
    {
        QueryPerformanceCounter(&m_QPCStartTime);
        m_ElapsedTicks = 0;
    }

    // Get elapsed time since the previous Update call.
    uint64_t GetElapsedTicks() { Tick(); return m_ElapsedTicks; }
    double GetElapsedSeconds() { return TicksToSeconds(GetElapsedTicks()); }
    double GetElapsedMicroSeconds() { return TicksToMilliSeconds(GetElapsedTicks()); }

    // Integer format represents time using 10,000,000 ticks per second.
    static const uint64_t TicksPerSecond = 10000000;

    constexpr static double TicksToSeconds(uint64_t ticks)      { return static_cast<double>(ticks) / TicksPerSecond; }
    constexpr static double TicksToMilliSeconds(uint64_t ticks) { return TicksToSeconds(ticks) * 1000; }
    constexpr static double TicksToMicroSeconds(uint64_t ticks) { return TicksToMilliSeconds(ticks) * 1000; }

    constexpr static uint64_t SecondsToTicks(double seconds)    { return static_cast<uint64_t>(seconds * TicksPerSecond); }
    constexpr static uint64_t MilliSecondsToTicks(double ms)    { return SecondsToTicks(ms / 1000); }
    constexpr static uint64_t MicroSecondsToTicks(double us)    { return MilliSecondsToTicks(us / 1000); }

    // Update timer state
    uint64_t Tick();

protected:

    // Source timing data uses QPC units.
    inline static LARGE_INTEGER ms_QPCFrequency;
    LARGE_INTEGER               m_QPCStartTime;

    // Derived timing data uses a canonical tick format.
    uint64_t m_ElapsedTicks = 0;

    friend struct StepTimerStaticsInitializer;
};
