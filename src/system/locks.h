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

private:
    bool TryLock()
    {
        return !m_Lock.test_and_set(std::memory_order_acquire);
    }

    std::atomic_flag m_Lock = ATOMIC_FLAG_INIT;
};

#define bbeAutoLock(lock) lock.Lock(); bbeOnExitScope {lock.Unlock();}
