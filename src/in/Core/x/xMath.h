#ifndef XMATH_H
#define XMATH_H

#include <types.h>

#include <cmath>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define xfeq0(x) (((x) >= -EPSILON) && ((x) <= EPSILON))
#define xmin(a,b) ((a) < (b) ? (a) : (b))
#define xmax(a,b) ((a) > (b) ? (a) : (b))
#define xclamp(x,a,b) xmax((a), xmin((x), (b)))
#define xsqr(x) ((x) * (x))

#define SQR(x) ((x) * (x))

#define ALIGN(x, a) ((x) + ((a)-1) & ~((a)-1))

#define PI 3.14159265f

#define INVERSEPI (1.0f / (PI))
#define TWOPI (2.0f * (PI))

// To use DEG2RAD/RAD2DEG with extern'd .sdata2 symbols,
// define SDATA2_PI and SDATA2_180 at the top of the .cpp file,
// BEFORE any includes
//
// Example:
//
//     #define SDATA2_PI _771_1
//     #define SDATA2_180 _778_0
//
//     #include "xMath.h"
//
//     extern F64 _771_1;
//     extern F64 _778_0;

#ifndef SDATA2_PI
#define SDATA2_PI 3.14159265359
#endif

#ifndef SDATA2_180
#define SDATA2_180 180.0
#endif

#define DEG2RAD(x) (F32)((SDATA2_PI) * (x) / (SDATA2_180))
#define RAD2DEG(x) (F32)((SDATA2_180) * (x) / (SDATA2_PI))

F32 xatof(const char* x);
F32 xsqrt(F32 x);
F32 xexp(F32 x);
F32 xasin(F32 x);
F32 xatan2(F32 y, F32 x);

inline F32 xfmod(F32 x, F32 y)
{
    return std::fmodf(x, y);
}

inline void xsqrtfast(F32& out, register F32 x)
{
    if (x <= 0.0f)
    {
        out = 0.0f;
    }
    else
    {
        // Have to use inline asm here, for some reason using
        // __frsqrte and __fres generates an extra frsp instruction
        asm {
            frsqrte x, x;
            fres x, x;
        }
    }

    out = x;
}

#endif