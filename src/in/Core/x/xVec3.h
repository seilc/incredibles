#ifndef XVEC3_H
#define XVEC3_H

#include "xMath.h"
#include "iMath.h"

#include <rwcore.h>
#include <types.h>

union xVec3
{
    RwV3d m_RwV3d;

    struct
    {
        F32 x;
        F32 y;
        F32 z;
    };

    F32 a[3];

    static const xVec3 m_Null;
    static const xVec3 m_UnitAxisX;
    static const xVec3 m_UnitAxisY;
    static const xVec3 m_UnitAxisZ;

    F32 length() const;
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

    xVec3 operator-(const xVec3& v) const;
    xVec3 operator*(F32 s) const;
    xVec3& operator+=(const xVec3& v);
    xVec3& operator*=(F32 s);
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

#define xVec3NormalizeMacro(outlen, o, v)                                                          \
    {                                                                                              \
        F32 len = 1.0f;                                                                        \
        F32 len2 = xsqr((v)->x) + xsqr((v)->y) + xsqr((v)->z);                                 \
                                                                                                   \
        if (xeq(len2, 1.0f, EPSILON))                                                              \
        {                                                                                          \
            (o)->x = (v)->x;                                                                       \
            (o)->y = (v)->y;                                                                       \
            (o)->z = (v)->z;                                                                       \
        }                                                                                          \
        else if (xeq(len2, 0.0f, EPSILON))                                                         \
        {                                                                                          \
            (o)->x = 0.0f;                                                                         \
            (o)->y = 1.0f;                                                                         \
            (o)->z = 0.0f;                                                                         \
                                                                                                   \
            len = 0.0f;                                                                            \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            len = xsqrt(len2);                                                                     \
                                                                                                   \
            F32 len_inv = 1.0f / len;                                                          \
                                                                                                   \
            (o)->x = (v)->x * len_inv;                                                             \
            (o)->y = (v)->y * len_inv;                                                             \
            (o)->z = (v)->z * len_inv;                                                             \
        }                                                                                          \
                                                                                                   \
        (outlen) = len;                                                                            \
    }

#endif