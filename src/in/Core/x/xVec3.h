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

inline void xVec3Add(xVec3* o, const xVec3* a, const xVec3* b)
{
    o->x = a->x + b->x;
    o->y = a->y + b->y;
    o->z = a->z + b->z;
}

#define xVec3NormalizeMacro(outlen, o, v)                                                          \
    {                                                                                              \
        F32 len = 1.0f;                                                                        \
        F32 len2 = SQR((v)->x) + SQR((v)->y) + SQR((v)->z);                                    \
                                                                                                   \
        if (APPROX_EQUAL(len2, 1.0f))                                                              \
        {                                                                                          \
            (o)->x = (v)->x;                                                                       \
            (o)->y = (v)->y;                                                                       \
            (o)->z = (v)->z;                                                                       \
        }                                                                                          \
        else if (APPROX_EQUAL(len2, 0.0f))                                                         \
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