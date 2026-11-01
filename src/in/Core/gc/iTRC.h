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
    static bool TRCCheck();
};

#define iTRC_CHECK() iTRCDisk::TRCCheck()

#endif
