#ifndef ITIME_H
#define ITIME_H

#include "types.h"

#include <dolphin.h>

typedef OSTime iTime;

#define iTimeFromSec(sec) ((iTime)OSSecondsToTicks(sec))

void iTimeInit();
void iTimeExit();
iTime iTimeGet();
F32 iTimeDiffSec(iTime time);
F32 iTimeDiffSec(iTime t0, iTime t1);
F32 iTimeGetGame();
void iTimeGameAdvance(F32 elapsed);
void iTimeSetGame(F32 time);
void iProfileClear(U32 sceneID);
void iFuncProfileDump();

#endif
