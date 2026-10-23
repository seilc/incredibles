#ifndef XCOLOR_H
#define XCOLOR_H

#include <types.h>
#include <rwcore.h>

typedef union xColor_tag
{
    struct
    {
        U8 r, g, b, a;
    };

    RwRGBA rgba;
} xColor;

typedef struct _xFColor
{
    F32 r;
    F32 g;
    F32 b;
    F32 a;
} xFColor;

extern const xColor g_RED;
extern const xColor g_BLUE;
extern const xColor g_YELLOW;
extern const xColor g_BLACK;
extern const xColor g_WHITE;
extern const xColor g_CLEAR;
extern const xColor g_PIMP_GOLD;
extern const xColor g_CHARTREUSE;

inline void xColorInit(xColor& rToInit, U8 r, U8 g, U8 b, U8 a)
{
    rToInit.r = r;
    rToInit.g = g;
    rToInit.b = b;
    rToInit.a = a;
}

#endif