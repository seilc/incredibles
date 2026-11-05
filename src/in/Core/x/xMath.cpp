#include "xMath.h"

#include "decomp.h"

#include <stdlib.h>

static S32 xmath_inited;
static S32 xmath_exited;
static U32 rndseed;

S32 binom_triangle[MAX_BINOM_SIZE];

static void xBinomInit();

void xMathInit()
{
    if (xmath_inited) {
        return;
    }

    xmath_inited = 1;
    rndseed = 0;
    xBinomInit();
}

void xMathExit()
{
    if (xmath_exited) {
        return;
    }

    xmath_exited = 1;
}

F32 xatof(const char* x)
{
    return atof(x);
}

DECOMP_FORCEFLOAT(HUGE)
DECOMP_FORCEFLOAT(0.5f)
DECOMP_FORCEFLOAT(1.0f)
DECOMP_FORCEFLOAT(2.0f)
DECOMP_FORCEFLOAT(0.0f)
DECOMP_FORCEACTIVE(xsqrt)

DECOMP_FORCEBLOCK((S32 k) { *(F32*)0 = k; })

U32 xMathSolveQuadratic(F32 a, F32 b, F32 c, F32* x1, F32* x2)
{
    F32 d, dx, p;

    if (a == 0.0f) {
        if (b == 0.0f) {
            return 0;
        }

        *x1 = -c / b;
        return 1;
    }

    d = b * b - 4.0f * a * c;
    if (d < 0.0f) {
        return 0;
    }

    p = 1.0f / (2.0f * a);
    *x1 = -b * p;
    if (d == 0.0f) {
        return 1;
    }

    *x2 = *x1;
    dx = p * xsqrt(d);
    if (a > 0.0f) {
        *x1 -= dx;
        *x2 += dx;
    } else {
        *x1 += dx;
        *x2 -= dx;
    }

    return 2;
}

DECOMP_FORCEFLOAT(PI)
DECOMP_FORCEFLOAT(-1.0f)

F32 xAngleClamp(F32 a)
{
    F32 b = xfmod(a, 2.0f * PI);
    if (b < 0.0f) {
        b += 2.0f * PI;
    }

    return b;
}

F32 xAngleClampFast(F32 a)
{
    if (a < 0.0f) {
        a += 2.0f * PI;
    } else if (a >= 2.0f * PI) {
        a -= 2.0f * PI;
    }

    return a;
}

F32 xDangleClamp(F32 a)
{
    F32 b = xfmod(a, 2.0f * PI);
    if (b >= PI) {
        b -= 2.0f * PI;
    } else if (b < -PI) {
        b += 2.0f * PI;
    }

    return b;
}

F32 xDangleClampFast(F32 a)
{
    if (a >= PI) {
        a -= 2.0f * PI;
    } else if (a < -PI) {
        a += 2.0f * PI;
    }

    return a;
}

static void xBinomInit()
{
    binom_triangle[0] = 1;
    S32* row0 = binom_triangle;
    S32* row1 = binom_triangle + 1;

    for (S32 i = 1; i < 32; i++) {
        row1[0] = row1[i] = 1;

        const S32 *col00 = row0, *col01 = row0 + 1; S32 *col1 = row1 + 1, *end_col1 = row1 + i;
        for (; col1 < end_col1; ++col00, ++col01, ++col1)
            *col1 = *col00 + *col01;

        row0 = row1;
        row1 += i + 1;
    }
}

void xAccelMove(F32& x, F32& v, F32 a, F32 dt, F32 endx, F32 maxv)
{
    F32 offset = endx - x;
    F32 t1 = (xabs(v) < 0.001f || !xsamesign(offset, v)) ? HUGE : offset / v;
    F32 t2 = xabs(v / a);

    if (t1 < t2) a *= -1.0f;
    if (offset < 0.0f) a *= -1.0f;

    const F32 oldv = v;
    F32 dv, newv, adx;
    dv = a * dt;
    newv = v + dv;

    if (xabs(newv) <= maxv) { v = newv; adx = 0.5f * dv * dt; }
    else if (xabs(v) <= maxv) {
        v = range_limit(newv, -maxv, maxv);
        if (oldv != v) {
            F32 diff = v - oldv;
            adx = 0.5f * diff * diff / a;
        } else {
            adx = 0.0f;
        }
    }
    else if (!xsamesign(newv, dv)) { v = newv; adx = 0.5f * dv * dt; }
    else adx = 0.0f;

    F32 dx = oldv * dt + adx;

    if (t1 > t2) {
        if (xsamesign(dx, offset) && xabs(dx) > xabs(offset)) {
            x = endx;
            v = 0.0f;
            return;
        }
    } else if (!xsamesign(dx, offset)) {
        x = endx;
        v = 0.0f;
        return;
    }

    x += dx;
}

void xAccelMove2(F32& x, F32& v, F32 decel, F32 accel, F32 dt, F32 endx, F32 maxv)
{
    F32 offset = endx - x;
    F32 t1 = (xabs(v) < 0.001f || !xsamesign(offset, v)) ? HUGE : offset / v;
    F32 t2 = xabs(v / decel);
    F32 a = (t1 < t2) ? decel : accel;

    if (offset < 0.0f) {
        a *= -1.0f;
    }

    const F32 oldv = v;
    F32 dv, newv, adx;
    dv = a * dt;
    newv = v + dv;

    if (xabs(newv) <= maxv) {
        v = newv;
        adx = 0.5f * dv * dt;
    } else if (xabs(v) <= maxv) {
        v = range_limit(newv, -maxv, maxv);
        if (oldv != v) {
            F32 diff = v - oldv;
            adx = 0.5f * diff * diff / a;
        } else {
            adx = 0.0f;
        }
    } else if (!xsamesign(newv, dv)) {
        v = newv;
        adx = 0.5f * dv * dt;
    } else {
        adx = 0.0f;
    }

    F32 dx = oldv * dt + adx;

    if (t1 > t2) {
        if (xsamesign(dx, offset) && xabs(dx) > xabs(offset)) {
            x = endx;
            v = 0.0f;
            return;
        }
    } else if (!xsamesign(dx, offset)) {
        x = endx;
        v = 0.0f;
        return;
    }

    x += dx;
}

F32 xAccelMoveTime(F32 dx, F32 a, F32, F32 maxv)
{
    F32 time;
    F32 atime = maxv / a;
    F32 adist = atime * (0.5f * a * atime);

    dx *= 0.5f;
    if (adist < dx) {
        time = xsqrt(2.0f * dx / a);
    } else {
        time = atime + (dx - adist) / maxv;
    }

    return 2.0f * time;
}

void xAccelMove(F32& x, F32& v, F32 a, F32 dt, F32 maxv)
{
    if (xabs(v) > xabs(maxv)) {
        if (v < 0.0f) {
            if (a > 0.0f) {
                a = -a;
            }
        } else if (a < 0.0f) {
            a = -a;
        }

        a = -a;
    }

    if (a < 0.0f) {
        if (maxv > 0.0f) {
            maxv = -maxv;
        }
    } else if (maxv < 0.0f) {
        maxv = -maxv;
    }

    F32 diff = maxv - v;
    F32 dv = a * dt;
    if (xabs(diff) < xabs(dv)) {
        x += v * dt + 0.5f * diff * diff / a;
        v = maxv;
    } else {
        x += 0.5f * a * dt * dt + v * dt;
        v += dv;
    }
}

void xAccelMoveRange(F32& x, F32& v, F32 accel_min, F32 accel_max, F32 dt, F32 endx, F32 maxv)
{
    F32 offset = endx - x;
    F32 t1 = (xabs(v) < 0.001f || !xsamesign(offset, v)) ? HUGE : offset / v;
    F32 t2 = (v >= 0.0f) ? -v / accel_min : -v / accel_max;
    F32 a;

    if (t1 >= t2) {
        a = (offset < 0.0f) ? accel_min : accel_max;
    } else {
        a = (offset < 0.0f) ? accel_max : accel_min;
    }

    const F32 oldv = v;
    F32 dv, newv, adx;
    dv = a * dt;
    newv = v + dv;

    if (xabs(newv) <= maxv) {
        v = newv;
        adx = 0.5f * dv * dt;
    } else if (xabs(v) <= maxv) {
        v = range_limit(newv, -maxv, maxv);
        if (oldv != v) {
            F32 diff = v - oldv;
            adx = 0.5f * diff * diff / a;
        } else {
            adx = 0.0f;
        }
    } else if (!xsamesign(newv, dv)) {
        v = newv;
        adx = 0.5f * dv * dt;
    } else {
        adx = 0.0f;
    }

    F32 dx = oldv * dt + adx;

    if (t1 > t2) {
        if (xsamesign(dx, offset) && xabs(dx) > xabs(offset)) {
            dx = offset;
            v = 0.0f;
        }
    } else if (!xsamesign(dx, offset)) {
        dx = offset;
        v = 0.0f;
    }

    x += dx;
}

void xAccelStop(F32& x, F32& v, F32 a, F32 dt)
{
    if (xfeq0(v)) {
        return;
    }

    if (v < 0.0f) {
        if (a > 0.0f) {
            a = -a;
        }
    } else if (a < 0.0f) {
        a = -a;
    }

    a = -a;

    F32 oldv = v;
    v += a * dt;
    if (xsamesign(oldv, v)) {
        x += 0.5f * dt * dt * a + oldv * dt;
    } else {
        if (!xfeq0(a)) {
            x -= 0.5f * oldv * oldv / a;
        }

        v = 0.0f;
    }
}

F32 xFuncPiece_Eval(xFuncPiece* func, F32 param, xFuncPiece** iterator)
{
    F32 result;

    while (func && func->end < param - EPSILON) func = func->next;

    if (func) {
        result = func->coef[func->order];
        for (S32 i = func->order - 1; i >= 0; i--) {
            result = result * param + func->coef[i];
        }

        if (iterator) *iterator = func;
    } else {
        result = 0.0f;
        if (iterator) *iterator = NULL;
    }

    return result;
}

void xFuncPiece_Derivative(xFuncPiece* deriv, xFuncPiece* func)
{
    while (func && deriv) {
        if (func->order != 0) {
            deriv->order = func->order - 1;
            deriv->end = func->end;
            for (S32 i = 0; i <= deriv->order; i++) {
                deriv->coef[i] = (i + 1) * func->coef[i + 1];
            }
        } else {
            deriv->order = 0;
            deriv->coef[0] = 0.0f;
        }

        func = func->next;
        deriv = deriv->next;
    }
}

void xFuncPiece_EndPoints(xFuncPiece* func, F32 pi, F32 pf, F32 fi, F32 ff)
{
    F32 fdiff = ff - fi;
    func->end = pf - pi;
    F32 xfinv = 1.0f / func->end;
    func->order = 1;
    func->coef[0] = fi;
    func->coef[1] = fdiff * xfinv;
    xFuncPiece_ShiftPiece(func, func, -pi);
}

void xFuncPiece_EndPointsAndSlopes(xFuncPiece* func, F32 pi, F32 pf, F32 fi, F32 ff, F32 vi, F32 vf)
{
    F32 fdiff = ff - fi;
    func->end = pf - pi;
    F32 xfinv = 1.0f / func->end;
    F32 xfinv2 = xfinv * xfinv;
    F32 xfinv3 = xfinv2 * xfinv;
    func->order = 3;
    func->coef[0] = fi;
    func->coef[1] = vi;
    func->coef[2] = 3.0f * fdiff * xfinv2 - xfinv * (2.0f * vi + vf);
    func->coef[3] = -2.0f * fdiff * xfinv3 + xfinv2 * (vi + vf);
    xFuncPiece_ShiftPiece(func, func, -pi);
}

void xFuncPiece_EndPointsAllInitial(xFuncPiece* func, F32 pi, F32 pf, F32 fi, F32 ff, F32 vi, F32 ai)
{
    F32 fdiff = ff - fi;
    func->end = pf - pi;
    F32 xfinv = 1.0f / func->end;
    F32 xfinv2 = xfinv * xfinv;
    F32 xfinv3 = xfinv2 * xfinv;
    func->order = 3;
    func->coef[0] = fi;
    func->coef[1] = vi;
    func->coef[2] = 0.5f * ai;
    func->coef[3] = fdiff * xfinv3 - vi * xfinv2 - 0.5f * ai * xfinv;
    xFuncPiece_ShiftPiece(func, func, -pi);
}

void xFuncPiece_ShiftPiece(xFuncPiece* shift, xFuncPiece* func, F32 newZero)
{
    S32 i, j;
    xFuncPiece temp;

    for (i = 0; i < func->order; i++) temp.coef[i] = 0.0f;

    temp.coef[func->order] = func->coef[func->order];
    for (i = func->order - 1; i >= 0; i--) {
        for (j = i; j < func->order; j++) {
            temp.coef[j] += newZero * temp.coef[j + 1];
        }

        temp.coef[i] += func->coef[i];
    }

    shift->order = func->order;
    for (i = 0; i <= shift->order; i++) {
        shift->coef[i] = temp.coef[i];
    }

    shift->end = func->end - newZero;
}

void xResponseCurve::reset(U32 values, const void* curve, U32 nodes)
{
    _values = values;
    this->curve = (const inode*)curve;
    _nodes = nodes;
    active_node = 0;
}

void xResponseCurve::eval_linear(F32 t, F32* value) const
{
    F32* end = value + _values;
    size_t stride = _values * sizeof(F32) + sizeof(node);
    find_active_node(t, stride);

    const inode& n1 = *(const inode*)((const U8*)curve + active_node * stride);
    const inode& n2 = *(const inode*)((const U8*)&n1 + stride);
    F32 dt = n2.t - n1.t;
    if (xfeq0(dt)) {
        const F32* v1 = n1.value;
        for (; value != end; ++v1, ++value)
            *value = *v1;
        return;
    }

    F32 u = (t - n1.t) / dt;
    const F32 *v1 = n1.value, *v2 = n2.value;
    for (; value != end; ++v1, ++v2, ++value)
        *value = *v1 + u * (*v2 - *v1);
}

void xResponseCurve::eval_smooth(F32 t, F32* value) const
{
    if (_nodes == 2) { eval_linear(t, value); return; }

    F32* end = value + _values;
    size_t stride = _values * sizeof(F32) + sizeof(node);
    find_active_node(t, stride);

    const inode& n1 = *(const inode*)((const U8*)curve + active_node * stride);
    const inode& n2 = *(const inode*)((const U8*)&n1 + stride);
    F32 dt = n2.t - n1.t;
    if (xfeq0(dt)) {
        const F32* v1 = n1.value;
        for (; value != end; ++v1, ++value)
            *value = *v1;
        return;
    }

    F32 u = (t - n1.t) / dt;
    F32 u2 = u * u;
    F32 u3 = u2 * u;
    F32 c0 = -0.5f * u3 + u2 + -0.5f * u;
    F32 c1 = 1.5f * u3 + -2.5f * u2 + 1.0f;
    F32 c2 = -1.5f * u3 + 2.0f * u2 + 0.5f * u;
    F32 c3 = 0.5f * u3 + -0.5f * u2;

    if (active_node > 0 && active_node < _nodes - 2) {
        const inode &n0 = curve[active_node - 1], &n3 = curve[active_node + 2];
        const F32 *v0 = n0.value, *v1 = n1.value, *v2 = n2.value, *v3 = n3.value;
        for (; value != end; ++v0, ++v1, ++v2, ++v3, ++value)
            *value = c0 * *v0 + c1 * *v1 + c2 * *v2 + c3 * *v3;
    }
    else if (active_node > 0) {
        F32 c23 = c2 + c3;
        const inode& n0 = curve[active_node - 1];
        const F32 *v0 = n0.value, *v1 = n1.value, *v2 = n2.value;
        for (; value != end; ++v0, ++v1, ++v2, ++value)
            *value = c0 * *v0 + c1 * *v1 + c23 * *v2;
    } else {
        F32 c01 = c0 + c1;
        const inode& n3 = curve[active_node + 2];
        const F32 *v1 = n1.value, *v2 = n2.value, *v3 = n3.value;
        for (; value != end; ++v1, ++v2, ++v3, ++value)
            *value = c01 * *v1 + c2 * *v2 + c3 * *v3;
    }
}
