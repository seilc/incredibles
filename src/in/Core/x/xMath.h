#ifndef XMATH_H
#define XMATH_H

#include "iMath.h"

#include <stddef.h>
#include <cmath>

#define xabs(x) iabs(x)
#define xeq(a,b,e) (xabs((a) - (b)) <= (e))
#define xfeq0(x) (((x) >= -EPSILON) && ((x) <= EPSILON))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MAX3(a,b,c) (MAX(MAX((a), (b)), (c)))
#define CLAMP(x,a,b) MAX((a), MIN((x), (b)))
#define xsqr(x) ((x) * (x))
#define xsignbit(x) ((x) < 0.0f ? 1 : 0)
#define xsamesign(a,b) (xsignbit(a) == xsignbit(b))

#define PI 3.14159265f
#define HUGE 1e38f

template <class T>
T range_limit(T v, T minv, T maxv)
{
    return v <= minv ? minv : (v >= maxv ? maxv : v);
}

static const S32 MAX_BINOM_SIZE = 528;
extern S32 binom_triangle[MAX_BINOM_SIZE];

inline S32 xBinom(S32 n, S32 r)
{
    return binom_triangle[n * (n + 1) / 2 + r];
}

struct xFuncPiece
{
    F32 coef[5];
    F32 end;
    S32 order;
    xFuncPiece* next;
};

struct xResponseCurve
{
    struct node
    {
        F32 t;
    };

    struct inode : node
    {
        F32 value[1];
    };

    void reset(U32 values, const void* curve, U32 nodes);
    void eval_linear(F32 t, F32* value) const;
    void eval_smooth(F32 t, F32* value) const;

private:
    void find_active_node(F32 t, size_t stride) const
    {
        U32 max_active_node = _nodes - 2;
        const U8* it = (const U8*)curve + stride * active_node;
        while (true) {
            inode& n = *(inode*)it;
            inode& next = *(inode*)(it + stride);
            if (t < n.t) {
                if (active_node == 0) break;
                it -= stride;
                active_node--;
            }
            else if (t > next.t) {
                if (active_node >= max_active_node) break;
                it += stride;
                active_node++;
            } else {
                break;
            }
        }
    }

    U32 _values;
    const inode* curve;
    U32 _nodes;
    mutable U32 active_node;
};

void xMathInit();
void xMathExit();
F32 xatof(const char* x);
U32 xMathSolveQuadratic(F32 a, F32 b, F32 c, F32* x1, F32* x2);
F32 xAngleClamp(F32 a);
F32 xAngleClampFast(F32 a);
F32 xDangleClamp(F32 a);
F32 xDangleClampFast(F32 a);
void xAccelMove(F32& x, F32& v, F32 a, F32 dt, F32 endx, F32 maxv);
void xAccelMove2(F32& x, F32& v, F32 decel, F32 accel, F32 dt, F32 endx, F32 maxv);
F32 xAccelMoveTime(F32 dx, F32 a, F32, F32 maxv);
void xAccelMove(F32& x, F32& v, F32 a, F32 dt, F32 maxv);
void xAccelMoveRange(F32& x, F32& v, F32 accel_min, F32 accel_max, F32 dt, F32 endx, F32 maxv);
void xAccelStop(F32& x, F32& v, F32 a, F32 dt);
F32 xFuncPiece_Eval(xFuncPiece* func, F32 param, xFuncPiece** iterator);
void xFuncPiece_Derivative(xFuncPiece* deriv, xFuncPiece* func);
void xFuncPiece_EndPoints(xFuncPiece* func, F32 pi, F32 pf, F32 fi, F32 ff);
void xFuncPiece_EndPointsAndSlopes(xFuncPiece* func, F32 pi, F32 pf, F32 fi, F32 ff, F32 vi, F32 vf);
void xFuncPiece_EndPointsAllInitial(xFuncPiece* func, F32 pi, F32 pf, F32 fi, F32 ff, F32 vi, F32 ai);
void xFuncPiece_ShiftPiece(xFuncPiece* shift, xFuncPiece* func, F32 newZero);

F32 xsqrt(F32 x);
F32 xlog(F32 x);
F32 xexp(F32 x);
F32 xpow(F32 x, F32 y);
F32 xacos(F32 x);
F32 xasin(F32 x);
F32 xatan2(F32 y, F32 x);
F32 xfmod(F32 x, F32 y);
void xsqrtfast(F32& o, F32 fVal);

inline F32 xDampSpringCoeff(F32 dt, F32 k)
{
    return xexp(-k * dt);
}

inline void xDampSpring(F32& s, F32& v, F32 dt, F32 k, F32 e)
{
    F32 c1 = s, c2 = v + k * s;
    s = e * (c1 + c2 * dt);
    v = c2 * e - k * s;
}

inline void xDampSpring(F32& s, F32& v, F32 dt, F32 k)
{
    xDampSpring(s, v, dt, k, xDampSpringCoeff(dt, k));
}

inline F32 xrmod(F32 ang)
{
    F32 frac = (1.0f / (2.0f * PI)) * ang;
    if (frac < 0.0f) return 2.0f * PI * (1.0f + (frac - std::ceilf(frac)));
    else if (frac >= 1.0f) return 2.0f * PI * (frac - std::floorf(frac));
    return ang;
}

#include "xMathInlines.h"

#endif
