
#define bbeAutoLock(lck) \
    static_assert(std::is_base_of_v<std::_Mutex_base, std::remove_reference_t<decltype(lck)>>); \
    AutoScopeCaller bbeUniqueVariable(ScopedLock){ [&](){ bbeProfileLock(lck); lck.lock(); }, [&](){ lck.unlock(); } };

#define bbeAutoLockRead(lck) \
    static_assert(std::is_same_v<std::shared_mutex, std::remove_reference_t<decltype(lck)>>); \
    AutoScopeCaller bbeUniqueVariable(ScopedReadLock){ [&](){ bbeProfileLock(lck##_LockRead); lck.lock_shared(); }, [&](){ lck.unlock_shared(); } };

#define bbeAutoLockWrite(lck) \
    static_assert(std::is_same_v<std::shared_mutex, std::remove_reference_t<decltype(lck)>>); \
    AutoScopeCaller bbeUniqueVariable(ScopedWriteLock){ [&](){ bbeProfileLock(lck##_LockWrite); lck.lock(); }, [&](){ lck.unlock(); } };

#define bbeAutoLockScopedRWUpgrade(lck) \
    static_assert(std::is_same_v<std::shared_mutex, std::remove_reference_t<decltype(lck)>>); \
    AutoScopeCaller bbeUniqueVariable(ScopedRWLockUpgrade){ [&](){ bbeProfileLock(lck##_LockUpgrade); lck.unlock_shared(); lck.lock(); }, [&](){ lck.unlock(); lck.lock_shared(); } };
