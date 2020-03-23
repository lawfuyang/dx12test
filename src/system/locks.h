#pragma once

#include <type_traits>

#include "extern/tbb/tbb/spin_mutex.h"

#include "system/profiler.h"

class SpinLock
{
public:
    SpinLock(const char* name)
        : m_Name(name)
    {}

    void Lock()
    {
        m_SpinLock.lock();
    }

    bool TryLock()
    {
        return m_SpinLock.try_lock();
    }

    void Unlock()
    {
        m_SpinLock.unlock();
    }

    const char* GetName() const { return m_Name; }

private:
    tbb::spin_mutex m_SpinLock;
    const char* m_Name = "";
};

class AdaptiveLock : public SpinLock
{
public:
    AdaptiveLock(const char* name)
        : SpinLock(name)
    {}

    // SpinLock prediction by Foster Brereton: https://hackernoon.com/building-a-c-hybrid-spin-mutex-f98de535b4ac
    void Lock()
    {
        using clock_t = std::chrono::high_resolution_clock;

        const auto startTime = clock_t::now();
        std::size_t spinTime = 0;
        while (!SpinLock::TryLock())
        {
            spinTime = (clock_t::now() - startTime).count();
            if (spinTime >= m_SpinPredication * 2)
            {
                m_Mutex.lock();
                m_MutexLocked = true;
                break;
            }
        }

        m_SpinPredication += (spinTime - m_SpinPredication) / 8;
    }

    void Unlock()
    {
        if (m_MutexLocked)
        {
            m_Mutex.unlock();
            m_MutexLocked = false;
        }
        else
        {
            SpinLock::Unlock();
        }
    }

private:
    std::mutex m_Mutex;

    std::size_t m_SpinPredication = 0;
    bool m_MutexLocked = false;
};

template <typename LockType>
struct ProfiledScopedLock
{
    ProfiledScopedLock(LockType& lck)
        : m_Lock(lck)
    {
        bbeProfileBlockBegin(ScopedLock);

        m_Lock.Lock();
    }

    ~ProfiledScopedLock()
    {
        m_Lock.Unlock();

        bbeProfileBlockEnd();
    }

    LockType& m_Lock;
    bbeDefineProfilerToken(ScopedLock, "Locks", m_Lock.GetName(), 0xFF0000);
};

#define bbeAutoLock(lock) const ProfiledScopedLock<decltype(lock)> bbeUniqueVariable(scopedLock){ lock };
