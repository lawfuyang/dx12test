#pragma once

#include <chrono>

class StopWatch
{
public:
    StopWatch()
        : m_StartTime(std::chrono::high_resolution_clock::now())
    { }

    uint64_t ElapsedMS() const { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_StartTime).count(); }
    uint64_t ElapsedUS() const { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_StartTime).count(); }
    uint64_t ElapsedNS() const { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_StartTime).count(); }

    auto GetStartTime() const { return m_StartTime; }
    void Start() { m_StartTime = std::chrono::high_resolution_clock::now(); }
    void Restart() { Start(); }
private:
    std::chrono::high_resolution_clock::time_point m_StartTime;
};
