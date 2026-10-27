#ifndef XMATH_H
#define XMATH_H

#include "types.h"

#include <cmath>

#define xabs(x) iabs(x)
#define xeq(a,b,e) (xabs((a) - (b)) <= (e))
#define xfeq0(x) (((x) >= -EPSILON) && ((x) <= EPSILON))
#define xmin(a,b) ((a) < (b) ? (a) : (b))
#define xmax(a,b) ((a) > (b) ? (a) : (b))
#define xmax3(a,b,c) (xmax(xmax((a), (b)), (c)))
#define xclamp(x,a,b) xmax((a), xmin((x), (b)))
#define xsqr(x) ((x) * (x))

#define PI 3.14159265f
#define HUGE 1e38f

F32 xatof(const char* x);
F32 xsqrt(F32 x);
F32 xexp(F32 x);
F32 xasin(F32 x);
F32 xatan2(F32 y, F32 x);

inline F32 xfmod(F32 x, F32 y)
{
    return std::fmodf(x, y);
}

inline F32 xrmod(F32 ang)
{
    F32 frac = (1.0f / (2.0f * PI)) * ang;
    if (frac < 0.0f) return 2.0f * PI * (1.0f + (frac - std::ceilf(frac)));
    else if (frac >= 1.0f) return 2.0f * PI * (frac - std::floorf(frac));
    return ang;
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