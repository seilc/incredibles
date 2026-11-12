#ifndef XCOLLIDE_H
#define XCOLLIDE_H

#include "xModel.h"

struct xEnt;
struct xEnv;
struct xQCData;
struct xScene;

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

struct xParabola
{
    xVec3 initPos;
    xVec3 initVel;
    F32 gravity;
    F32 minTime;
    F32 maxTime;
};

inline void xParabolaEvalPos(const xParabola* p, xVec3* pos, F32 time)
{
    xVec3Copy(pos, &p->initPos);
    xVec3AddScaled(pos, &p->initVel, time);
    pos->y -= 0.5f * p->gravity * time * time;
}

inline void xParabolaEvalVel(const xParabola* p, xVec3* vel, F32 time)
{
    xVec3Copy(vel, &p->initVel);
    vel->y -= p->gravity * time;
}

#define k_HIT_IT ((U32)(1 << 0))
#define k_HIT_CALC_HDNG ((U32)(1 << 12))
#define k_HIT_CALC_TRI ((U32)(1 << 13))

enum xCollideSphereHitType
{
    exCOLLIDESPHEREHITTYPE_UNKNOWN,
    exCOLLIDESPHEREHITTYPE_INTERIOR,
    exCOLLIDESPHEREHITTYPE_EDGE,
    exCOLLIDESPHEREHITTYPE_VERTEX
};

U32 xSphereHitsSphere(const xSphere* a, const xSphere* b, xCollis* coll);
U32 xSphereHitsBox(const xSphere* a, const xBox* b, xCollis* coll);
U32 xSphereHitsOBB(const xSphere* a, const xBox* b, const xMat4x3* m, xCollis* coll); // UNUSED
U32 xSphereHitsOBB_nu(const xSphere* s, const xBox* b, const xMat4x3* m, xCollis* coll);
U32 xSphereHitsModel(const xSphere* b, const xModelInstance* m, xCollis* coll);
void xParabolaRecenter(xParabola* p, F32 newZeroT);
S32 xParabolaHitsEnv(xParabola* p, const xEnv* env, xCollis* colls);
S32 xCapsuleIsectsModelClosestToLine(const xCapsule* c, const xModelInstance* model, xCollis* colls);
S32 xCapsuleIsectsEnvClosestToLine(const xCapsule* c, const xEnv* env, xCollis* colls);
void xCapsuleHitsEnt(xScene*, xRay3*, xQCData*, xEnt* ent, void* data); // UNUSED
S32 xCapsuleIsectsSceneClosestToLine(const xCapsule* c, xScene* sc, xCollis* colls); // UNUSED
U32 xBoxHitsSphere(const xBox* a, const xSphere* b, xCollis* coll);
U32 xObbHitsObb(const xBox* a, const xMat4x3* a_mat, const xBox* b, const xMat4x3* b_mat, xCollis* coll);
U32 xBoxHitsObb(const xBox* a, const xBox* b, const xMat4x3* mat, xCollis* coll);
void xCollideCalcTri(xCollis::tri_data& tri, const xModelInstance& model, const xVec3& center, const xVec3& heading);
xVec3 xCollisTriHit(const xCollis::tri_data& tri, const xModelInstance& model);
bool xSphereHitsBox(const xVec3& c, F32 r, const xBox& b);
bool xOBBHitsOBB(const xBox& a, const xMat4x3& amat, const xBox& b, const xMat4x3& bmat);
bool xSphereHitsVCylinder(const xVec3& sc, F32 sr, const xVec3& cc, F32 cr, F32 ch);
U32 xSphereHitsVCylinder(const xSphere* sph, const xCylinder* cyl, xCollis* coll);
U32 xCylinderHitsSphere(const xCylinder* cyl, const xSphere* sph, xCollis* coll);
bool xSphereHitsVCircle(const xVec3& sc, F32 sr, const xVec3& cc, F32 cr);
bool xSphereHitsCone(const xVec3& center, F32 radius, const xVec3& kv0, F32 kr0, const xVec3& kv1, F32 kr1);
void xBoxForTriangle(xBox& box, const xVec3& p0, const xVec3& p1, const xVec3& p2);
void xBoxForModel(xBox& o, const xModelInstance* m, bool childModels);
void xBoxForModelLocal(xBox& o, const xModelInstance* m, bool childModels);
void xSphereForModel(xSphere& o, const xModelInstance* model, bool childModels);
U32 xRayHitsModelSingle(const xRay3* r, const xModelInstance* m, xCollis* coll);
U32 xRayHitsModel(const xRay3* r, const xModelInstance* m, xCollis* coll);
U32 xRayHitsEnv(const xRay3* r, const xEnv* env, xCollis* coll);
S32 xSphereHitsModel3(const xSphere* b, const xModelInstance* m, xCollis* colls, U8 ncolls, F32 sth);
S32 xSphereHitsEnv3(const xSphere* b, const xEnv* env, xCollis* colls, U8 ncolls, F32 sth);
U32 xSphereHitsEnv(const xSphere* b, const xEnv* env, xCollis* coll);

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
