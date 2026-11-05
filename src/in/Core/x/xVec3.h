#ifndef XVEC3_H
#define XVEC3_H

#include "xMath.h"
#include "iMath.h"

#include <rwcore.h>

struct xVec3
{
    union
    {
        RwV3d m_RwV3d;
        struct
        {
            F32 x, y, z;
        };
        F32 a[3];
    };

    static const xVec3 m_Null;
    static const xVec3 m_Ones;
    static const xVec3 m_NegOnes;
    static const xVec3 m_UnitAxisX;
    static const xVec3 m_UnitAxisY;
    static const xVec3 m_UnitAxisZ;
    static const xVec3 m_HalfVec;
    static const xVec3 m_NegHalfVec;
    static const xVec3 m_DoubleVec;
    static const xVec3 m_NegDoubleVec;

    F32 length() const { return xsqrt(length2()); }
    F32 normalize()
    {
        F32 f = xinvsqrt(length2());
        *this *= f;
        return 1.0f / f;
    }
    F32 length2() const;
    void up_normalize();

    void assign(F32 x, F32 y, F32 z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    void assign(F32 xyz)
    {
        assign(xyz, xyz, xyz);
    }

    xVec3 operator+(const xVec3& v) const { xVec3 r = *this; r += v; return r; }
    xVec3 operator-(const xVec3& v) const;
    xVec3 operator*(F32 s) const;
    xVec3 operator/(F32 f) const { xVec3 r = *this; r /= f; return r; }
    xVec3 operator-() const { xVec3 r = *this; r.x = -r.x; r.y = -r.y; r.z = -r.z; return r; }
    xVec3& operator+=(const xVec3& v);
    xVec3& operator-=(const xVec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    xVec3& operator+=(F32 f) { x += f; y += f; z += f; return *this; }
    xVec3& operator-=(F32 f) { x -= f; y -= f; z -= f; return *this; }
    xVec3& operator*=(F32 s);
    xVec3& operator/=(F32 f) { F32 i = 1.0f / f; x *= i; y *= i; z *= i; return *this; }

    xVec3& cross(const xVec3& c0, const xVec3& c1)
    {
        x = c0.y * c1.z - c1.y * c0.z;
        y = c0.z * c1.x - c1.z * c0.x;
        z = c0.x * c1.y - c1.x * c0.y;
        return *this;
    }

    void cross_xpos(const xVec3& c0) { x = 0.0f; y = c0.z; z = -c0.y; }
    void cross_ypos(const xVec3& c0) { x = -c0.z; y = 0.0f; z = c0.x; }

    F32 dot(const xVec3& c) const;
    xVec3& negate() { x = -x; y = -y; z = -z; return *this; }

    xVec3 cross(const xVec3& c) const
    {
        xVec3 v = { y * c.z - c.y * z, z * c.x - c.z * x, x * c.y - c.x * y };
        return v;
    }

    F32 safe_normalize(const xVec3& safety)
    {
        F32 len = length();
        if (len < 0.00001f)
            *this = safety;
        else
            *this *= 1.0f / len;
        return len;
    }
};

F32 xVec3Normalize(xVec3* o, const xVec3* v);
void xVec3Copy(xVec3* o, const xVec3* v);
F32 xVec3Dot(const xVec3* a, const xVec3* b);
F32 xVec3Length(const xVec3* v);
F32 xVec3Length(F32 x, F32 y, F32 z);
void xVec3Cross(xVec3* o, const xVec3* a, const xVec3* b);
void xVec3SMul(xVec3* o, const xVec3* v, F32 s);
void xVec3Sub(xVec3* o, const xVec3* a, const xVec3* b);
void xVec3AddScaled(xVec3* o, const xVec3* v, F32 s);
void xVec3Init(xVec3* o, F32 x, F32 y, F32 z);
void xVec3Init(xVec3& o, F32 x, F32 y, F32 z);
void xVec3AddTo(xVec3& o, const xVec3& v);
void xVec3AddTo(xVec3* o, const xVec3* v);
void xVec3Inv(xVec3* o, const xVec3* v);
F32 xVec3Hdng(xVec3* hdng, const xVec3* a, const xVec3* b);
F32 xVec3NormalizeSafe(xVec3& v);
F32 xVec3Dist(const xVec3* a, const xVec3* b);
F32 xVec3Dist2(const xVec3* a, const xVec3* b);
void xVec3Lerp(xVec3* o, const xVec3* a, const xVec3* b, F32 f);

inline void xVec3Add(xVec3* o, const xVec3* a, const xVec3* b)
{
    o->x = a->x + b->x;
    o->y = a->y + b->y;
    o->z = a->z + b->z;
}

inline F32 xVec3Hdng(xVec3* hdng, const xVec3* a, const xVec3* b)
{
    F32 d;
    F32 dx__ = b->x - a->x;
    F32 dy__ = b->y - a->y;
    F32 dz__ = b->z - a->z;
    F32 dist2 = dx__*dx__ + dy__*dy__ + dz__*dz__;
    if (xeq(dist2, 1.0f, 0.00001f)) {
        hdng->x = dx__;
        hdng->y = dy__;
        hdng->z = dz__;
        d = 1.0f;
    } else if (xeq(dist2, 0.0f, 0.00001f)) {
        hdng->x = 0.0f;
        hdng->y = 1.0f;
        hdng->z = 0.0f;
        d = 0.0f;
    } else {
        d = xsqrt(dist2);
        F32 dist_inv = 1.0f / d;
        hdng->x = dx__ * dist_inv;
        hdng->y = dy__ * dist_inv;
        hdng->z = dz__ * dist_inv;
    }
    return d;
}

#define xVec3AddScaleMacro(o, a, b, s)          \
{                                               \
    (o)->x = (a)->x + (s) * (b)->x;             \
    (o)->y = (a)->y + (s) * (b)->y;             \
    (o)->z = (a)->z + (s) * (b)->z;             \
}

#define xVec3NormalizeMacro(o, v, outlen)                                                         \
{                                                                                                 \
    F32 len2 = xsqr((v)->x) + xsqr((v)->y) + xsqr((v)->z);                                        \
    if (xeq(len2, 1.0f, EPSILON)) {                                                               \
        (o)->x = (v)->x;                                                                          \
        (o)->y = (v)->y;                                                                          \
        (o)->z = (v)->z;                                                                          \
        *(outlen) = 1.0f;                                                                         \
    } else if (xeq(len2, 0.0f, EPSILON)) {                                                        \
        (o)->x = 0.0f;                                                                            \
        (o)->y = 1.0f;                                                                            \
        (o)->z = 0.0f;                                                                            \
        *(outlen) = 0.0f;                                                                         \
    } else {                                                                                      \
        *(outlen) = xsqrt(len2);                                                                  \
        F32 len_inv = 1.0f / *(outlen);                                                           \
        (o)->x = (v)->x * len_inv;                                                                \
        (o)->y = (v)->y * len_inv;                                                                \
        (o)->z = (v)->z * len_inv;                                                                \
    }                                                                                             \
}

#include "xVec3Inlines.h"

#endif
