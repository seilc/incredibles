#ifndef XMATHINLINES_H
#define XMATHINLINES_H

inline F32 xsqrt(register F32 x)
{
    register F32 c_half = 0.5f;
    register F32 c_three = 3.0f;
    register F32 rsqrt;
    register F32 nwork0;
    register F32 nwork1;

    if (x <= 0.0f || isinf(x)) {
        return x;
    }

    asm {
        frsqrte rsqrt, x
        fmuls nwork0, rsqrt, rsqrt
        fmuls nwork1, rsqrt, c_half
        fnmsubs nwork0, nwork0, x, c_three
        fmuls rsqrt, nwork0, nwork1
    }

    if (rsqrt > 0.00001f) {
        return 1.0f / rsqrt;
    }

    return 100000.0f;
}

inline F32 xlog(F32 x)
{
    return std::logf(x);
}

inline F32 xexp(F32 x)
{
    return std::expf(x);
}

inline F32 xfmod(F32 x, F32 y)
{
    return std::fmodf(x, y);
}

inline F32 xpow(F32 x, F32 y)
{
    return std::powf(x, y);
}

inline F32 xacos(F32 x)
{
    if (x > 1.0f) {
        x = 1.0f;
    }

    if (x < -1.0f) {
        x = -1.0f;
    }

    return std::acosf(x);
}

inline F32 xasin(F32 x)
{
    if (x > 1.0f) {
        x = 1.0f;
    }

    if (x < -1.0f) {
        x = -1.0f;
    }

    return std::asinf(x);
}

inline void xsqrtfast(F32& o, register F32 fVal)
{
    register F32 fRetVal;

    if (fVal <= 0.0f)
        o = 0.0f;
    else
        asm {
            frsqrte fRetVal, fVal
            fres fRetVal, fRetVal
        }

    o = fRetVal;
}

inline void xsinfast(F32& fResult, F32 fAngle)
{
    F32 fASqr = fAngle * fAngle;
    fResult = 0.00761f;
    fResult *= fASqr;
    fResult -= 0.16605f;
    fResult *= fASqr;
    fResult += 1.0f;
    fResult *= fAngle;
}

#endif
