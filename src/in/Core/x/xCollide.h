#ifndef XCOLLIDE_H
#define XCOLLIDE_H

#include "xModel.h"

struct xCollis
{
    struct tri_data
    {
        U32 index;
        F32 r;
        F32 d;
    };

    U32 flags;
    U32 oid;
    void* optr;
    xModelInstance* mptr;
    F32 dist;
    F32 test_dist;
    xVec3 norm;
    xVec3 tohit;
    xVec3 depen;
    xVec3 hdng;
    union
    {
        struct
        {
            F32 t;
            F32 u;
            F32 v;
        } tuv;
        tri_data tri;
    };
};

enum xCollideSphereHitType
{
    exCOLLIDESPHEREHITTYPE_UNKNOWN,
    exCOLLIDESPHEREHITTYPE_INTERIOR,
    exCOLLIDESPHEREHITTYPE_EDGE,
    exCOLLIDESPHEREHITTYPE_VERTEX
};

U32 xSphereHitsSphere(const xSphere* a, const xSphere* b, xCollis* coll);
U32 xSphereHitsBox(const xSphere* a, const xBox* b, xCollis* coll);
U32 xSphereHitsOBB_nu(const xSphere* s, const xBox* b, const xMat4x3* m, xCollis* coll);
bool xSphereHitsBox(const xVec3& c, F32 r, const xBox& b);
U32 xBoxHitsSphere(const xBox* a, const xSphere* b, xCollis* coll);
U32 xObbHitsObb(const xBox* a, const xMat4x3* a_mat, const xBox* b, const xMat4x3* b_mat,
                   xCollis* coll);
U32 xBoxHitsObb(const xBox* a, const xBox* b, const xMat4x3* mat, xCollis* coll);
bool xOBBHitsOBB(const xBox& a, const xMat4x3& amat, const xBox& b, const xMat4x3& bmat);
U32 xSphereHitsVCylinder(const xSphere* sph, const xCylinder* cyl, xCollis* coll);
U32 xCylinderHitsSphere(const xCylinder* cyl, const xSphere* sph, xCollis* coll);
void xBoxForModel(xBox& o, const xModelInstance* m, bool childModels);
void xSphereForModel(xSphere& o, const xModelInstance* model, bool childModels);

inline bool xSphereHitsSphere(const xVec3& sc, F32 sr, const xVec3& cc, F32 cr)
{
    return (cc - sc).length2() <= (sr + cr) * (sr + cr);
}

inline bool xSphereHitsSphere(const xSphere& a, const xSphere& b)
{
    return xSphereHitsSphere(a.center, a.r, b.center, b.r);
}

inline bool xSphereHitsBox(const xSphere& s, const xBox& b)
{
    return xSphereHitsBox(s.center, s.r, b);
}

inline bool xSphereHitsOBB(const xVec3& c, F32 r, const xBox& b, const xMat4x3& mat)
{
    xVec3 v;
    xMat4x3Tolocal(&v, &mat, &c);
    return xSphereHitsBox(v, r, b);
}

inline bool xSphereHitsOBB(const xSphere& s, const xBox& b, const xMat4x3& mat)
{
    return xSphereHitsOBB(s.center, s.r, b, mat);
}

inline bool xBoxHitsBox(const xBox& a, const xBox& b)
{
    return a.upper.x >= b.lower.x && a.upper.y >= b.lower.y && a.upper.z >= b.lower.z &&
           a.lower.x <= b.upper.x && a.lower.y <= b.upper.y && a.lower.z <= b.upper.z;
}

#endif