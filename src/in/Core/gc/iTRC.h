#ifndef ITRC_H
#define ITRC_H

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

typedef void(*iTRCDiskCallback)();

class iTRCDisk
{
public:
    static void Init();
    static void SetPadStopRumblingFunction(iTRCDiskCallback padStopRumbing);
    static void SetSndSuspendFunction(iTRCDiskCallback sndSuspendFunction);
    static void SetSndResumeFunction(iTRCDiskCallback sndResumeFunction);
    static void SetAsyncLoadSuspendFunction(iTRCDiskCallback asyncLoadSuspendFunction);
    static void SetAsyncLoadResumeFunction(iTRCDiskCallback asyncLoadResumeFunction);
    static void SetMovieSuspendFunction(iTRCDiskCallback movieSuspendFunction);
    static void SetMovieResumeFunction(iTRCDiskCallback movieResumeFunction);
    static bool TRCCheck();
};

#define iTRC_CHECK() iTRCDisk::TRCCheck()

typedef void(*ResetButtonCallback)();

class ResetButton
{
public:
    static void SetSndKillFunction(ResetButtonCallback sndKillFunction);
    static void SetMovieKillFunction(ResetButtonCallback movieKillFunction);
    static void ResetSystem();
};

#endif
