#ifndef ITIME_H
#define ITIME_H

#include "types.h"

#include <dolphin.h>

typedef OSTime iTime;

#define iTimeFromSec(sec) ((iTime)OSSecondsToTicks(sec))

iTime iTimeGet();
F32 iTimeDiffSec(iTime time);
F32 iTimeDiffSec(iTime t0, iTime t1);
F32 iTimeGetGame();

#endif
