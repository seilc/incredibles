#ifndef ITRC_H
#define ITRC_H

#include "types.h"

enum _tagPadInit
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
};

struct _tagiTRCPadInfo
{
    _tagPadInit pad_init;
};

class iTRCDisk
{
public:
#if DEBUG || RELEASE
    static bool TRCCheck(const char* file, S32 line);
#else
    static bool TRCCheck();
#endif
};

#if DEBUG || RELEASE
#define iTRC_CHECK_L(line) iTRCDisk::TRCCheck(__FUNCTION__, (line))
#else
#define iTRC_CHECK_L(line) iTRCDisk::TRCCheck()
#endif

#define iTRC_CHECK() iTRC_CHECK_L(__LINE__)

#endif
