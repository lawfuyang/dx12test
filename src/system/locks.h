#pragma once

#include <atomic>

#include "system/profiler.h"

class SpinLock
{
public:
    void Lock()
    {
        while (!TryLock()) {}
    }

    void Unlock()
    {
        m_Lock.clear(std::memory_order_release);
    }

protected:
    bool TryLock()
    {
        return !m_Lock.test_and_set(std::memory_order_acquire);
    }

    std::atomic_flag m_Lock = ATOMIC_FLAG_INIT;
};

class AdaptiveLock : public SpinLock
{
public:
    void Lock()
    {
        const uint32_t SPIN_LIMIT = 100;

        for (uint32_t i = 0; i < SPIN_LIMIT; ++i)
        {
            if (TryLock()) return;
        }

        m_Mutex.lock();
        m_IsLocked = true;
    }

    void Unlock()
    {
        if (m_IsLocked)
        {
            m_Mutex.unlock();
            m_IsLocked = false;
        }
    }

private:
    std::mutex m_Mutex;
    bool m_IsLocked = false;
};

template <typename LockType>
struct ProfiledScopedLock
{
    ProfiledScopedLock(LockType& lck, const char* fileAndLine)
        : m_Lock(lck)
    {
        char fileAndLineBuffer[MAX_PATH] = {};
        sprintf(fileAndLineBuffer, "Lock: %s", fileAndLine);
        bbeProfileBlockBegin(fileAndLineBuffer);

        m_Lock.Lock();
    }

    ~ProfiledScopedLock()
    {
        m_Lock.Unlock();

        bbeProfileBlockEnd();
    }

    LockType& m_Lock;
};

#define bbeAutoLock(lock) const ProfiledScopedLock<decltype(lock)> bbeUniqueVariable(scopedLock){ lock, bbeFILEandLINE };
