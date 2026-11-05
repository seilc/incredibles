#ifndef XVEC3INLINES_H
#define XVEC3INLINES_H

inline U32 xVec3Equals(const xVec3* a, const xVec3* b)
{
    return xeq(a->x, b->x, 0.0f) && xeq(a->y, b->y, 0.0f) && xeq(a->z, b->z, 0.0f);
}

inline void xVec3AddScaled(xVec3& a, const xVec3& b, F32 scale)
{
    a.x += b.x * scale;
    a.y += b.y * scale;
    a.z += b.z * scale;
}

inline void xVec3AddScaled(xVec3& o, const xVec3& a, const xVec3& b, F32 scale)
{
    o.x = a.x + b.x * scale;
    o.y = a.y + b.y * scale;
    o.z = a.z + b.z * scale;
}

inline void xVec3Sub(xVec3& o, const xVec3& a, const xVec3& b)
{
    o.x = a.x - b.x;
    o.y = a.y - b.y;
    o.z = a.z - b.z;
}

inline void xVec3Cross(xVec3& o, const xVec3& a, const xVec3& b)
{
    o.x = a.y * b.z - b.y * a.z, o.y = a.z * b.x - b.z * a.x, o.z = a.x * b.y - b.x * a.y;
}

inline void xVec3SMul(xVec3& o, const xVec3& v, F32 s)
{
    o.x = v.x * s;
    o.y = v.y * s;
    o.z = v.z * s;
}

inline void xVec3SMulBy(xVec3* v, F32 s)
{
    v->x *= s, v->y *= s, v->z *= s;
}

inline void xVec3SMulBy(xVec3& v, F32 s)
{
    v.x *= s;
    v.y *= s;
    v.z *= s;
}

inline void xVec3ScaleC(xVec3& o, F32 scale)
{
    o.x *= scale;
    o.y *= scale;
    o.z *= scale;
}

inline F32 xVec3Length2(const xVec3* v)
{
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

inline F32 xVec3Length(const xVec3* v)
{
    return xsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

inline F32 xVec3Dist(const xVec3* a, const xVec3* b)
{
    F32 d;
    F32 dx__ = a->x - b->x;
    F32 dy__ = a->y - b->y;
    F32 dz__ = a->z - b->z;
    d = xsqrt(dx__ * dx__ + dy__ * dy__ + dz__ * dz__);
    return d;
}

inline void xVec3Lerp(xVec3* o, const xVec3* a, const xVec3* b, F32 f)
{
    o->x = a->x * (1.0f - f) + b->x * f, o->y = a->y * (1.0f - f) + b->y * f, o->z = a->z * (1.0f - f) + b->z * f;
}

#endif
