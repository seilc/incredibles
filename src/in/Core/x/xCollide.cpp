#include "xCollide.h"

#include "xClumpColl.h"
#include "xDraw.h"
#include "xEnt.h"
#include "xEnv.h"
#include "xGlobals.h"
#include "xJSP.h"
#include "xMath.h"
#include "zGrid.h"
#include "zScene.h"
#include "zSurface.h"

U32 xSphereHitsSphere(const xSphere* a, const xSphere* b, xCollis* coll)
{
    xIsect isx;
    iSphereIsectSphere(b, a, &isx);

    if (!(isx.penned <= 0.0f)) {
        coll->flags &= ~k_HIT_IT;
        return FALSE;
    }

    if (isx.contained <= 0.0f) {
        coll->flags |= 0x10;
    }

    coll->dist = a->r + isx.penned;

    if (coll->flags & 0x1600) {
        if (isx.dist == 0.0f) {
            xVec3Copy(&coll->tohit, &g_O3);
        } else {
            F32 scale = -coll->dist / isx.dist;
            xVec3SMul(&coll->tohit, &isx.norm, scale);
        }
    }

    if (coll->flags & 0x800) {
        if (isx.dist == 0.0f) {
            xVec3Copy(&coll->depen, &g_O3);
        } else {
            F32 scale = -isx.penned / isx.dist;
            xVec3SMul(&coll->depen, &isx.norm, scale);
        }
    }

    if (coll->flags & 0x1200) {
        xVec3Normalize(&coll->hdng, &coll->tohit);
    }

    if (coll->flags & 0x200) {
        xVec3Inv(&coll->norm, &coll->hdng);
    }

    coll->flags |= k_HIT_IT;
    return TRUE;
}

U32 xSphereHitsBox(const xSphere* a, const xBox* b, xCollis* coll)
{
    xIsect isx;
    iBoxIsectSphere(b, a, &isx);

    if (!(isx.penned <= 0.0f)) {
        coll->flags &= ~k_HIT_IT;
        return FALSE;
    }

    if (isx.contained <= 0.0f) {
        coll->flags |= 0x10;
    }

    coll->dist = isx.dist;

    if (coll->flags & 0x400) {
        xVec3Copy(&coll->tohit, &isx.norm);
    }

    if (coll->flags & 0x800) {
        if (isx.dist == 0.0f) {
            xVec3Copy(&coll->depen, &g_O3);
        } else {
            F32 scale = isx.penned / isx.dist;
            xVec3SMul(&coll->depen, &isx.norm, scale);
        }
    }

    if (coll->flags & 0x1200) {
        xVec3Normalize(&coll->hdng, &isx.norm);
    }

    if (coll->flags & 0x200) {
        xVec3Inv(&coll->norm, &coll->hdng);
    }

    coll->flags |= k_HIT_IT;
    return TRUE;
}

// UNUSED: needed to match (code from Scooby PS2)
U32 xSphereHitsOBB(const xSphere* a, const xBox* b, const xMat4x3* m, xCollis* coll)
{
    xSphere mxa;
    F32 mscale = xVec3Length(&m->right);
    xMat3x3 mnormal;
    xMat3x3Normalize(&mnormal, m);
    xMat4x3Tolocal(&mxa.center, m, &m->pos, &a->center);
    mxa.r = a->r / mscale;
    if (!xSphereHitsBox(&mxa, b, coll)) {
        return FALSE;
    }

    if (coll->flags & 0x200) {
        xMat3x3RMulVec(&coll->norm, &mnormal, &coll->norm);
    }

    if (coll->flags & 0x800) {
        xMat3x3RMulVec(&coll->depen, m, &coll->depen);
    }

    if (coll->flags & 0x400) {
        xMat3x3RMulVec(&coll->tohit, m, &coll->tohit);
    }

    if (coll->flags & k_HIT_CALC_HDNG) {
        xMat3x3RMulVec(&coll->hdng, &mnormal, &coll->hdng);
    }

    coll->dist *= mscale;
    return TRUE;
}

U32 xSphereHitsOBB_nu(const xSphere* s, const xBox* b, const xMat4x3* m, xCollis* coll)
{
    xSphere xfs;
    xVec3 scale;
    xMat4x3 mnormal;

    xVec3NormalizeMacro(&mnormal.right, &m->right, &scale.x);
    xVec3NormalizeMacro(&mnormal.up, &m->up, &scale.y);
    xVec3NormalizeMacro(&mnormal.at, &m->at, &scale.z);
    mnormal.pos = m->pos;
    xBox sbox = *b;
    sbox.upper.x *= scale.x, sbox.upper.y *= scale.y, sbox.upper.z *= scale.z;
    sbox.lower.x *= scale.x, sbox.lower.y *= scale.y, sbox.lower.z *= scale.z;
    xMat4x3Tolocal(&xfs.center, &mnormal, &s->center);
    xfs.r = s->r;
    xSphereHitsBox(&xfs, &sbox, coll);

    if (!(coll->flags & k_HIT_IT)) {
        return FALSE;
    }

    if (coll->flags & 0x200) {
        xMat3x3RMulVec(&coll->norm, &mnormal, &coll->norm);
    }

    if (coll->flags & 0x800) {
        xMat3x3RMulVec(&coll->depen, &mnormal, &coll->depen);
    }

    if (coll->flags & 0x400) {
        xMat3x3RMulVec(&coll->tohit, &mnormal, &coll->tohit);
    }

    if (coll->flags & k_HIT_CALC_HDNG) {
        xMat3x3RMulVec(&coll->hdng, &mnormal, &coll->hdng);
    }

    return TRUE;
}

enum Dimension
{
    XDIM,
    YDIM,
    ZDIM
};

static S32 PointWithinTriangle(xVec3* _pt, xVec3** _tri, xVec3* _normal)
{
    RwV3d* pt = (RwV3d*)_pt;
    RwV3d** tri = (RwV3d**)_tri;
    RwV3d* normal = (RwV3d*)_normal;
    Dimension dimension;
    F32 absX = xabs(normal->x);
    F32 absY = xabs(normal->y);
    F32 absZ = xabs(normal->z);
    S32 inside = 0;
    S32 i, j;

    dimension = (absZ > absY) ? ((absZ > absX) ? ZDIM : XDIM) : ((absY > absX) ? YDIM : XDIM);

    switch (dimension) {
    case XDIM:
        for (i = 0, j = 2; i < 3; j = i++) {
            if (((tri[i]->y <= pt->y && pt->y < tri[j]->y) || (tri[j]->y <= pt->y && pt->y < tri[i]->y)) &&
                pt->z < (tri[j]->z - tri[i]->z) * (pt->y - tri[i]->y) / (tri[j]->y - tri[i]->y) + tri[i]->z) {
                inside = !inside;
            }
        }
        break;
    case YDIM:
        for (i = 0, j = 2; i < 3; j = i++) {
            if (((tri[i]->z <= pt->z && pt->z < tri[j]->z) || (tri[j]->z <= pt->z && pt->z < tri[i]->z)) &&
                pt->x < (tri[j]->x - tri[i]->x) * (pt->z - tri[i]->z) / (tri[j]->z - tri[i]->z) + tri[i]->x) {
                inside = !inside;
            }
        }
        break;
    case ZDIM:
        for (i = 0, j = 2; i < 3; j = i++) {
            if (((tri[i]->y <= pt->y && pt->y < tri[j]->y) || (tri[j]->y <= pt->y && pt->y < tri[i]->y)) &&
                pt->x < (tri[j]->x - tri[i]->x) * (pt->y - tri[i]->y) / (tri[j]->y - tri[i]->y) + tri[i]->x) {
                inside = !inside;
            }
        }
        break;
    }

    return inside;
}

static void FindNearestPointOnLine(xVec3* result, xVec3* point, xVec3* start, xVec3* end)
{
    RwReal mu;
    RwV3d line, candidate;

    RwV3dSub(&line, (RwV3d*)end, (RwV3d*)start);

    mu = RwV3dDotProduct((RwV3d*)point, &line) - RwV3dDotProduct((RwV3d*)start, &line);

    if (mu <= 0.0f) {
        candidate = *(RwV3d*)start;
    } else {
        RwReal lineLength2 = RwV3dDotProduct(&line, &line);

        if (mu < lineLength2) {
            mu /= lineLength2;

            RwV3dScale(&candidate, &line, mu);
            RwV3dAdd(&candidate, &candidate, (RwV3d*)start);
        } else {
            candidate = *(RwV3d*)end;
        }
    }

    *(RwV3d*)result = candidate;
}

static void properSphereIsectTri(const xVec3* center, F32 radius, xVec3* tohit, F32* dist_ptr, RpCollisionTriangle* tri)
{
    xVec3 projPoint;
    F32 dist2plane, dist = *dist_ptr;

    dist2plane = xVec3Dot((xVec3*)&tri->normal, (xVec3*)&tri->point) - xVec3Dot((xVec3*)&tri->normal, center);

    xVec3SMul(&projPoint, (xVec3*)&tri->normal, dist2plane);

    xVec3Copy(tohit, &projPoint);

    xVec3AddTo(&projPoint, center);

    if (PointWithinTriangle(&projPoint, (xVec3**)tri->vertices, (xVec3*)&tri->normal)) {
        dist = xabs(dist2plane);
    } else {
        F32 dist2 = xsqr(dist);
        F32 radius2 = xsqr(radius);

        for (U32 i = 0; i < 3; i++) {
            xVec3 vertClosestPoint;
            F32 vertDist2;

            FindNearestPointOnLine(&vertClosestPoint, &projPoint, (xVec3*)tri->vertices[i], (xVec3*)tri->vertices[(i + 1) % 3]);

            xVec3 temp;
            xVec3Sub(&temp, &vertClosestPoint, center);
            vertDist2 = xVec3Length2(&temp);

            if (vertDist2 < dist2 && vertDist2 < radius2) {
                dist2 = vertDist2;

                xVec3Copy(tohit, &temp);
            }
        }

        dist = xsqrt(dist2);
    }

    *dist_ptr = dist;
}

static S32 sCollidingJSP;

struct xSphereHitsModel_context
{
    xCollis* coll;
    RpIntersection localx;
};

static RpCollisionTriangle* sphereHitsEnvCB(RpIntersection* isx, RpWorldSector* sector, RpCollisionTriangle* tri, F32 dist, void* data)
{
    xCollis* coll = (xCollis*)data;
    F32 radius = isx->t.sphere.radius;
    xVec3* center = (xVec3*)&isx->t.sphere.center;

    if (coll->flags == 0) {
        coll->flags |= k_HIT_IT;
        return NULL;
    }

    xVec3 tohit;
    dist = HUGE;

    properSphereIsectTri(center, radius, &tohit, &dist, tri);

    if (dist < 0.0f) {
        return tri;
    }

    if (dist == 0.0f) {
        return tri;
    }

    if (dist >= radius) {
        return tri;
    }

    if (dist >= coll->dist) {
        return tri;
    }

    coll->dist = dist;

    if (coll->flags & 0x200) {
        xVec3Copy(&coll->norm, (xVec3*)&tri->normal);
    }

    if (coll->flags & 0x400) {
        xVec3Copy(&coll->tohit, &tohit);
    }

    if (coll->flags & 0x800) {
        F32 scale = 1.0f - radius / coll->dist;
        scale -= 1e-5f;

        xVec3Copy(&coll->depen, &tohit);
        xVec3SMulBy(&coll->depen, scale);
    }

    if (coll->flags & k_HIT_CALC_HDNG) {
        xVec3Normalize(&coll->hdng, &tohit);
    }

    if (sector) {
        coll->oid = sector->triangles[tri->index].matIndex;
    } else if (sCollidingJSP) {
        coll->oid = ((xClumpCollBSPTriangle*)tri->index)->matIndex;
    } else {
        coll->oid = tri->index;
    }

    coll->flags |= k_HIT_IT;
    return tri;
}

static RpCollisionTriangle* sphereHitsModelCB(RpIntersection*, RpCollisionTriangle* tri, F32 dist, void* data)
{
    xSphereHitsModel_context* context = (xSphereHitsModel_context*)data; return sphereHitsEnvCB(&context->localx, NULL, tri, dist, context->coll);
}

U32 xSphereHitsModel(const xSphere* b, const xModelInstance* m, xCollis* coll)
{
    RpIntersection isx;
    xSphereHitsModel_context context;
    context.coll = coll;

    if (m->Flags & 0x800) xModelAnimCollApply(*m);

    isx.type = rpINTERSECTSPHERE;
    *(xSphere*)&isx.t.sphere = *b;

    xMat4x3* mat = (xMat4x3*)m->Mat;
    RwFrame* frame = RpAtomicGetFrame(m->Data);

    RwFrameTransform(frame, (RwMatrix*)mat, rwCOMBINEREPLACE);

    F32 mscale = xVec3Length(&mat->right);

    xMat4x3Tolocal((xVec3*)&context.localx.t.sphere.center, mat, &b->center);
    context.localx.t.sphere.radius = b->r / mscale;

    coll->flags &= ~k_HIT_IT;
    coll->dist = HUGE;

    if (coll->flags & k_HIT_CALC_TRI) {
        coll->flags |= 0x400;
    }

    RpAtomicForAllIntersections(m->Data, &isx, sphereHitsModelCB, &context);

    if ((coll->flags & k_HIT_CALC_TRI) && (coll->flags & k_HIT_IT)) {
        xCollideCalcTri(coll->tri, *m, *(xVec3*)&context.localx.t.sphere.center, coll->tohit);
    }

    if (coll->flags & 0x400)
        xMat3x3RMulVec(&coll->tohit, mat, &coll->tohit);

    if (coll->flags & 0x800)
        xMat3x3RMulVec(&coll->depen, mat, &coll->depen);

    if (coll->flags & k_HIT_CALC_HDNG)
        xMat3x3RMulVec(&coll->hdng, mat, &coll->hdng);

    if (coll->flags & 0x200) {
        xMat3x3RMulVec(&coll->norm, mat, &coll->norm);

        F32 mag2 = coll->norm.length2();

        if (mag2 && !xeq(mag2, 1.0f, 1e-5f)) {
            coll->norm *= 1.0f / xsqrt(mag2);
        }
    }

    coll->dist *= mscale;

    if (m->Flags & 0x800) xModelAnimCollRestore(*m);

    return coll->flags & k_HIT_IT;
}

void xParabolaRecenter(xParabola* p, F32 newZeroT)
{
    xVec3 newPos, newVel;

    xParabolaEvalPos(p, &newPos, newZeroT);
    xParabolaEvalVel(p, &newVel, newZeroT);
    xVec3Copy(&p->initPos, &newPos);
    xVec3Copy(&p->initVel, &newVel);
    p->maxTime -= newZeroT;
    p->minTime -= newZeroT;
}

struct ParabolaCBData
{
    xParabola* p;
    xVec3 N;
    F32 d;
    xCollis* colls;
};

static S32 xParabolaEnvCB(xClumpCollBSPTriangle* triangles, void* data)
{
    ParabolaCBData* pd = (ParabolaCBData*)data;
    xParabola* p = pd->p;
    xCollis* colls = pd->colls;

    do {
        if (xClumpColl_IsCollidableTriangle(triangles->flags)) {
            xVec3 *v0, *v1, *v2;
            U8 tester;
            F32 a, b, c, det;
            xVec3 v1p, v2p, N;
            F32 pdist;
            F32 t1, t2;
            xVec3 s1, s2;
            F32 c1, c2, c0;
            F32 d11, d12, d22;
            F32 p1, p2;
            xVec3 pp;

            v0 = (xVec3*)&triangles->v.p[0];
            if (triangles->flags & 0x2) {
                v1 = (xVec3*)&triangles->v.p[2];
                v2 = (xVec3*)&triangles->v.p[1];
            } else {
                v1 = (xVec3*)&triangles->v.p[1];
                v2 = (xVec3*)&triangles->v.p[2];
            }

            tester = 0;
            if (xVec3Dot(&pd->N, v0) - pd->d > 0.0f) tester |= 0x1;
            else tester |= 0x2;
            if (xVec3Dot(&pd->N, v1) - pd->d > 0.0f) tester |= 0x1;
            else tester |= 0x2;
            if (xVec3Dot(&pd->N, v2) - pd->d > 0.0f) tester |= 0x1;
            else tester |= 0x2;

            if ((tester & 0x1) && (tester & 0x2)) {
                t1 = -1.0f;
                t2 = -1.0f;

                xVec3Sub(&v1p, v1, v0);
                xVec3Sub(&v2p, v2, v0);
                xVec3Cross(&N, &v1p, &v2p);
                if (xVec3Normalize(&N, &N)) {
                    pdist = -xVec3Dot(&N, v0);

                    a = 0.5f * (-N.y * p->gravity);
                    b = N.x * p->initVel.x + N.y * p->initVel.y + N.z * p->initVel.z;
                    c = pdist + (N.x * p->initPos.x + N.y * p->initPos.y + N.z * p->initPos.z);

                    if (a < 1e-5f && a > -1e-5f) {
                        if (b > 1e-5f || b < -1e-5f) {
                            t1 = -c / b;
                        }
                    } else {
                        det = xsqr(b) - 4.0f * a * c;
                        if (det > 0.0f) {
                            t1 = (-b + xsqrt(det)) / (2.0f * a);
                            t2 = (-b - xsqrt(det)) / (2.0f * a);
                        } else if (det == 0.0f) {
                            t1 = -b / a;
                        }
                    }

                    d11 = xVec3Dot(&v1p, &v1p);
                    d12 = xVec3Dot(&v1p, &v2p);
                    d22 = xVec3Dot(&v2p, &v2p);

                    if (t1 >= p->minTime && t1 < p->maxTime && t1 < pd->colls->dist) {
                        xVec3Copy(&s1, &p->initPos);
                        xVec3AddScaled(&s1, &p->initVel, t1);
                        s1.y -= 0.5f * p->gravity * t1 * t1;

                        xVec3Sub(&pp, &s1, v0);
                        p1 = xVec3Dot(&pp, &v1p);
                        p2 = xVec3Dot(&pp, &v2p);

                        c1 = (p1 * d22 - p2 * d12) / (d11 * d22 - xsqr(d12));
                        c2 = (p2 * d11 - p1 * d12) / (d11 * d22 - xsqr(d12));
                        c0 = 1.0f - c1 - c2;

                        if (c1 >= 0.0f && c2 >= 0.0f && c0 >= 0.0f) {
                            colls->flags |= k_HIT_IT;
                            pd->colls->dist = t1;
                            xVec3Copy(&pd->colls->norm, &N);
                        }
                    }

                    if (t2 >= p->minTime && t2 < p->maxTime && t2 < pd->colls->dist) {
                        xVec3Copy(&s2, &p->initPos);
                        xVec3AddScaled(&s2, &p->initVel, t2);
                        s2.y -= 0.5f * p->gravity * t2 * t2;

                        xVec3Sub(&pp, &s2, v0);
                        p1 = xVec3Dot(&pp, &v1p);
                        p2 = xVec3Dot(&pp, &v2p);

                        c1 = (p1 * d22 - p2 * d12) / (d11 * d22 - xsqr(d12));
                        c2 = (p2 * d11 - p1 * d12) / (d11 * d22 - xsqr(d12));
                        c0 = 1.0f - c1 - c2;

                        if (c1 >= 0.0f && c2 >= 0.0f && c0 >= 0.0f) {
                            colls->flags |= k_HIT_IT;
                            pd->colls->dist = t2;
                            xVec3Copy(&pd->colls->norm, &N);
                        }
                    }
                }
            }
        }
    } while (triangles++->flags & 0x1);

    return TRUE;
}

S32 xParabolaHitsEnv(xParabola* p, const xEnv* env, xCollis* colls)
{
    RwBBox xb;
    F32 tmp;

    if (p->minTime >= p->maxTime) return FALSE;

    colls->flags = 0;
    colls->dist = HUGE;

    xb.inf.x = p->initPos.x + p->initVel.x * p->minTime;
    xb.sup.x = p->initPos.x + p->initVel.x * p->maxTime;
    if (xb.inf.x > xb.sup.x) {
        tmp = xb.inf.x;
        xb.inf.x = xb.sup.x;
        xb.sup.x = tmp;
    }

    xb.inf.z = p->initPos.z + p->initVel.z * p->minTime;
    xb.sup.z = p->initPos.z + p->initVel.z * p->maxTime;
    if (xb.inf.z > xb.sup.z) {
        tmp = xb.inf.z;
        xb.inf.z = xb.sup.z;
        xb.sup.z = tmp;
    }

    xb.inf.y = p->initPos.y + p->initVel.y * p->minTime;
    xb.inf.y -= 0.5f * p->gravity * p->minTime * p->minTime;
    xb.sup.y = p->initPos.y + p->initVel.y * p->maxTime;
    xb.sup.y -= 0.5f * p->gravity * p->maxTime * p->maxTime;
    if (xb.inf.y > xb.sup.y) {
        tmp = xb.inf.y;
        xb.inf.y = xb.sup.y;
        xb.sup.y = tmp;
    }

    if (p->gravity > 1e-5f || p->gravity < -1e-5f) {
        F32 extremumT = p->initVel.y / p->gravity;
        if (p->minTime < extremumT && extremumT < p->maxTime) {
            F32 extremum = p->initPos.y + extremumT * (p->initVel.y - 0.5f * p->gravity * extremumT);

            if (extremum < xb.inf.y) {
                xb.inf.y = extremum;
            } else if (extremum > xb.sup.y) {
                xb.sup.y = extremum;
            }
        }
    }

    ParabolaCBData data;
    data.p = p;

    data.N.x = -p->initVel.z;
    data.N.y = 0.0f;
    data.N.z = p->initVel.x;

    if (data.N.x < 1e-5f && data.N.x > -1e-5f && data.N.z < 1e-5f && data.N.z > -1e-5f) {
        data.N.x = 1.0f;
        data.N.z = 0.0f;
    }

    data.d = xVec3Dot(&data.N, &p->initPos);

    data.colls = colls;

    for (S32 i = 0; i < env->geom->jsp_count; i++) {
        if (xEnvIsJSPActive(env, i)) {
            data.colls->test_dist = data.colls->dist;
            xClumpColl_ForAllBoxLeafNodeIntersections(env->geom->jsp_list[i]->colltree, &xb, xParabolaEnvCB, &data);
            if (data.colls->dist < data.colls->test_dist) {
                data.colls->oid = get_matid(env->geom->jsp_list[i], data.colls->oid, 0);
            }
        }
    }

    if (colls->flags & k_HIT_IT) {
        return TRUE;
    }

    return FALSE;
}

static const xCapsule* cb_capsule;

static RpCollisionTriangle* CapsuleEnvClosestToLineCB(RpIntersection*, RpWorldSector*, RpCollisionTriangle* tri, F32, void* data)
{
    xCollis* colls = (xCollis*)data;

    const xVec3* p1 = &cb_capsule->start;
    const xVec3* p2 = &cb_capsule->end;
    const xVec3* v0 = (const xVec3*)tri->vertices[0];
    const xVec3* v1 = (const xVec3*)tri->vertices[1];
    const xVec3* v2 = (const xVec3*)tri->vertices[2];

    if ((v0->x == v1->x && v0->y == v1->y && v0->z == v1->z) ||
        (v0->x == v2->x && v0->y == v2->y && v0->z == v2->z) ||
        (v2->x == v1->x && v2->y == v1->y && v2->z == v1->z)) {
        return tri;
    }

    F32 t, u, v;
    xLine3TriMinDist(p1, p2, v0, v1, v2, t, u, v);

    F32 ln_ptx, ln_pty, ln_ptz;
    ln_ptx = p1->x * (1.0f - t) + p2->x * t, ln_pty = p1->y * (1.0f - t) + p2->y * t, ln_ptz = p1->z * (1.0f - t) + p2->z * t;
    F32 tri_ptx, tri_pty, tri_ptz;
    tri_ptx = v0->x * (1.0f - u) + v1->x * u, tri_pty = v0->y * (1.0f - u) + v1->y * u, tri_ptz = v0->z * (1.0f - u) + v1->z * u;
    F32 tempx, tempy, tempz;
    tempx = v2->x - v0->x, tempy = v2->y - v0->y, tempz = v2->z - v0->z;
    tri_ptx += tempx * v, tri_pty += tempy * v, tri_ptz += tempz * v;
    F32 tohitx, tohity, tohitz;
    tohitx = tri_ptx - ln_ptx, tohity = tri_pty - ln_pty, tohitz = tri_ptz - ln_ptz;
    F32 d2 = xsqr(tohitx) + xsqr(tohity) + xsqr(tohitz);

    if (d2 > xsqr(cb_capsule->r)) {
        return tri;
    }

    if (d2 > colls->dist) {
        return tri;
    }

    colls->dist = d2;

    if (xabs(d2) <= 1e-5f && t > colls->tuv.t) {
        return tri;
    }

    colls->oid = tri->index;
    colls->tri.index = tri->index;
    colls->tuv.t = t;
    colls->tuv.u = u;
    colls->tuv.v = v;
    colls->tohit.x = tohitx, colls->tohit.y = tohity, colls->tohit.z = tohitz;
    colls->norm.x = tri->normal.x, colls->norm.y = tri->normal.y, colls->norm.z = tri->normal.z;

    return tri;
}

static RpCollisionTriangle* CapsuleEnvClosestToStartCB(RpIntersection*, RpWorldSector*, RpCollisionTriangle* tri, F32, void* data); // UNUSED

static RpCollisionTriangle* CapsuleModelClosestToLineCB(RpIntersection*, RpCollisionTriangle* tri, F32 dist, void* data)
{
    return CapsuleEnvClosestToLineCB(NULL, NULL, tri, dist, data);
}

static S32 LeafNodeCapsuleIntersect(xClumpCollBSPTriangle* triangles, void* data)
{
    CallBackParam* cbParam = (CallBackParam*)data;

    do {
        if (xClumpColl_IsCollidableTriangle(triangles->flags)) {
            RwV3d *v0, *v1, *v2;
            RpCollisionTriangle collisionTri;

            v0 = &triangles->v.p[0];
            if (triangles->flags & 0x2) {
                v1 = &triangles->v.p[2];
                v2 = &triangles->v.p[1];
            } else {
                v1 = &triangles->v.p[1];
                v2 = &triangles->v.p[2];
            }

            collisionTri.point = *v0;
            collisionTri.index = (RwInt32)triangles;
            xClumpCollTriangleNormalMacro(&collisionTri.normal, v0, v1, v2);
            collisionTri.vertices[0] = v0;
            collisionTri.vertices[1] = v1;
            collisionTri.vertices[2] = v2;

            if (!cbParam->u.worldCB(NULL, NULL, &collisionTri, 0.0f, cbParam->data)) {
                return FALSE;
            }
        }
    } while (triangles++->flags & 0x1);

    return TRUE;
}

S32 xCapsuleIsectsModelClosestToLine(const xCapsule* c, const xModelInstance* model, xCollis* colls)
{
    if (model->Flags & 0x800) xModelAnimCollApply(*model);

    RpIntersection isx;
    xSphere cbound;
    cbound.center.x = c->start.x + c->end.x, cbound.center.y = c->start.y + c->end.y, cbound.center.z = c->start.z + c->end.z;
    cbound.center.x *= 0.5f, cbound.center.y *= 0.5f, cbound.center.z *= 0.5f;
    xVec3DistMacro(&c->start, &c->end, &cbound.r);
    cbound.r *= 0.5f;
    cbound.r += c->r;

    isx.type = rpINTERSECTSPHERE;
    *(xSphere*)&isx.t.sphere = cbound;

    xMat4x3& mat = *(xMat4x3*)model->Mat;
    xMat3x3 old_mat = mat;

    if (model->Scale.x != 0.0f) {
        xMat3x3MulScale(&mat, &mat, (xVec3*)&model->Scale);
    }

    F32 mscale = mat.right.length();

    RwFrame* frame = RpAtomicGetFrame(model->Data);

    RwFrameTransform(frame, (RwMatrix*)&mat, rwCOMBINEREPLACE);

    xCapsule lcap;
    cb_capsule = &lcap;
    xMat4x3Tolocal(&lcap.start, &mat, &c->start);
    xMat4x3Tolocal(&lcap.end, &mat, &c->end);

    lcap.r = c->r / mscale;

    colls->dist = HUGE;
    colls->tuv.t = HUGE;
    RpAtomicForAllIntersections(model->Data, &isx, CapsuleModelClosestToLineCB, colls);

    if (model->Flags & 0x800) xModelAnimCollRestore(*model);

    if (colls->dist < HUGE) {
        colls->flags |= k_HIT_IT;
        colls->optr = NULL;
        colls->mptr = NULL;
        colls->dist = xsqrt(colls->dist);
        colls->dist *= mscale;
        xMat3x3RMulVec(&colls->tohit, &mat, &colls->tohit);
        if (xabs(colls->dist) <= 1e-5f) {
            colls->dist = 1e-5f;
        }

        F32 s = 1.0f / colls->dist;
        colls->hdng.x = colls->tohit.x * s, colls->hdng.y = colls->tohit.y * s, colls->hdng.z = colls->tohit.z * s;
        s *= colls->dist - c->r;
        colls->depen.x = colls->tohit.x * s, colls->depen.y = colls->tohit.y * s, colls->depen.z = colls->tohit.z * s;
        (xMat3x3&)mat = old_mat;
        return TRUE;
    }

    colls->flags &= ~k_HIT_IT;
    (xMat3x3&)mat = old_mat;
    return FALSE;
}

S32 xCapsuleIsectsEnvClosestToLine(const xCapsule* c, const xEnv* env, xCollis* colls)
{
    cb_capsule = c;
    colls->dist = HUGE;
    colls->tuv.t = HUGE;

    RwV3d delta;
    xClumpCollV3dGradient grad;
    CallBackParam param;

    RwV3dSub(&delta, (const RwV3d*)&c->end, (const RwV3d*)&c->start);
    xClumpCollV3dGradientMacro(&grad, &delta);
    param.u.worldCB = CapsuleEnvClosestToLineCB;
    param.data = colls;

    for (S32 i = 0; i < env->geom->jsp_count; i++) {
        if (xEnvIsJSPActive(env, i)) {
            colls->test_dist = colls->dist;

            xClumpColl_ForAllCapsuleLeafNodeIntersections(env->geom->jsp_list[i]->colltree, (RwLine*)c, c->r, &grad, LeafNodeCapsuleIntersect, &param);

            if (colls->dist < colls->test_dist) {
                colls->oid = get_matid(env->geom->jsp_list[i], colls->oid, 0);
            }
        }
    }

    if (colls->dist < HUGE) {
        colls->flags |= k_HIT_IT;
        colls->optr = NULL;
        colls->mptr = NULL;
        if (colls->dist > 0.0f) {
            colls->dist = xsqrt(colls->dist);
            F32 s = 1.0f / colls->dist;
            colls->hdng.x = colls->tohit.x * s, colls->hdng.y = colls->tohit.y * s, colls->hdng.z = colls->tohit.z * s;
            s *= colls->dist - c->r;
            colls->depen.x = colls->tohit.x * s, colls->depen.y = colls->tohit.y * s, colls->depen.z = colls->tohit.z * s;
        }

        return TRUE;
    }

    colls->flags &= ~k_HIT_IT;
    return FALSE;
}

static const xCapsule* cb_cap;
S32 xcollide_docam;

// UNUSED: needed to match (speculative code)
void xCapsuleHitsEnt(xScene*, xRay3*, xQCData*, xEnt* ent, void* data)
{
    xCollis* coll = (xCollis*)data;

    xCollis c;
    c.flags = 0;

    if (!xCapsuleIsectsModelClosestToLine(cb_cap, xcollide_docam ? ent->camcollModel : ent->model, &c)) {
        return;
    }

    if (c.dist < coll->dist) {
        *coll = c;
        coll->optr = ent;
    }
}

// UNUSED: needed to match (speculative code)
S32 xCapsuleIsectsSceneClosestToLine(const xCapsule* c, xScene* sc, xCollis* colls)
{
    xCapsuleIsectsEnvClosestToLine(c, sc->env, colls);

    cb_cap = c;

    xLine3 ln;
    ln.p1 = c->start;
    ln.p2 = c->end;

    xRay3 ray;
    xVec3Copy(&ray.origin, &ln.p1);
    xVec3Sub(&ray.dir, &ln.p2, &ln.p1);
    ray.max_t = xVec3Length(&ray.dir);
    F32 one_len = 1.0f / MAX(ray.max_t, 0.00001f);
    xVec3SMul(&ray.dir, &ray.dir, one_len);
    ray.flags = XRAY3_USE_MAX;

    xQCData qcr;
    xQuickCullForRay(&qcr, &ray);

    xRayHitsGrid(&colls_grid, sc, &ray, xCapsuleHitsEnt, &qcr, colls);
    xRayHitsGrid(&colls_oso_grid, sc, &ray, xCapsuleHitsEnt, &qcr, colls);

    return colls->flags & k_HIT_IT;
}

U32 xBoxHitsSphere(const xBox* a, const xSphere* b, xCollis* coll)
{
    xIsect isx;
    iBoxIsectSphere(a, b, &isx);

    if (!(isx.penned <= 0.0f)) {
        coll->flags &= ~k_HIT_IT;
        return FALSE;
    }

    if (isx.contained <= 0.0f) {
        coll->flags |= 0x10;
    }

    coll->dist = isx.dist;

    if (coll->flags & 0x400) {
        xVec3Copy(&coll->tohit, &isx.norm);
    }

    if (coll->flags & 0x800) {
        if (isx.dist == 0.0f) {
            xVec3Copy(&coll->depen, &g_O3);
        } else {
            F32 scale = isx.penned / isx.dist;
            xVec3SMul(&coll->depen, &isx.norm, scale);
        }
    }

    if (coll->flags & 0x1200) {
        xVec3Normalize(&coll->hdng, &isx.norm);
    }

    if (coll->flags & 0x200) {
        xVec3Inv(&coll->norm, &coll->hdng);
    }

    coll->flags |= k_HIT_IT;
    return TRUE;
}

#define NORMALIZE_AXIS(axis, extent)                                                              \
{                                                                                                 \
    F32 _mag = xsqr((axis).x) + xsqr((axis).y) + xsqr((axis).z);                                  \
    if (xabs(_mag - 1.0f) > 1e-6f && _mag > 1e-5f) {                                              \
        _mag = xsqrt(_mag);                                                                       \
        (extent) *= _mag;                                                                         \
        _mag = 1.0f / _mag;                                                                       \
        (axis).x *= _mag;                                                                         \
        (axis).y *= _mag;                                                                         \
        (axis).z *= _mag;                                                                         \
    }                                                                                             \
}

static U32 Mgc_BoxBoxTest(const xBox* a, const xMat4x3* matA, const xBox* b, const xMat4x3* matB)
{
    xVec3 akA[3], akB[3];
    F32 afEA[3], afEB[3];

    afEA[0] = 0.5f * (a->upper.x - a->lower.x);
    afEA[1] = 0.5f * (a->upper.y - a->lower.y);
    afEA[2] = 0.5f * (a->upper.z - a->lower.z);
    akA[0] = matA->right;
    akA[1] = matA->up;
    akA[2] = matA->at;
    NORMALIZE_AXIS(akA[0], afEA[0]);
    NORMALIZE_AXIS(akA[1], afEA[1]);
    NORMALIZE_AXIS(akA[2], afEA[2]);

    afEB[0] = 0.5f * (b->upper.x - b->lower.x);
    afEB[1] = 0.5f * (b->upper.y - b->lower.y);
    afEB[2] = 0.5f * (b->upper.z - b->lower.z);
    akB[0] = matB->right;
    akB[1] = matB->up;
    akB[2] = matB->at;
    NORMALIZE_AXIS(akB[0], afEB[0]);
    NORMALIZE_AXIS(akB[1], afEB[1]);
    NORMALIZE_AXIS(akB[2], afEB[2]);

    xVec3 centA, centB;
    centA.x = 0.5f * (a->upper.x + a->lower.x);
    centA.y = 0.5f * (a->upper.y + a->lower.y);
    centA.z = 0.5f * (a->upper.z + a->lower.z);
    centB.x = 0.5f * (b->upper.x + b->lower.x);
    centB.y = 0.5f * (b->upper.y + b->lower.y);
    centB.z = 0.5f * (b->upper.z + b->lower.z);
    xMat4x3Toworld(&centA, matA, &centA);
    xMat4x3Toworld(&centB, matB, &centB);

    xVec3 kD;
    kD.x = centB.x - centA.x;
    kD.y = centB.y - centA.y;
    kD.z = centB.z - centA.z;

    F32 aafC[3][3], aafAbsC[3][3], afAD[3];
    F32 fR0, fR1, fR, fR01;
    aafC[0][0] = akA[0].x * akB[0].x + akA[0].y * akB[0].y + akA[0].z * akB[0].z;
    aafC[0][1] = akA[0].x * akB[1].x + akA[0].y * akB[1].y + akA[0].z * akB[1].z;
    aafC[0][2] = akA[0].x * akB[2].x + akA[0].y * akB[2].y + akA[0].z * akB[2].z;
    afAD[0] = akA[0].x * kD.x + akA[0].y * kD.y + akA[0].z * kD.z;
    aafAbsC[0][0] = xabs(aafC[0][0]);
    aafAbsC[0][1] = xabs(aafC[0][1]);
    aafAbsC[0][2] = xabs(aafC[0][2]);
    fR = xabs(afAD[0]);
    fR1 = afEB[0] * aafAbsC[0][0] + afEB[1] * aafAbsC[0][1] + afEB[2] * aafAbsC[0][2];
    fR01 = afEA[0] + fR1;
    if (fR > fR01) {
        return FALSE;
    }

    aafC[1][0] = akA[1].x * akB[0].x + akA[1].y * akB[0].y + akA[1].z * akB[0].z;
    aafC[1][1] = akA[1].x * akB[1].x + akA[1].y * akB[1].y + akA[1].z * akB[1].z;
    aafC[1][2] = akA[1].x * akB[2].x + akA[1].y * akB[2].y + akA[1].z * akB[2].z;
    afAD[1] = akA[1].x * kD.x + akA[1].y * kD.y + akA[1].z * kD.z;
    aafAbsC[1][0] = xabs(aafC[1][0]);
    aafAbsC[1][1] = xabs(aafC[1][1]);
    aafAbsC[1][2] = xabs(aafC[1][2]);
    fR = xabs(afAD[1]);
    fR1 = afEB[0] * aafAbsC[1][0] + afEB[1] * aafAbsC[1][1] + afEB[2] * aafAbsC[1][2];
    fR01 = afEA[1] + fR1;
    if (fR > fR01) {
        return FALSE;
    }

    aafC[2][0] = akA[2].x * akB[0].x + akA[2].y * akB[0].y + akA[2].z * akB[0].z;
    aafC[2][1] = akA[2].x * akB[1].x + akA[2].y * akB[1].y + akA[2].z * akB[1].z;
    aafC[2][2] = akA[2].x * akB[2].x + akA[2].y * akB[2].y + akA[2].z * akB[2].z;
    afAD[2] = akA[2].x * kD.x + akA[2].y * kD.y + akA[2].z * kD.z;
    aafAbsC[2][0] = xabs(aafC[2][0]);
    aafAbsC[2][1] = xabs(aafC[2][1]);
    aafAbsC[2][2] = xabs(aafC[2][2]);
    fR = xabs(afAD[2]);
    fR1 = afEB[0] * aafAbsC[2][0] + afEB[1] * aafAbsC[2][1] + afEB[2] * aafAbsC[2][2];
    fR01 = afEA[2] + fR1;
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(akB[0].x * kD.x + akB[0].y * kD.y + akB[0].z * kD.z);
    fR0 = afEA[0] * aafAbsC[0][0] + afEA[1] * aafAbsC[1][0] + afEA[2] * aafAbsC[2][0];
    fR01 = fR0 + afEB[0];
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(akB[1].x * kD.x + akB[1].y * kD.y + akB[1].z * kD.z);
    fR0 = afEA[0] * aafAbsC[0][1] + afEA[1] * aafAbsC[1][1] + afEA[2] * aafAbsC[2][1];
    fR01 = fR0 + afEB[1];
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(akB[2].x * kD.x + akB[2].y * kD.y + akB[2].z * kD.z);
    fR0 = afEA[0] * aafAbsC[0][2] + afEA[1] * aafAbsC[1][2] + afEA[2] * aafAbsC[2][2];
    fR01 = fR0 + afEB[2];
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(afAD[2] * aafC[1][0] - afAD[1] * aafC[2][0]);
    fR0 = afEA[1] * aafAbsC[2][0] + afEA[2] * aafAbsC[1][0];
    fR1 = afEB[1] * aafAbsC[0][2] + afEB[2] * aafAbsC[0][1];
    fR01 = fR0 + fR1;
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(afAD[2] * aafC[1][1] - afAD[1] * aafC[2][1]);
    fR0 = afEA[1] * aafAbsC[2][1] + afEA[2] * aafAbsC[1][1];
    fR1 = afEB[0] * aafAbsC[0][2] + afEB[2] * aafAbsC[0][0];
    fR01 = fR0 + fR1;
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(afAD[2] * aafC[1][2] - afAD[1] * aafC[2][2]);
    fR0 = afEA[1] * aafAbsC[2][2] + afEA[2] * aafAbsC[1][2];
    fR1 = afEB[0] * aafAbsC[0][1] + afEB[1] * aafAbsC[0][0];
    fR01 = fR0 + fR1;
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(afAD[0] * aafC[2][0] - afAD[2] * aafC[0][0]);
    fR0 = afEA[0] * aafAbsC[2][0] + afEA[2] * aafAbsC[0][0];
    fR1 = afEB[1] * aafAbsC[1][2] + afEB[2] * aafAbsC[1][1];
    fR01 = fR0 + fR1;
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(afAD[0] * aafC[2][1] - afAD[2] * aafC[0][1]);
    fR0 = afEA[0] * aafAbsC[2][1] + afEA[2] * aafAbsC[0][1];
    fR1 = afEB[0] * aafAbsC[1][2] + afEB[2] * aafAbsC[1][0];
    fR01 = fR0 + fR1;
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(afAD[0] * aafC[2][2] - afAD[2] * aafC[0][2]);
    fR0 = afEA[0] * aafAbsC[2][2] + afEA[2] * aafAbsC[0][2];
    fR1 = afEB[0] * aafAbsC[1][1] + afEB[1] * aafAbsC[1][0];
    fR01 = fR0 + fR1;
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(afAD[1] * aafC[0][0] - afAD[0] * aafC[1][0]);
    fR0 = afEA[0] * aafAbsC[1][0] + afEA[1] * aafAbsC[0][0];
    fR1 = afEB[1] * aafAbsC[2][2] + afEB[2] * aafAbsC[2][1];
    fR01 = fR0 + fR1;
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(afAD[1] * aafC[0][1] - afAD[0] * aafC[1][1]);
    fR0 = afEA[0] * aafAbsC[1][1] + afEA[1] * aafAbsC[0][1];
    fR1 = afEB[0] * aafAbsC[2][2] + afEB[2] * aafAbsC[2][0];
    fR01 = fR0 + fR1;
    if (fR > fR01) {
        return FALSE;
    }

    fR = xabs(afAD[1] * aafC[0][2] - afAD[0] * aafC[1][2]);
    fR0 = afEA[0] * aafAbsC[1][2] + afEA[1] * aafAbsC[0][2];
    fR1 = afEB[0] * aafAbsC[2][1] + afEB[1] * aafAbsC[2][0];
    fR01 = fR0 + fR1;
    if (fR > fR01) {
        return FALSE;
    }
    return TRUE;
}

U32 xObbHitsObb(const xBox* a, const xMat4x3* a_mat, const xBox* b, const xMat4x3* b_mat, xCollis* coll)
{
    if (Mgc_BoxBoxTest(a, a_mat, b, b_mat)) {
        coll->flags |= k_HIT_IT;
        return TRUE;
    } else {
        coll->flags &= ~k_HIT_IT;
        return FALSE;
    }
}

U32 xBoxHitsObb(const xBox* a, const xBox* b, const xMat4x3* mat, xCollis* coll)
{
    if (Mgc_BoxBoxTest(a, &g_I3, b, mat)) {
        coll->flags |= k_HIT_IT;
        return TRUE;
    } else {
        coll->flags &= ~k_HIT_IT;
        return FALSE;
    }
}

namespace {
    inline void render_tri(xCollis::tri_data&, const xModelInstance&) {}
}

void xCollideCalcTri(xCollis::tri_data& tri, const xModelInstance& model, const xVec3& center, const xVec3& heading)
{
    xVec3 v[3];

    RpGeometry* geom = RpAtomicGetGeometry(model.Data);
    const xVec3* verts = model.anim_coll.verts;
    if (!verts) {
        const RpMorphTarget* mt = RpGeometryGetMorphTarget(geom, 0);
        verts = (const xVec3*)RpMorphTargetGetVertices(mt);
    }
    const RpTriangle& t = RpGeometryGetTriangles(geom)[tri.index];
    for (S32 i = 0; i < 3; i++)
        v[i] = verts[t.vertIndex[i]];

    xVec3 e[3];
    e[0] = v[1] - v[0];
    e[1] = v[2] - v[1];
    e[2] = v[0] - v[2];

    struct
    {
        xVec3 norm;
        F32 D;
    } plane;
    plane.norm = e[0].cross(e[2]);
    plane.D = -v[0].dot(plane.norm);

    xVec3 p;
    F32 num = plane.D + plane.norm.dot(center);
    F32 denom = plane.norm.dot(heading);
    if (xfeq0(denom)) {
        p = v[0];
    } else {
        p = heading * -(num / denom) + center;
    }

    xVec3 b, A, B, C, AxB, CxB;
    A = p - v[0];
    B = v[2] - v[1];
    C = v[1] - v[0];
    AxB = A.cross(B);
    CxB = C.cross(B);
    F32 len2 = AxB.length2();
    if (xfeq0(len2)) {
        b = v[1];
    } else {
        b = v[0] + A * (CxB.dot(AxB) / len2);
    }

    {
        xVec3 d = b - v[0];
        xVec3 ad = d.get_abs();
        if (ad.x >= ad.y && ad.x >= ad.z)
            tri.r = xfeq0(ad.x) ? 0.0f : (p.x - v[0].x) / d.x;
        else if (ad.y >= ad.z)
            tri.r = xfeq0(ad.y) ? 0.0f : (p.y - v[0].y) / d.y;
        else
            tri.r = xfeq0(ad.z) ? 0.0f : (p.z - v[0].z) / d.z;
    }

    {
        xVec3 d = v[2] - v[1];
        xVec3 ad = d.get_abs();
        if (ad.x >= ad.y && ad.x >= ad.z)
            tri.d = xfeq0(ad.x) ? 0.0f : (b.x - v[1].x) / d.x;
        else if (ad.y >= ad.z)
            tri.d = xfeq0(ad.y) ? 0.0f : (b.y - v[1].y) / d.y;
        else
            tri.d = xfeq0(ad.z) ? 0.0f : (b.z - v[1].z) / d.z;
    }

    render_tri(tri, model);
}

xVec3 xCollisTriHit(const xCollis::tri_data& tri, const xModelInstance& model)
{
    const xMat4x3& m = *(const xMat4x3*)model.Mat;
    xVec3 v[3];

    RpGeometry* geom = RpAtomicGetGeometry(model.Data);
    const xVec3* verts = model.anim_coll.verts;
    if (!verts) {
        const RpMorphTarget* mt = RpGeometryGetMorphTarget(geom, 0);
        verts = (const xVec3*)RpMorphTargetGetVertices(mt);
    }
    const RpTriangle& t = RpGeometryGetTriangles(geom)[tri.index];
    for (S32 i = 0; i < 3; i++) {
        v[i] = verts[t.vertIndex[i]];
        xMat4x3Toworld(&v[i], &m, &v[i]);
    }

    xVec3 r[2], d;
    r[0] = v[0] + (v[1] - v[0]) * tri.r;
    r[1] = v[0] + (v[2] - v[0]) * tri.r;
    d = r[0] + (r[1] - r[0]) * tri.d;

    xVec3 A, B, C, AxB, CxB;
    A = d - v[0];
    B = r[1] - r[0];
    C = r[0] - v[0];
    AxB = A.cross(B);
    CxB = C.cross(B);
    F32 len2 = AxB.length2();

    return xfeq0(len2) ? v[0] : v[0] + A * (CxB.dot(AxB) / len2);
}

bool xSphereHitsBox(const xVec3& c, F32 r, const xBox& b)
{
    F32 ex = 0.5f * (b.upper.x - b.lower.x);
    F32 ey = 0.5f * (b.upper.y - b.lower.y);
    F32 ez = 0.5f * (b.upper.z - b.lower.z);

    F32 ax = xabs(c.x - (b.lower.x + ex));
    F32 ay = xabs(c.y - (b.lower.y + ey));
    F32 az = xabs(c.z - (b.lower.z + ez));

    F32 dx = ax - ex;
    F32 dy = ay - ey;
    F32 dz = az - ez;

    if (ax <= ex) {
        if (ay <= ey) {
            if (az <= ez) return true;
            return dz <= r;
        }

        if (az <= ez) return dy <= r;
        return xsqr(dy) + xsqr(dz) <= xsqr(r);
    }

    if (ay <= ey) {
        if (az <= ez) return dx <= r;
        return xsqr(dx) + xsqr(dz) <= xsqr(r);
    }

    if (az <= ez) return xsqr(dx) + xsqr(dy) <= xsqr(r);
    return xsqr(dx) + xsqr(dy) + xsqr(dz) <= xsqr(r);
}

bool xOBBHitsOBB(const xBox& a, const xMat4x3& amat, const xBox& b, const xMat4x3& bmat)
{
    return Mgc_BoxBoxTest(&a, &amat, &b, &bmat) != 0;

    F32 ax = 0.5f * (a.upper.x - a.lower.x);
    F32 ay = 0.5f * (a.upper.y - a.lower.y);
    F32 az = 0.5f * (a.upper.z - a.lower.z);
    F32 bx = 0.5f * (b.upper.x - b.lower.x);
    F32 by = 0.5f * (b.upper.y - b.lower.y);
    F32 bz = 0.5f * (b.upper.z - b.lower.z);

    xVec3 acenter;
    acenter.x = 0.5f * (a.upper.x + a.lower.x), acenter.y = 0.5f * (a.upper.y + a.lower.y), acenter.z = 0.5f * (a.upper.z + a.lower.z);
    xMat4x3Toworld(&acenter, &amat, &acenter);
    xVec3 bcenter;
    bcenter.x = 0.5f * (b.upper.x + b.lower.x), bcenter.y = 0.5f * (b.upper.y + b.lower.y), bcenter.z = 0.5f * (b.upper.z + b.lower.z);
    xMat4x3Toworld(&bcenter, &bmat, &bcenter);

    xVec3 offset;
    offset = bcenter - acenter;

    xVec3 aoffset;
    aoffset.x = amat.right.dot(offset), aoffset.y = amat.up.dot(offset), aoffset.z = amat.at.dot(offset);

    xMat3x3 xmat;
    xmat.right.x = amat.right.dot(bmat.right);
    xmat.right.y = amat.right.dot(bmat.up);
    xmat.right.z = amat.right.dot(bmat.at);
    xmat.up.x = amat.up.dot(bmat.right);
    xmat.up.y = amat.up.dot(bmat.up);
    xmat.up.z = amat.up.dot(bmat.at);
    xmat.at.x = amat.at.dot(bmat.right);
    xmat.at.y = amat.at.dot(bmat.up);
    xmat.at.z = amat.at.dot(bmat.at);

    xMat3x3 axmat;
    axmat.right = xmat.right;
    axmat.right.set_abs();
    axmat.up = xmat.up;
    axmat.up.set_abs();
    axmat.at = xmat.at;
    axmat.at.set_abs();

    F32 ar, br, r;

    br = bx * axmat.right.x + by * axmat.right.y + bz * axmat.right.z;
    ar = ax + br;
    if (xabs(aoffset.x) > ar) {
        return false;
    }

    br = bx * axmat.up.x + by * axmat.up.y + bz * axmat.up.z;
    ar = ay + br;
    if (xabs(aoffset.y) > ar) {
        return false;
    }

    br = bx * axmat.at.x + by * axmat.at.y + bz * axmat.at.z;
    ar = az + br;
    if (xabs(aoffset.z) > ar) {
        return false;
    }

    r = bmat.right.dot(offset);
    ar = ax * axmat.right.x + ay * axmat.up.x + az * axmat.at.x;
    br = ar + bx;
    if (xabs(r) > br) {
        return false;
    }

    r = bmat.up.dot(offset);
    ar = ax * axmat.right.y + ay * axmat.up.y + az * axmat.at.y;
    br = ar + by;
    if (xabs(r) > br) {
        return false;
    }

    r = bmat.at.dot(offset);
    ar = ax * axmat.right.z + ay * axmat.up.z + az * axmat.at.z;
    br = ar + bz;
    if (xabs(r) > br) {
        return false;
    }

    if (axmat.right.x > 0.999f || axmat.right.y > 0.999f || axmat.right.z > 0.999f ||
        axmat.up.x > 0.999f || axmat.up.y > 0.999f || axmat.up.z > 0.999f || axmat.at.x > 0.999f ||
        axmat.at.y > 0.999f || axmat.at.z > 0.999f) {
        return true;
    }

    r = aoffset.z * xmat.up.x - aoffset.y * xmat.at.x;
    ar = ay * axmat.at.x + az * axmat.up.x;
    br = by * axmat.right.z + bz * axmat.right.y + ar;
    if (xabs(r) > br) {
        return false;
    }

    r = aoffset.z * xmat.up.y - aoffset.y * xmat.at.y;
    ar = ay * axmat.at.y + az * axmat.up.y;
    br = bx * axmat.right.z + bz * axmat.right.x + ar;
    if (xabs(r) > br) {
        return false;
    }

    r = aoffset.z * xmat.up.z - aoffset.y * xmat.at.z;
    ar = ay * axmat.at.z + az * axmat.up.z;
    br = bx * axmat.right.y + by * axmat.right.x + ar;
    if (xabs(r) > br) {
        return false;
    }

    r = aoffset.x * xmat.at.x - aoffset.z * xmat.right.x;
    ar = ax * axmat.at.x + az * axmat.right.x;
    br = by * axmat.up.z + bz * axmat.up.y + ar;
    if (xabs(r) > br) {
        return false;
    }

    r = aoffset.x * xmat.at.y - aoffset.z * xmat.right.y;
    ar = ax * axmat.at.y + az * axmat.right.y;
    br = bx * axmat.up.z + bz * axmat.up.x + ar;
    if (xabs(r) > br) {
        return false;
    }

    r = aoffset.x * xmat.at.z - aoffset.z * xmat.right.z;
    ar = ax * axmat.at.z + az * axmat.right.z;
    br = bx * axmat.up.y + by * axmat.up.x + ar;
    if (xabs(r) > br) {
        return false;
    }

    r = aoffset.y * xmat.right.x - aoffset.x * xmat.up.x;
    ar = ax * axmat.up.x + ay * axmat.right.x;
    br = by * axmat.at.z + bz * axmat.at.y + ar;
    if (xabs(r) > br) {
        return false;
    }

    r = aoffset.y * xmat.right.y - aoffset.x * xmat.up.y;
    ar = ax * axmat.up.y + ay * axmat.right.y;
    br = bx * axmat.at.z + bz * axmat.at.x + ar;
    if (xabs(r) > br) {
        return false;
    }

    r = aoffset.y * xmat.right.z - aoffset.x * xmat.up.z;
    ar = ax * axmat.up.z + ay * axmat.right.z;
    br = bx * axmat.at.y + by * axmat.at.x + ar;
    if (xabs(r) > br) {
        return false;
    }

    return true;
}

bool xSphereHitsVCylinder(const xVec3& sc, F32 sr, const xVec3& cc, F32 cr, F32 ch)
{
    F32 ydist = cc.y - sc.y;
    if (ydist < -sr || ydist > ch + sr) return false;

    if (ydist < 0.0f) sr = xsqrt(xsqr(sr) - xsqr(ydist));
    else if (ydist > ch) sr = xsqrt(xsqr(sr) - xsqr(ydist - ch));

    xVec2 xzloc1 = { sc.x, sc.z }, xzloc2 = { cc.x, cc.z };
    F32 xzdist2 = (xzloc2 - xzloc1).length2();
    F32 max_xzdist = sr + cr;
    if (xzdist2 > xsqr(max_xzdist)) return false;

    return true;
}

U32 xSphereHitsVCylinder(const xSphere* sph, const xCylinder* cyl, xCollis* coll)
{
    F32 sr = sph->r;
    F32 ydist = cyl->center.y - sph->center.y;
    if (xabs(ydist) > cyl->h + sr) return FALSE;

    if (ydist < -cyl->h) sr = xsqrt(xsqr(sr) - xsqr(ydist + cyl->h));
    else if (ydist > cyl->h) sr = xsqrt(xsqr(sr) - xsqr(ydist - cyl->h));

    xVec2 xzloc1 = { sph->center.x, sph->center.z }, xzloc2 = { cyl->center.x, cyl->center.z };
    F32 xzdist2 = (xzloc2 - xzloc1).length2();
    F32 max_xzdist = sr + cyl->r;
    if (xzdist2 > xsqr(max_xzdist)) return FALSE;

    if ((coll->flags & 0x200) || (coll->flags & 0x400) || (coll->flags & 0x800)) {
        coll->norm = sph->center - cyl->center;
        coll->norm.y = 0.0f;
        coll->norm.safe_normalize(xVec3::m_Null);
    }

    if ((coll->flags & 0x100) || (coll->flags & 0x400) || (coll->flags & 0x800)) {
        F32 xzcenter_dist = xsqrt(xzdist2);
        coll->dist = max_xzdist - xzcenter_dist;
    }

    if (coll->flags & 0x400) {
        coll->tohit = coll->norm * -(sr - coll->dist);
        if (ydist < -cyl->h) {
            coll->tohit.y = -cyl->h;
        } else if (ydist > cyl->h) {
            coll->tohit.y = cyl->h;
        }
    }

    if (coll->flags & 0x800) {
        coll->depen = coll->norm * -coll->dist;
    }

    coll->flags |= k_HIT_IT;
    return TRUE;
}

U32 xCylinderHitsSphere(const xCylinder* cyl, const xSphere* sph, xCollis* coll)
{
    F32 sr = sph->r;
    F32 ydist = cyl->center.y - sph->center.y;
    if (xabs(ydist) > cyl->h + sr) return FALSE;

    if (ydist < -cyl->h) sr = xsqrt(xsqr(sr) - xsqr(ydist + cyl->h));
    else if (ydist > cyl->h) sr = xsqrt(xsqr(sr) - xsqr(ydist - cyl->h));

    xVec2 xzloc1 = { sph->center.x, sph->center.z }, xzloc2 = { cyl->center.x, cyl->center.z };
    F32 xzdist2 = (xzloc2 - xzloc1).length2();
    F32 max_xzdist = sr + cyl->r;
    if (xzdist2 > xsqr(max_xzdist)) return FALSE;

    if ((coll->flags & 0x200) || (coll->flags & 0x400) || (coll->flags & 0x800)) {
        coll->norm = cyl->center - sph->center;
        coll->norm.y = 0.0f;
        coll->norm.safe_normalize(xVec3::m_Null);
    }

    if ((coll->flags & 0x100) || (coll->flags & 0x400) || (coll->flags & 0x800)) {
        F32 xzcenter_dist = xsqrt(xzdist2);
        coll->dist = max_xzdist - xzcenter_dist;
    }

    if (coll->flags & 0x400) {
        coll->tohit = coll->norm * -(cyl->r - coll->dist);
        if (ydist < -cyl->h) {
            coll->tohit.y = -cyl->h;
        }
        else if (ydist > cyl->h) {
            coll->tohit.y = cyl->h;
        }
    }

    if (coll->flags & 0x800) {
        coll->depen = coll->norm * -coll->dist;
    }

    coll->flags |= k_HIT_IT;
    return TRUE;
}

bool xSphereHitsVCircle(const xVec3& sc, F32 sr, const xVec3& cc, F32 cr)
{
    F32 ydist = cc.y - sc.y;
    if (ydist < -sr || ydist > sr) return false;

    sr = xsqrt(xsqr(sr) - xsqr(ydist));

    xVec2 xzloc1 = { sc.x, sc.z }, xzloc2 = { cc.x, cc.z };
    F32 xzdist2 = (xzloc2 - xzloc1).length2();
    F32 max_xzdist = sr + cr;
    if (xzdist2 > xsqr(max_xzdist)) return false;

    return true;
}

bool xSphereHitsCone(const xVec3& center, F32 radius, const xVec3& kv0, F32 kr0, const xVec3& kv1, F32 kr1)
{
    xMat4x3 mat;
    mat.up = kv1; mat.up -= kv0;
    F32 dist = mat.up.length();
    mat.up /= dist;
    mat.right = center; mat.right -= kv0;
    mat.at.cross(mat.up, mat.right);
    mat.at.up_normalize();
    mat.right.cross(mat.at, mat.up);
    mat.pos = kv0;

    xVec3 C;
    xMat4x3Tolocal(&C, &mat, &center);

    F32 R = radius;
    F32 Cx = C.x;
    F32 Cy = C.y;
    F32 Ax = kr0;
    F32 Bx = kr1;
    F32 By = dist;

    if (Cy < -R || Cy > By + R) {
        return false;
    }

    F32 Dx = Bx - Ax, Dy = By;

    F32 M = Ax + Dx * Cy / Dy;
    if (M >= Cx) {
        return true;
    }

    F32 Ex = Ax - Cx;
    F32 Ey = -Cy;

    F32 a = xsqr(Dx) + xsqr(Dy);
    F32 b = 2.0f * (Dx * Ex + Dy * Ey);
    F32 c = xsqr(Ex) + xsqr(Ey) - xsqr(R);
    F32 det = xsqr(b) - 4.0f * a * c;
    if (det < 0.0f) {
        return false;
    }

    det = xsqrt(det);
    F32 i2a = 0.5f / a;
    F32 t0 = i2a * (-b + det);
    F32 t1 = i2a * (-b - det);
    if ((t0 < 0.0f && t1 < 0.0f) || (t0 > 1.0f && t1 > 1.0f)) {
        return false;
    }

    return true;
}

void xBoxForTriangle(xBox& box, const xVec3& p0, const xVec3& p1, const xVec3& p2)
{
    if (p0.x < p1.x) {
        box.lower.x = p0.x;
        box.upper.x = p1.x;
    } else {
        box.lower.x = p1.x;
        box.upper.x = p0.x;
    }

    if (p0.y < p1.y) {
        box.lower.y = p0.y;
        box.upper.y = p1.y;
    } else {
        box.lower.y = p1.y;
        box.upper.y = p0.y;
    }

    if (p0.z < p1.z) {
        box.lower.z = p0.z;
        box.upper.z = p1.z;
    } else {
        box.lower.z = p1.z;
        box.upper.z = p0.z;
    }

    if (box.upper.x < p2.x) box.upper.x = p2.x;
    if (box.upper.y < p2.y) box.upper.y = p2.y;
    if (box.upper.z < p2.z) box.upper.z = p2.z;
    if (box.lower.x > p2.x) box.lower.x = p2.x;
    if (box.lower.y > p2.y) box.lower.y = p2.y;
    if (box.lower.z > p2.z) box.lower.z = p2.z;
}

void xBoxForModel(xBox& o, const xModelInstance* m, bool childModels)
{
    xBoxForModelLocal(o, m, childModels);

    xVec3AddTo(&o.upper, &((xMat4x3*)m->Mat)->pos);
    xVec3AddTo(&o.lower, &((xMat4x3*)m->Mat)->pos);
}

void xBoxForModelLocal(xBox& o, const xModelInstance* m, bool childModels)
{
    RpAtomic* imodel;
    RpGeometry* geom;
    RpMorphTarget* mtgt;
    U16 idx;
    const xModelInstance* curModel = m;
    xVec3 lower, upper;

    do {
        imodel = curModel->Data;

        geom = imodel->geometry;

        if (!geom->triangles) {
            lower.x = imodel->boundingSphere.center.x - imodel->boundingSphere.radius;
            lower.y = imodel->boundingSphere.center.y - imodel->boundingSphere.radius;
            lower.z = imodel->boundingSphere.center.z - imodel->boundingSphere.radius;
            upper.x = imodel->boundingSphere.center.x + imodel->boundingSphere.radius;
            upper.y = imodel->boundingSphere.center.y + imodel->boundingSphere.radius;
            upper.z = imodel->boundingSphere.center.z + imodel->boundingSphere.radius;

            if (curModel == m) {
                o.lower = lower;
                o.upper = upper;
            } else {
                iBoxBoundVec(&o, &o, &lower);
                iBoxBoundVec(&o, &o, &upper);
            }
        } else {
            if (geom->numMorphTargets > 1) {
            } else if (geom->numMorphTargets < 1) {
                return;
            }

            mtgt = RpGeometryGetMorphTarget(geom, 0);
            if (curModel == m) {
                iBoxInitBoundVec(&o, (xVec3*)mtgt->verts);
                idx = 1;
            } else {
                idx = 0;
            }

            for (; idx < geom->numVertices; idx++) {
                iBoxBoundVec(&o, &o, (xVec3*)&mtgt->verts[idx]);
            }
        }

        curModel = curModel->Next;
    } while (curModel && childModels);

    o.upper.x = MAX(o.lower.x + 0.01f, o.upper.x);
    o.upper.y = MAX(o.lower.y + 0.01f, o.upper.y);
    o.upper.z = MAX(o.lower.z + 0.01f, o.upper.z);
}

void xSphereForModel(xSphere& o, const xModelInstance* model, bool childModels)
{
    RpAtomic* imodel;
    RpGeometry* geom;
    RpMorphTarget* mtgt;
    U16 idx;
    const xModelInstance* cur_model;
    xSphere s;

    cur_model = model;

    do {
        imodel = cur_model->Data;

        geom = imodel->geometry;

        if (!geom->triangles) {
            s.center.x = imodel->boundingSphere.center.x;
            s.center.y = imodel->boundingSphere.center.y;
            s.center.z = imodel->boundingSphere.center.z;
            s.r = imodel->boundingSphere.radius;
            if (cur_model == model) {
                o = s;
            } else {
                iSphereBoundSphere(&o, &o, &s);
            }
        } else {
            if (geom->numMorphTargets > 1) {
            } else if (geom->numMorphTargets < 1) {
                return;
            }

            mtgt = RpGeometryGetMorphTarget(geom, 0);
            if (cur_model == model) {
                iSphereInitBoundVec(&o, (xVec3*)mtgt->verts);
                idx = 1;
            } else {
                idx = 0;
            }

            for (; idx < geom->numVertices; idx++) {
                iSphereBoundVec(&o, &o, (xVec3*)&mtgt->verts[idx]);
            }
        }

        cur_model = cur_model->Next;
    } while (childModels == true && cur_model);
}

static xRay3 cbray;

static RpCollisionTriangle* rayHitsEnvBackwardCB(RpIntersection*, RpWorldSector* sector, RpCollisionTriangle* tri, F32 dist, void* data)
{
    xCollis* coll = (xCollis*)data;

    dist = 1.0f - dist;
    dist *= cbray.max_t;

    if (dist >= coll->dist) {
        return tri;
    }

    if (sector) {
        coll->oid = sector->triangles[tri->index].matIndex;
    } else if (sCollidingJSP) {
        coll->oid = ((xClumpCollBSPTriangle*)tri->index)->matIndex;
    } else {
        coll->oid = tri->index;
    }

    coll->dist = dist;
    coll->flags |= k_HIT_IT;
    coll->tri.index = tri->index;
    return tri;
}

static RpCollisionTriangle* rayHitsModelBackwardCB(RpIntersection* isx, RpCollisionTriangle* tri, F32 dist, void* data)
{
    return rayHitsEnvBackwardCB(isx, NULL, tri, dist, data);
}

static RpCollisionTriangle* rayHitsEnvCB(RpIntersection*, RpWorldSector* sector, RpCollisionTriangle* tri, F32 dist, void* data)
{
    xCollis* coll = (xCollis*)data;

    dist *= cbray.max_t;

    if (dist >= coll->dist) {
        return tri;
    }

    if (sector) {
        coll->oid = sector->triangles[tri->index].matIndex;
    } else if (sCollidingJSP) {
        coll->oid = ((xClumpCollBSPTriangle*)tri->index)->matIndex;

        U8 flags = ((xClumpCollBSPTriangle*)tri->index)->flags;
        if ((coll->flags & 0x4000) && !(flags & 0x8)) return tri;
        if (flags & 0x10) {
            coll->flags |= 0x20000;
        } else {
            coll->flags &= ~0x20000;
        }
    } else {
        coll->oid = tri->index;
    }

    coll->dist = dist;
    coll->flags |= k_HIT_IT;
    coll->tri.index = tri->index;

    if (coll->flags & 0x200) {
        coll->norm = *(xVec3*)&tri->normal;
    }

    return tri;
}

static RpCollisionTriangle* rayHitsModelCB(RpIntersection* isx, RpCollisionTriangle* tri, F32 dist, void* data)
{
    return rayHitsEnvCB(isx, NULL, tri, dist, data);
}

U32 xRayHitsModelSingle(const xRay3* r, const xModelInstance* m, xCollis* coll)
{
    RpIntersection isx;

    if (m->Flags & 0x800) xModelAnimCollApply(*m);

    isx.type = rpINTERSECTLINE;
    xLine3FromRay(*(xLine3*)&isx.t.line, r);

    cbray = *r;
    if (!(r->flags & XRAY3_USE_MAX)) {
        cbray.max_t = 1.0f;
    }

    if (r->flags & XRAY3_USE_MIN) {
        cbray.max_t -= cbray.min_t;
    }

    xMat4x3* mat = (xMat4x3*)m->Mat;
    RwFrame* frame = RpAtomicGetFrame(m->Data);

    RwFrameTransform(frame, (RwMatrix*)mat, rwCOMBINEREPLACE);

    F32 mscale = xVec3Length(&mat->right);

    cbray.max_t /= mscale;

    coll->flags &= ~k_HIT_IT;
    coll->dist = HUGE;
    if (coll->flags & k_HIT_CALC_TRI) {
        coll->flags |= 0x400;
    }

    RpAtomicForAllIntersections(m->Data, &isx, rayHitsModelCB, coll);

    RwV3d temp = isx.t.line.start;
    isx.t.line.start = isx.t.line.end;
    isx.t.line.end = temp;

    RpAtomicForAllIntersections(m->Data, &isx, rayHitsModelBackwardCB, coll);

    coll->dist *= mscale;
    if (r->flags & XRAY3_USE_MIN) {
        coll->dist += cbray.min_t;
    }

    if ((coll->flags & k_HIT_CALC_TRI) && (coll->flags & k_HIT_IT)) {
        xVec3 center, heading;
        xMat4x3Tolocal(&center, (xMat4x3*)m->Mat, &r->origin);
        xMat4x3Tolocal(&heading, (xMat4x3*)m->Mat, &coll->tohit);
        xCollideCalcTri(coll->tri, *m, center, heading);
    }

    if (m->Flags & 0x800) xModelAnimCollRestore(*m);

    return coll->flags & k_HIT_IT;
}

U32 xRayHitsModel(const xRay3* r, const xModelInstance* m, xCollis* coll)
{
    xRayHitsModelSingle(r, m, coll);

    for (xModelInstance* model = m->Next; model; model = model->Next) {
        xCollis temp_coll;
        temp_coll.flags = coll->flags & ~k_HIT_IT;
        if (xRayHitsModelSingle(r, model, &temp_coll)) {
            if (temp_coll.dist < coll->dist)
                memcpy(coll, &temp_coll, sizeof(xCollis));
        }
    }

    return coll->flags & k_HIT_IT;
}

U32 xRayHitsEnv(const xRay3* r, const xEnv* env, xCollis* coll)
{
    RpIntersection isx;

    isx.type = rpINTERSECTLINE;
    xLine3FromRay(*(xLine3*)&isx.t.line, r);

    coll->flags &= ~k_HIT_IT;
    coll->dist = HUGE;

    cbray = *r;
    if (!(r->flags & XRAY3_USE_MAX)) {
        cbray.max_t = 1.0f;
    }

    if (r->flags & XRAY3_USE_MIN) {
        cbray.max_t -= cbray.min_t;
    }

    sCollidingJSP = 1;
    for (S32 i = 0; i < env->geom->jsp_count; i++) {
        if (xEnvIsJSPActive(env, i)) {
            xIsect iBoxIsect;
            iBoxIsectRay(&env->geom->jsp_bound[i], r, &iBoxIsect);

            if (iBoxIsect.penned <= 0.0f) {
                coll->test_dist = coll->dist;
                xClumpColl_ForAllIntersections(env->geom->jsp_list[i]->colltree, &isx, rayHitsEnvCB, coll);
                if (coll->dist < coll->test_dist) {
                    coll->oid = get_matid(env->geom->jsp_list[i], coll->oid, 0);
                }
            }
        }
    }
    sCollidingJSP = 0;

    if (r->flags & XRAY3_USE_MIN) {
        coll->dist += cbray.min_t;
    }

    return coll->flags & k_HIT_IT;
}

static U8 cbnumcs;
static U8 cbmaxcs;
static U8 FLOOR;
static U8 NEXT2;
static U8 OTHER;
static RpIntersection cbisx_local;
static F32 cbath;

static RpCollisionTriangle* sphereHitsEnv3CB(RpIntersection* isx, RpWorldSector* sector, RpCollisionTriangle* tri, F32 dist, void* data)
{
    xCollis* colls;

    if (cbnumcs == cbmaxcs) {
        return NULL;
    }

    F32 radius = isx->t.sphere.radius;
    xVec3* center = (xVec3*)&isx->t.sphere.center;
    colls = (xCollis*)data;
    xVec3 tohit, hdng;
    U8 idx;

    dist = HUGE;

    properSphereIsectTri(center, radius, &tohit, &dist, tri);

    if (dist == 0.0f) {
        return tri;
    }

    if (dist > radius) {
        return tri;
    }

    if (xsqr(tohit.y) > xsqr(tohit.x) + xsqr(tohit.z)) {
        if (FLOOR == 0xFF) {
            idx = FLOOR = cbnumcs++;
        }
        else if (tohit.y < 0.0f) {
            if (colls[FLOOR].hdng.y > 0.0f || dist < colls[FLOOR].dist ||
                (xabs(dist - colls[FLOOR].dist) < 0.001f && tri->normal.y > colls[FLOOR].norm.y)) {
                idx = FLOOR;
            } else {
                return tri;
            }
        } else {
            if (colls[FLOOR].hdng.y > 0.0f &&
                (dist < colls[FLOOR].dist || (xabs(dist - colls[FLOOR].dist) < 0.001f && tri->normal.y > colls[FLOOR].norm.y))) {
                idx = FLOOR;
            } else {
                return tri;
            }
        }

        xVec3SMul(&hdng, &tohit, 1.0f / dist);
    }
    else if (NEXT2 == 0xFF) {
        if (OTHER == 0xFF) {
            idx = OTHER = cbnumcs++;
            xVec3SMul(&hdng, &tohit, 1.0f / dist);
        } else {
            xVec3SMul(&hdng, &tohit, 1.0f / dist);

            F32 dot = xVec3Dot(&hdng, &colls[OTHER].hdng);
            if (xabs(dot) > 0.70710677f) {
                if (dist < colls[OTHER].dist) {
                    idx = OTHER;
                } else {
                    return tri;
                }
            } else {
                idx = NEXT2 = cbnumcs++;
                if (dist < colls[OTHER].dist) {
                    idx = OTHER;
                    OTHER = NEXT2;
                    NEXT2 = idx;
                    idx = OTHER;
                }
            }
        }
    } else {
        if (dist > colls[NEXT2].dist) {
            return tri;
        }

        xVec3SMul(&hdng, &tohit, 1.0f / dist);
        F32 odot = xVec3Dot(&hdng, &colls[OTHER].hdng);
        if (dist < colls[OTHER].dist) {
            if (xabs(odot) > 0.70710677f) {
                F32 ndot = xVec3Dot(&hdng, &colls[NEXT2].hdng);
                if (xabs(ndot) > 0.70710677f) {
                    if (NEXT2 < OTHER) {
                        OTHER = NEXT2;
                    }

                    idx = OTHER;

                    NEXT2 = 0xFF;
                    cbnumcs--;
                    colls[cbnumcs].flags &= ~k_HIT_IT;
                    colls[cbnumcs].dist = HUGE;
                } else {
                    idx = OTHER;
                }
            } else {
                idx = NEXT2;
            }
        } else {
            if (xabs(odot) > 0.70710677f) {
                return tri;
            }

            idx = NEXT2;
        }
    }

    colls[idx].dist = dist;
    xVec3Copy(&colls[idx].hdng, &hdng);

    if (colls->flags & 0x200) {
        xVec3Copy(&colls[idx].norm, (xVec3*)&tri->normal);
    }

    if (colls->flags & 0x400) {
        xVec3Copy(&colls[idx].tohit, &tohit);
    }

    if (colls->flags & 0x800) {
        F32 scale = dist - radius;
        xVec3Copy(&colls[idx].depen, &hdng);
        xVec3SMulBy(&colls[idx].depen, scale);
    }

    colls[idx].flags |= k_HIT_IT;

    if (sector) {
        colls[idx].oid = sector->triangles[tri->index].matIndex;
    } else if (sCollidingJSP) {
        colls[idx].oid = ((xClumpCollBSPTriangle*)tri->index)->matIndex;
    } else {
        colls[idx].oid = tri->index;
    }

    colls[idx].tri.index = tri->index;

    if (cbnumcs == cbmaxcs) {
        return NULL;
    }

    return tri;
}

static RpCollisionTriangle* sphereHitsModel3CB(RpIntersection*, RpCollisionTriangle* tri, F32 dist, void* data)
{
    return sphereHitsEnv3CB(&cbisx_local, NULL, tri, dist, data);
}

S32 xSphereHitsModel3(const xSphere* b, const xModelInstance* m, xCollis* colls, U8 ncolls, F32 sth)
{
    RpIntersection isx;
    U8 idx, i;

    if (m->Flags & 0x800) {
        xModelAnimCollApply(*m);
    }

    for (i = 0; i < ncolls; i++)
        if (colls[i].flags & k_HIT_CALC_TRI)
            colls[i].flags |= 0x400;

    isx.type = rpINTERSECTSPHERE;
    memcpy(&isx.t.sphere, b, sizeof(RwSphere));

    xMat4x3* mat = (xMat4x3*)m->Mat;
    RwFrame* frame = RpAtomicGetFrame(m->Data);

    RwFrameTransform(frame, (RwMatrix*)mat, rwCOMBINEREPLACE);

    F32 mscale = xVec3Length(&mat->right);

    xMat3x3 mnormal;
    xMat3x3Normalize(&mnormal, mat);

    xMat4x3Tolocal((xVec3*)&cbisx_local.t.sphere.center, mat, &b->center);
    cbisx_local.t.sphere.radius = b->r / mscale;

    for (idx = 0; idx < ncolls; idx++) {
        colls[idx].flags &= ~k_HIT_IT;
        colls[idx].dist = HUGE;
    }

    cbath = sth;
    cbnumcs = 0;
    cbmaxcs = ncolls;
    FLOOR = NEXT2 = OTHER = 0xFF;

    RpAtomicForAllIntersections(m->Data, &isx, sphereHitsModel3CB, colls);

    if (colls->flags & 0x1F00) {
        for (idx = 0; idx < cbnumcs; idx++) {
            if (colls[idx].flags & k_HIT_IT) {
                if (colls[idx].flags & k_HIT_CALC_TRI) xCollideCalcTri(colls[idx].tri, *m, *(xVec3*)&cbisx_local.t.sphere.center, colls[idx].tohit);

                xMat3x3RMulVec(&colls[idx].tohit, mat, &colls[idx].tohit);
                xMat3x3RMulVec(&colls[idx].depen, mat, &colls[idx].depen);
                xMat3x3RMulVec(&colls[idx].hdng, &mnormal, &colls[idx].hdng);
                xMat3x3RMulVec(&colls[idx].norm, &mnormal, &colls[idx].norm);
                colls[idx].dist *= mscale;
            }
        }
    }

    if (m->Flags & 0x800) xModelAnimCollRestore(*m);

    return cbnumcs;
}

static RpCollisionTriangle* sphereHitsEnv4CB(RpIntersection* isx, RpWorldSector* sector, RpCollisionTriangle* tri, F32 dist, void* data); // UNUSED

S32 xSphereHitsEnv3(const xSphere* b, const xEnv* env, xCollis* colls, U8 ncolls, F32 sth)
{
    RpIntersection isx;
    U8 idx;

    isx.type = rpINTERSECTSPHERE;
    memcpy(&isx.t.sphere, b, sizeof(RwSphere));

    for (idx = 0; idx < ncolls; idx++) {
        colls[idx].flags &= ~k_HIT_IT;
        colls[idx].dist = HUGE;
    }

    cbath = sth;
    cbnumcs = 0;
    cbmaxcs = ncolls;
    FLOOR = NEXT2 = OTHER = 0xFF;

    sCollidingJSP = 1;
    for (S32 i = 0; i < env->geom->jsp_count; i++) {
        if (xEnvIsJSPActive(env, i)) {
            if (xSphereHitsBox(*b, env->geom->jsp_bound[i])) {
                for (idx = 0; idx < ncolls; idx++) {
                    colls[idx].test_dist = colls[idx].dist;
                }

                xClumpColl_ForAllIntersections(env->geom->jsp_list[i]->colltree, &isx, sphereHitsEnv3CB, colls);

                for (idx = 0; idx < ncolls; idx++) {
                    if (colls[idx].dist < colls[idx].test_dist) {
                        colls[idx].oid = get_matid(env->geom->jsp_list[i], colls[idx].oid, 0);
                    }
                }
            }
        }
    }

    sCollidingJSP = 0;

    return cbnumcs;
}

U32 xSphereHitsEnv(const xSphere* b, const xEnv* env, xCollis* coll)
{
    RpIntersection isx;

    isx.type = rpINTERSECTSPHERE;
    memcpy(&isx.t.sphere, b, sizeof(RwSphere));
    coll->flags &= ~k_HIT_IT;

    coll->dist = HUGE;
    coll->mptr = NULL;

    coll->optr = NULL;

    sCollidingJSP = 1;
    for (S32 i = 0; i < env->geom->jsp_count; i++) {
        if (xEnvIsJSPActive(env, i)) {
            if (xSphereHitsBox(*b, env->geom->jsp_bound[i])) {
                coll->test_dist = coll->dist;
                xClumpColl_ForAllIntersections(env->geom->jsp_list[i]->colltree, &isx, sphereHitsEnvCB, coll);
                if (coll->dist < coll->test_dist) {
                    coll->oid = get_matid(env->geom->jsp_list[i], coll->oid, 0);
                }
            }
        }
    }

    sCollidingJSP = 0;

    return coll->flags & k_HIT_IT;
}
