#ifndef XMATH3_H
#define XMATH3_H

#include "xMath.h"
#include "xVec3.h"

struct xVec4
{
    F32 x;
    F32 y;
    F32 z;
    F32 w;
};

struct xFRect
{
    F32 x, y, w, h;
};

struct xMat3x3
{
    xVec3 right;
    S32 flags;
    xVec3 up;
    U32 pad1;
    xVec3 at;
    U32 pad2;
} ALIGN(16);

struct xMat4x3 : xMat3x3
{
    xVec3 pos;
    U32 pad3;
} ALIGN(16);

struct xQuat
{
    xVec3 v;
    F32 s;
};

struct xRot
{
    xVec3 axis;
    F32 angle;
};

struct xLine3
{
    xVec3 p1;
    xVec3 p2;
};

struct xBox
{
    xVec3 upper;
    xVec3 lower;
};

struct xBBox
{
    xVec3 center;
    xBox box;
};

struct xSphere
{
    xVec3 center;
    F32 r;
};

struct xCylinder
{
    xVec3 center;
    F32 r;
    F32 h;
};

struct xCapsule
{
    xVec3 start;
    xVec3 end;
    F32 r;
};

struct xPlane
{
    xVec3 norm;
    F32 d;

    void Init(const xVec3& pt)
    {
        d = norm.dot(pt);
    }

    void Init(const xVec3& normal, const xVec3& pt);

    F32 Dist(const xVec3& pt) const;
    void Project(xVec3& proj, const xVec3& pt, F32 dist_above) const;
};

#define XRAY3_USE_MIN (1<<10)
#define XRAY3_USE_MAX (1<<11)

struct xRay3
{
    xVec3 origin;
    xVec3 dir;
    F32 min_t;
    F32 max_t;
    S32 flags;
};

struct xIsect
{
    U32 flags;
    F32 penned;
    F32 contained;
    F32 lapped;
    xVec3 point;
    xVec3 norm;
    F32 dist;
};

struct xParabola
{
    xVec3 initPos;
    xVec3 initVel;
    F32 gravity;
    F32 minTime;
    F32 maxTime;
};

extern xVec3 g_O3;
extern xMat4x3 g_I3;

void xBoxInitBoundOBB(xBox* o, const xBox* b, const xMat4x3* m);
void xBoxInitBoundCapsule(xBox* b, const xCapsule* c);
void xMat3x3Euler(xMat3x3* m, F32 yaw, F32 pitch, F32 roll);
void xMat4x3Copy(xMat4x3* o, const xMat4x3* m);
void xQuatFromMat(xQuat* q, const xMat3x3* m);
void xQuatToMat(const xQuat* q, xMat3x3* m);
void xQuatSlerp(xQuat* o, const xQuat* a, const xQuat* b, F32 t);
void xMat3x3Mul(xMat3x3* o, const xMat3x3* a, const xMat3x3* b);
void xMat3x3ScaleC(xMat3x3* m, F32 x, F32 y, F32 z);
void xMat3x3Transpose(xMat3x3* o, const xMat3x3* m);
void xMat3x3Tolocal(xVec3* o, const xMat3x3* m, const xVec3* v);
F32 xMat3x3LookVec(xMat3x3* m, const xVec3* at);
void xMat3x3Normalize(xMat3x3* o, const xMat3x3* m);
void xMat4x3Mul(xMat4x3* o, const xMat4x3* a, const xMat4x3* b);
bool xVec3Eq(const xVec3& a, const xVec3& b, F32 tol);
void xMat3x3Euler(xMat3x3* m, const xVec3* ypr);
void xRotCopy(xRot* o, const xRot* r);

inline void xMat3x3Copy(xMat3x3* o, const xMat3x3* m)
{
    *o = *m;
}

inline void xMat3x3Identity(xMat3x3* m)
{
    xMat3x3Copy(m, &g_I3);
}

void xMat3x3Rot(xMat3x3* m, const xVec3* a, F32 t);
void xMat3x3GetEuler(const xMat3x3* m, xVec3* a);
void xMat3x3Euler(xMat3x3* m, const xVec3* ypr);
void xMat4x3MoveLocalRight(xMat4x3* m, F32 mag);
void xMat4x3MoveLocalUp(xMat4x3* m, F32 mag);
void xMat4x3MoveLocalAt(xMat4x3* m, F32 mag);

inline void xMat4x3Identity(xMat4x3* m)
{
    xMat4x3Copy(m, &g_I3);
}

inline static void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v)
{
    F32 x = m->right.x * v->x + m->up.x * v->y + m->at.x * v->z;
    F32 y = m->right.y * v->x + m->up.y * v->y + m->at.y * v->z;
    F32 z = m->right.z * v->x + m->up.z * v->y + m->at.z * v->z;

    o->x = x;
    o->y = y;
    o->z = z;
}

inline F32 xMat3x3LookAt(xMat3x3* m, const xVec3* from, const xVec3* to)
{
    xVec3 at;
    xVec3Sub(&at, to, from);
    return xMat3x3LookVec(m, &at);
}

inline void xMat3x3MulScaleC(xMat3x3* o, const xMat3x3* m, F32 x, F32 y, F32 z)
{
    o->right.assign(m->right.x * x, m->right.y * x, m->right.z * x);
    o->up.assign(m->up.x * y, m->up.y * y, m->up.z * y);
    o->at.assign(m->at.x * z, m->at.y * z, m->at.z * z);
}

inline void xMat3x3MulScale(xMat3x3* o, const xMat3x3* m, xVec3* s)
{
    return xMat3x3MulScaleC(o, m, s->x, s->y, s->z);
}

inline void xMat4x3ScaleC(xMat4x3* m, F32 x, F32 y, F32 z)
{
    xMat3x3ScaleC(m, x, y, z);
    xVec3Copy(&m->pos, &g_O3);
}

inline void xMat4x3Scale(xMat4x3* m, const xVec3* s)
{
    xMat4x3ScaleC(m, s->x, s->y, s->z);
}

inline void xMat4x3Toworld(xVec3* o, const xMat4x3* m, const xVec3* v)
{
    xMat3x3RMulVec(o, m, v);

    o->x += m->pos.x;
    o->y += m->pos.y;
    o->z += m->pos.z;
}

inline void xMat4x3Tolocal(xVec3* o, const xMat4x3* m, const xVec3* v)
{
    o->x = v->x - m->pos.x;
    o->y = v->y - m->pos.y;
    o->z = v->z - m->pos.z;

    xMat3x3Tolocal(o, m, o);
}

inline void xBoxFromCapsule(xBox& box, const xVec3& s, const xVec3& e, F32 r)
{
    xVec3& l = box.lower;
    xVec3& u = box.upper;

    if (s.x < e.x) { l.x = s.x; u.x = e.x; }
    else { l.x = e.x; u.x = s.x; }
    if (s.y < e.y) { l.y = s.y; u.y = e.y; }
    else { l.y = e.y; u.y = s.y; }
    if (s.z < e.z) { l.z = s.z; u.z = e.z; }
    else { l.z = e.z; u.z = s.z; }
    l -= r;
    u += r;
}

inline void xMat3x3LookVec(const xVec3& at, xVec3& right, xVec3& up)
{
    if (at.x > 0.8f || at.x < -0.8f) {
        right.cross_ypos(at);
    } else {
        right.cross_xpos(at);
    }

    xVec3NormalizeSafe(right);

    up.cross(at, right);
}

#endif