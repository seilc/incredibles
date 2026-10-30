#ifndef XMATH2_H
#define XMATH2_H

#include "xMath.h"

struct xVec2
{
    F32 x;
    F32 y;

    void assign(F32 x, F32 y)
    {
        this->x = x;
        this->y = y;
    }

    void assign(F32 xy)
    {
        assign(xy, xy);
    }
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