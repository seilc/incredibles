#ifndef IPAD_H
#define IPAD_H

#include "types.h"

typedef struct _tagxPad xPad;

typedef struct _tagiPad
{
    S32 port;
} iPad;

S32 iPadInit();
xPad* iPadEnable(xPad* pad, S16 port);
S32 iPadUpdate(xPad* pad, U32* on);
void iPadStopRumble(xPad* pad);
void iPadStopRumble();
void iPadStartRumble(xPad* pad, F32 intensity);
void iPadUpdateRumble(xPad* pad);
void iPadKill();

#endif
