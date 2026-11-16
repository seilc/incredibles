#ifndef XMATH2_H
#define XMATH2_H

#include "xMath.h"

struct xVec2
{
    F32 x, y;

    xVec2& assign(F32 x, F32 y) { this->x = x; this->y = y; return *this; }

    void assign(F32 xy)
    {
        assign(xy, xy);
    }

    xVec2& operator=(F32 f) { x = y = f; return *this; }
    xVec2& operator-=(const xVec2& v) { x -= v.x; y -= v.y; return *this; }
    xVec2 operator-(const xVec2& v) const { xVec2 temp = *this; temp -= v; return temp; }
    xVec2& operator*=(F32 f) { x *= f; y *= f; return *this; }
    xVec2& operator/=(F32 f) { F32 i = 1.0f / f; x *= i; y *= i; return *this; }
    F32 dot(const xVec2& c) const { return x * c.x + y * c.y; }
    F32 length() const { return xsqrt(length2()); }
    F32 length2() const { return x * x + y * y; }
    xVec2& normalize() { *this *= xinvsqrt(length2()); return *this; }

    xVec2& safe_normalize(const xVec2& safety)
    {
        F32 len2 = length2();
        if (len2 < 0.0000001f) {
            return *this = safety;
        }
        return *this *= 1.0f / xsqrt(len2);
    }
};

typedef struct _xMat3x2
{
    xVec2 right;
    xVec2 up;
    xVec2 pos;
} xMat3x2;

template <class T> struct basic_rect
{
    T x;
    T y;
    T w;
    T h;

    static const basic_rect<F32> m_Null;
    static const basic_rect<F32> m_Unit;

    basic_rect& assign(T x, T y, T w, T h) { this->x = x; this->y = y; this->w = w; this->h = h; return *this; }

    basic_rect& expand(T left, T top, T right, T bottom)
    {
        x -= left; w += left + right;
        y -= top; h += top + bottom;
        return *this;
    }

    basic_rect& expand(T v) { return expand(v, v, v, v); }
    basic_rect& contract(T v) { return expand(-v); }
    basic_rect& contract(T left, T top, T right, T bottom) { return expand(-left, -top, -right, -bottom); }

    bool empty() const { return w <= 0 || h <= 0; }

    void get_bounds(T& left, T& top, T& right, T& bottom) const
    {
        left = x; right = x + w;
        top = y; bottom = y + h;
    }

    basic_rect& set_bounds(T left, T top, T right, T bottom)
    {
        x = left; w = right - left;
        y = top; h = bottom - top;
        return *this;
    }

    basic_rect& operator|=(const basic_rect& c)
    {
        T l, t, r, b;
        get_bounds(l, t, r, b);
        T cl, ct, cr, cb;
        c.get_bounds(cl, ct, cr, cb);
        if (l > cl) l = cl;
        if (t > ct) t = ct;
        if (r < cr) r = cr;
        if (b < cb) b = cb;
        set_bounds(l, t, r, b);
        return *this;
    }

    basic_rect& scale(T xs, T ys, T ws, T hs) { x *= xs; y *= ys; w *= ws; h *= hs; return *this; }
    basic_rect& scale(T xs, T ys) { return scale(xs, ys, xs, ys); }
    basic_rect& scale(T vs) { return scale(vs, vs, vs, vs); }

    basic_rect& move(T x, T y) { this->x += x; this->y += y; return *this; }

    void clip(basic_rect& r1, basic_rect& r2) const
    {
        T xratio = r2.w / r1.w, yratio = r2.h / r1.h;
        if (r1.x < x) {
            T d1 = x - r1.x;
            T d2 = xratio * d1;
            r1.x = x; r1.w -= d1;
            r2.x += d2; r2.w -= d2;
        }
        if (r1.y < y) {
            T d1 = y - r1.y;
            T d2 = yratio * d1;
            r1.y = y; r1.h -= d1;
            r2.y += d2; r2.h -= d2;
        }
        T r = r1.x + r1.w, cr = x + w;
        if (r > cr) {
            r2.w -= xratio * (r - cr);
            r1.w = cr - r1.x;
        }
        T b = r1.y + r1.h, cb = y + h;
        if (b > cb) {
            r2.h -= yratio * (b - cb);
            r1.h = cb - r1.y;
        }
    }
};

extern const xVec2 g_O2;
extern const xVec2 g_X2;

inline void xVec2Init(xVec2* v, F32 x, F32 y)
{
    v->x = x;
    v->y = y;
}

inline F32 xVec2Dot(const xVec2* v1, const xVec2* v2)
{
    return v1->x * v2->x + v1->y * v2->y;
}

inline F32 xVec2Dist(F32 x1, F32 y1, F32 x2, F32 y2)
{
    F32 dx = x1 - x2;
    F32 dy = y1 - y2;
    return xsqrt(dx * dx + dy * dy);
}

inline void xVec2SubFromC(xVec2* o, F32 x, F32 y)
{
    o->x -= x;
    o->y -= y;
}

#endif