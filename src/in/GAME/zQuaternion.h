#ifndef ZQUATERNION_H
#define ZQUATERNION_H

#include "types.h"

class zQuat
{
public:
    zQuat(const zQuat& q)
    {
        Set(q.w, q.x, q.y, q.z);
    }

    ~zQuat()
    {
    }

    zQuat(F32 _w, F32 _x, F32 _y, F32 _z)
    {
        Set(_w, _x, _y, _z);
    }

    zQuat()
    {
        Identity();
    }

    void Set(F32 _w, F32 _x, F32 _y, F32 _z)
    {
        w = _w;
        x = _x;
        y = _y;
        z = _z;
    }

    void Identity()
    {
        Set(1.0f, 0.0f, 0.0f, 0.0f);
    }

    zQuat& operator=(const zQuat& q)
    {
        w = q.w;
        x = q.x;
        y = q.y;
        z = q.z;
        return *this;
    }

    F32 LenSq() const
    {
        return w * w + x * x + y * y + z * z;
    }

    F32 w;
    F32 x;
    F32 y;
    F32 z;
};

static zQuat operator+(const zQuat& q1, const zQuat& q2)
{
    return zQuat(zQuat(q1.w + q2.w, q1.x + q2.x, q1.y + q2.y, q1.z + q2.z));
}

static zQuat operator*(const zQuat& v, const F32& s)
{
    return zQuat(zQuat(v.w * s, v.x * s, v.y * s, v.z * s));
}

static F32 DotQuat(const zQuat& q1, const zQuat& q2)
{
    return q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
}

static zQuat CrossQuat(const zQuat& q1, const zQuat& q2)
{
    zQuat q;
    q.Set(q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,
          q1.w * q2.x + q1.x * q2.w + q1.z * q2.y - q1.y * q2.z,
          q1.w * q2.y + q1.y * q2.w + q1.x * q2.z - q1.z * q2.x,
          q1.w * q2.z + q1.z * q2.w + q1.y * q2.x - q1.x * q2.y);
    return q;
}

static void QRotationOperator(zQuat& q, const zQuat& v, const zQuat& q1)
{
    q = v * (2.0f * q1.w * q1.w - q1.LenSq()) + q1 * 2.0f * DotQuat(q1, v) + CrossQuat(q1, v) * 2.0f * q1.w;
}

#endif
