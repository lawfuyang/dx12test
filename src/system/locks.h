#pragma once

#include "system/profiler.h"

template <typename LockType>
struct ProfiledScopedLock
{
    ProfiledScopedLock(LockType& lock, const char* mtxName)
        : m_Lock(lock)
        , m_ProfileToken(MicroProfileGetToken("Locks", mtxName, 0xFF0000, MicroProfileTokenTypeCpu))
    {
        MICROPROFILE_ENTER_TOKEN(m_ProfileToken);
        m_Lock.lock();
    }

    ~ProfiledScopedLock()
    {
        m_Lock.unlock();
        bbeProfileBlockEnd();
    }

    LockType& m_Lock;
    MicroProfileToken m_ProfileToken = {};
};

#define bbeAutoLock(lock) const ProfiledScopedLock bbeUniqueVariable(scopedLock){ lock, bbeTOSTRING(lock) };
