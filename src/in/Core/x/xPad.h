#ifndef XPAD_H
#define XPAD_H

#include "iPad.h"
#include "xMath2.h"

#define k_XPAD_MAX 4
#define k_XPAD_BUTTON_MAX 22
#define k_XPAD_ANALOG_MAX 2

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

    U8 value[k_XPAD_BUTTON_MAX];
    U8 last_value[k_XPAD_BUTTON_MAX];
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
    F32 up_tmr[k_XPAD_BUTTON_MAX];
    F32 down_tmr[k_XPAD_BUTTON_MAX];
    analog_data analog[k_XPAD_ANALOG_MAX];
} xPad;

extern xPad mPad[k_XPAD_MAX];

S32 xPadInit();
xPad* xPadEnable(S32 idx);
void xPadRumbleEnable(S32 idx, S32 enable);
S32 xPadUpdate(S32 idx, F32 time_passed);
void xPadNormalizeAnalog(xPad& pad, S32 inner_zone, S32 outer_zone);
void xPadKill();
void xPadAnalogIsDigital(S32 idx, S32 enable);

inline F32 normalize_analog(S32 v, S32 v_min, S32 v_max, S32 dead_center, S32 dead_min, S32 dead_max)
{
    if (v <= dead_center) {
        if (v >= dead_min) return 0.0f;
        else if (v <= v_min) return -1.0f;
        else return (F32)(v - dead_min) / (dead_min - v_min);
    } else {
        if (v <= dead_max) return 0.0f;
        else if (v >= v_max) return 1.0f;
        else return (F32)(v - dead_max) / (v_max - dead_max);
    }
}

#endif
