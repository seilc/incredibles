#ifndef XVEC3INLINES_H
#define XVEC3INLINES_H

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

inline void xVec3ScaleC(xVec3& o, F32 scale)
{
    o.x *= scale;
    o.y *= scale;
    o.z *= scale;
}

#endif
