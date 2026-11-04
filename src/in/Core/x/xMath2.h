#ifndef XMATH2_H
#define XMATH2_H

#include "xMath.h"

struct xVec2
{
    F32 x;
    F32 y;

    xVec2& assign(F32 x, F32 y) { this->x = x; this->y = y; return *this; }

    void assign(F32 xy)
    {
        assign(xy, xy);
    }

    xVec2& operator-=(const xVec2& v) { x -= v.x; y -= v.y; return *this; }
    xVec2& operator*=(F32 f) { x *= f; y *= f; return *this; }
    xVec2& operator/=(F32 f) { F32 i = 1.0f / f; x *= i; y *= i; return *this; }
    F32 dot(const xVec2& c) const { return x * c.x + y * c.y; }
    F32 length() const { return xsqrt(length2()); }
    F32 length2() const { return x * x + y * y; }
};

template <class T> struct basic_rect
{
    T x;
    T y;
    T w;
    T h;

    static const basic_rect<F32> m_Null;
    static const basic_rect<F32> m_Unit;

    basic_rect& expand(T left, T top, T right, T bottom)
    {
        x -= left; w += left + right;
        y -= top; h += top + bottom;
        return *this;
    }

    basic_rect& expand(T v) { return expand(v, v, v, v); }
    basic_rect& contract(T v) { return expand(-v); }
};

extern const xVec2 g_O2;
extern const xVec2 g_X2;

#endif