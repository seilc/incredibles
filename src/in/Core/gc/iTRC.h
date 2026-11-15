#ifndef ITRC_H
#define ITRC_H

#include "types.h"

typedef enum _tagPadInit
{
    ePadInit_Open1,
    ePadInit_WaitStable2,
    ePadInit_EnableAnalog3,
    ePadInit_EnableAnalog3LetsAllPissOffChris,
    ePadInit_EnableRumble4,
    ePadInit_EnableRumbleTest5,
    ePadInit_PressureS6,
    ePadInit_PressureSTest7,
    ePadInit_Complete8a,
    ePadInit_Complete8b,
    ePadInit_Finished9
} ePadInit;

typedef struct _tagiTRCPadInfo
{
    ePadInit pad_init;
} iTRCPadInfo;

typedef void(*ResetButtonCallback)();

class ResetButton
{
public:
    static void EnableReset();
    static void DisableReset();
    static void SetDiscIded(bool value);
    static void SetSndKillFunction(ResetButtonCallback sndKillFunction);
    static void SetMovieKillFunction(ResetButtonCallback movieKillFunction);
    static void CheckResetButton();
    static void ResetSystem();

    static void (*mSndKill)();
    static void (*mMovieKillFunction)();
    static S32 mWasResetButtonPressed;
    static bool mResetEnabled;
    static bool mDiscIded;
};

typedef void(*iTRCDiskCallback)();

class iTRCDisk
{
public:
    static void Init();
    static void SetErrorMessage(const char* errorMessage);
    static void ResetMessage();
    static void SetPadStopRumblingFunction(iTRCDiskCallback padStopRumbing);
    static void SetSndSuspendFunction(iTRCDiskCallback sndSuspendFunction);
    static void SetSndResumeFunction(iTRCDiskCallback sndResumeFunction);
    static void SetAsyncLoadSuspendFunction(iTRCDiskCallback asyncLoadSuspendFunction);
    static void SetAsyncLoadResumeFunction(iTRCDiskCallback asyncLoadResumeFunction);
    static void SetMovieSuspendFunction(iTRCDiskCallback movieSuspendFunction);
    static void SetMovieResumeFunction(iTRCDiskCallback movieResumeFunction);
    static void SetMovieResetCallBack(iTRCDiskCallback movieResetCallBack);
    static bool IsDiskIDed();
    static void SetInFModLockingCode(bool value);
    static void Shutdown();
    static void DisplayErrorMessage();
    static S32 SetDVDState();
    static bool CheckDVDAndResetState();
    static bool TRCCheck();
    static void FMODCallBack(S32 status);
    static bool CheckDiskErrorInHigherPriorityThread();
    static void FMODUpdateDiskError(S32);
    static void EnableMovieResetCallBack(bool enable);

    static char mMessage[256];
    static bool mInCriticalSection;
    static bool mFModError;
    static bool mDiskErrorInHigherPriorityThread;
    static OSThread* mMainThread;
    static s32 mMainPriority;
    static OSThread* mFModThread;
    static s32 mFModPriority;
    static bool mInFModLockingCode;
    static void (*mSndSuspend)();
    static void (*mSndResume)();
    static void (*mAsyncLoadSuspend)();
    static void (*mAsyncLoadResume)();
    static void (*mPadStopRumbling)();
    static void (*mMovieSuspendFunction)();
    static void (*mMovieResumeFunction)();
    static void (*mMovieResetCallBack)();
};

#define iTRC_CHECK() iTRCDisk::TRCCheck()

extern S32 gWasDiskError;

#endif
