#ifndef XPAD_H
#define XPAD_H

#include "iPad.h"
#include "xMath2.h"

typedef struct _tagPadAnalog
{
    S8 x;
    S8 y;
} PadAnalog;

typedef enum _tagPadState
{
    ePad_Disabled,
    ePad_DisabledError,
    ePad_Enabled,
    ePad_Missing,
    ePad_Total
} PadState;

typedef struct _tagxPad
{
    struct analog_data
    {
        xVec2 offset;
        xVec2 dir;
        F32 mag;
        F32 ang;
    };

    U8 value[22];
    U8 last_value[22];
    U32 on;
    U32 pressed;
    U32 released;
    PadAnalog analog1;
    PadAnalog analog2;
    PadState state;
    U32 flags;
    S16 port;
    S16 slot;
    iPad context;
    F32 al2d_timer;
    F32 ar2d_timer;
    F32 d_timer;
    F32 up_tmr[22];
    F32 down_tmr[22];
    analog_data analog[2];
} xPad;

S32 xPadUpdate(S32 idx, F32 time_passed);

#endif