#pragma once

#include <type_traits>

#include "extern/tbb/include/tbb/spin_mutex.h"
#include "extern/tbb/include/tbb/spin_rw_mutex.h"

#include "system/profiler.h"

template <bool ReadWrite>
class SpinLockImp
{
public:
    using LockType = std::conditional_t<ReadWrite, tbb::spin_rw_mutex, tbb::spin_mutex>;

private:
    LockType m_Lock;
};
using SpinLock   = SpinLockImp<false>;
using RWSpinLock = SpinLockImp<true>;

template <bool ReadWrite>
class AdaptiveLockImp : public SpinLockImp<ReadWrite>
{
public:
    void Lock()
    {
        const uint32_t SPIN_LIMIT = 100;

        for (uint32_t i = 0; i < SPIN_LIMIT; ++i)
        {
            // TODO: Spin for fast locks and early exit
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
using AdaptiveLock   = AdaptiveLockImp<false>;
using RWAdaptiveLock = AdaptiveLockImp<true>;

template <typename LockType>
struct ProfiledScopedLock
{
    ProfiledScopedLock(LockType& lck, const char* fileAndLine)
        : m_Lock(lck)
    {
        fileAndLine += std::string{ fileAndLine }.find_last_of('\\');
        bbeProfileBlockBegin(StringFormat("Lock: %s", fileAndLine).c_str());

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
