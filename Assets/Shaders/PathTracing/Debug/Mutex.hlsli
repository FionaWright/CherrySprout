#ifndef H_MUTEX_H
#define H_MUTEX_H

enum MutexState : hlsl::uint
{
    eUnlocked,
    eLocked,
};

bool TryAcquireMutex(uint dbgID)
{
    hlsl::uint lockState = MutexState::eLocked;
    InterlockedCompareExchange(
        gDbgBufferErrorInfo[dbgID].Lock,
        (hlsl::uint)MutexState::eUnlocked,
        (hlsl::uint)MutexState::eLocked,
        lockState);
    return lockState == MutexState::eUnlocked;
}

void ReleaseMutex(uint dbgID)
{
    uint _i;
    InterlockedExchange(gDbgBufferErrorInfo[dbgID].Lock, (hlsl::uint)MutexState::eUnlocked, _i);
}

#endif