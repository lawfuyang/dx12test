#pragma once

class SpinLock
{
public:
    void Lock()
    {
        bbeProfileFunction();
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

#define bbeAutoLock(lock) lock.Lock(); bbeOnExitScope {lock.Unlock();}
