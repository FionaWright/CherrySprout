#ifndef H_MUTEX_H
#define H_MUTEX_H

enum MutexState : hlsl::uint
{
    eUnlocked,
    eLocked,
};

bool TryAcquireMutex(uint dbgID)
{
    uint size, _;
    gDbgBufferErrorInfo.GetDimensions(size, _);

    if (dbgID >= size)
        return false;

    hlsl::uint lockState = MutexState::eLocked;
    InterlockedCompareExchange(
        gDbgBufferErrorInfo[dbgID].Lock,
        (hlsl::uint)MutexState::eUnlocked,
        (hlsl::uint)MutexState::eLocked,
        lockState);
    return lockState == MutexState::eUnlocked;
}
bool TryAcquireMutexPixel(uint mutexID)
{
    uint size, _;
    gPerPixelMutex.GetDimensions(size, _);

    if (mutexID >= size)
        return false;

    hlsl::uint lockState = MutexState::eLocked;
    InterlockedCompareExchange(
        gPerPixelMutex[mutexID],
        (hlsl::uint)MutexState::eUnlocked,
        (hlsl::uint)MutexState::eLocked,
        lockState);
    return lockState == MutexState::eUnlocked;
}

void ReleaseMutex(uint dbgID)
{
    uint size, _;
    gDbgBufferErrorInfo.GetDimensions(size, _);

    if (dbgID >= size)
        return;

    uint _i;
    InterlockedExchange(gDbgBufferErrorInfo[dbgID].Lock, (hlsl::uint)MutexState::eUnlocked, _i);
}
void ReleaseMutexPixel(uint mutexID)
{
    uint size, _;
    gPerPixelMutex.GetDimensions(size, _);

    if (mutexID >= size)
        return;

    uint _i;
    InterlockedExchange(gPerPixelMutex[mutexID], (hlsl::uint)MutexState::eUnlocked, _i);
}

#define SPIN_LOCK_MAX_ITER 10
bool TryAcquireSpinLock(uint mutexID)
{
    uint size, _;
    gDbgBufferErrorInfo.GetDimensions(size, _);

    if (mutexID >= size)
        return false;

    uint i = 0;
    while (i < SPIN_LOCK_MAX_ITER)
    {
        if (TryAcquireMutex(mutexID))
            return true;
    }
    return false;
}
bool TryAcquireSpinLockPixel(uint mutexID)
{
    uint size, _;
    gPerPixelMutex.GetDimensions(size, _);

    if (mutexID >= size)
        return false;

    uint i = 0;
    while (i < SPIN_LOCK_MAX_ITER)
    {
        if (TryAcquireMutexPixel(mutexID))
            return true;
    }
    return false;
}

#endif