#include "system/timer.h"

struct StepTimerStaticsInitializer
{
    StepTimerStaticsInitializer()
    {
        QueryPerformanceFrequency(&Timer::ms_QPCFrequency);
    }
};
static StepTimerStaticsInitializer g_StepTimerStaticsInitializer;

uint64_t Timer::Tick()
{
    // Query the current time.
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    uint64_t timeDelta = currentTime.QuadPart - m_QPCStartTime.QuadPart;

    m_QPCStartTime = currentTime;

    // Convert QPC units into a canonical tick format
    timeDelta *= TicksPerSecond;
    timeDelta /= ms_QPCFrequency.QuadPart;

    // Variable timestep update logic.
    m_ElapsedTicks += timeDelta;
    return timeDelta;
}
