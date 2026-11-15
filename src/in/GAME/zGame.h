#ifndef ZGAME_H
#define ZGAME_H

#include "types.h"

extern F32 gRealTimeElapsed;
extern U32 startPressed;
extern U32 gSoak;

S32 zGameIsPaused();
void zGameDemoText(F32 fTime);

#endif