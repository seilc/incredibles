#ifndef ITRC_H
#define ITRC_H

#include "types.h"

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
