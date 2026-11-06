#include "xScene.h"

#include "xAnim.h"
#include "xClumpColl.h"
#include "xCollide.h"
#include "xEnt.h"
#include "xGrid.h"
#include "xJSP.h"
#include "xMath.h"
#include "xMemory.h"
#include "xModel.h"
#include "zGrid.h"
#include "zSurface.h"

#include <rpcollis.h>
#include <string.h>

static const S32 NUM_TRI_VERTS = 3;

xScene* g_xSceneCur;

void xSceneInit(xScene* sc, U16 num_trigs, U16 num_stats, U16 num_dyns, U16 num_npcs)
{
    sc->flags = 0;

    sc->num_trigs_allocd = num_trigs;
    sc->trigs = (xEnt**)xMEMALLOC((U32)num_trigs * sizeof(xEnt*), 0);

    sc->num_stats_allocd = num_stats;
    sc->stats = (xEnt**)xMEMALLOC((U32)num_stats * sizeof(xEnt*), 0);

    sc->num_dyns_allocd = num_dyns;
    sc->dyns = (xEnt**)xMEMALLOC((U32)num_dyns * sizeof(xEnt*), 0);

    sc->num_npcs_allocd = num_npcs;
    sc->npcs = (xEnt**)xMEMALLOC((U32)num_npcs * sizeof(xEnt*), 0);

    sc->num_ents_allocd = num_trigs + num_stats + num_dyns + num_npcs;
    sc->num_act_ents = 0;
    sc->act_ents = (xEnt**)xMEMALLOC(sc->num_ents_allocd * sizeof(xEnt*), 0);

    sc->num_trigs = 0;
    sc->num_stats = 0;
    sc->num_dyns = 0;
    sc->num_npcs = 0;

    sc->resolvID = NULL;

    xAnimInit();
    xModelInit();

    xAnimPoolInit(&sc->mempool, 50, 1, 1, 4);

    xModelPoolInit(49, 64);
    xModelPoolInit(74, 8);
    xModelPoolInit(384, 1);
}

void xSceneExit(xScene*)
{
}

void xSceneSave(xScene*, xSerial*)
{
}

void xSceneLoad(xScene*, xSerial*)
{
}

void xSceneSetup(xScene* sc)
{
    xEnvSetup(sc->env);
}

void xSceneAddEnt(xScene* sc, xEnt* ent)
{
    if (ent->collType == 0x1) {
        sc->trigs[sc->num_trigs++] = ent;
    } else if (ent->collType == 0x2) {
        sc->stats[sc->num_stats++] = ent;
    } else if (ent->collType == 0x4) {
        sc->dyns[sc->num_dyns++] = ent;
    } else if (ent->collType == 0x8) {
        sc->npcs[sc->num_npcs++] = ent;
    }

    sc->act_ents[sc->num_act_ents++] = ent;
}

xBase* xSceneResolvID(xScene* sc, U32 id)
{
    if (sc->resolvID) {
        return sc->resolvID(id);
    }

    return NULL;
}

void xSceneForAllEnts(xScene* sc, xSceneEntCallback func, void* data)
{
    for (U16 i = 0; i < sc->num_act_ents; i++) {
        if (!func(sc->act_ents[i], sc, data)) {
            break;
        }
    }
}

void xRayHitsGrid(xGrid* grid, xScene* sc, xRay3* r, xRayEntCallback rentcb, xQCData* qcr, void* data)
{
    xLine3 ln;
    xGridIterator it;

    r->flags |= XRAY3_USE_MIN | XRAY3_USE_MAX;

    xLine3FromRay(ln, r);

    S32 sclgx, sclgz, eclgx, eclgz;
    F32 sclcx, sclcz, eclcx, eclcz, halfclx, halfclz, quatrclx, quatrclz;

    halfclx = 0.5f * grid->csizex, halfclz = 0.5f * grid->csizez;
    quatrclx = 0.25f * grid->csizex, quatrclz = 0.25f * grid->csizez;

    S32 sz, ez, x, z;
    F32 dxdz;

    if (xabs(r->dir.z) <= 0.00001f) {
        if (r->dir.z >= 0.0f) {
            r->dir.z = 0.00001f;
        } else {
            r->dir.z = -0.00001f;
        }
    }

    dxdz = r->dir.x / r->dir.z;

    xGridGetCell(grid, ln.p1.x, ln.p1.y, ln.p1.z, sclgx, sclgz);
    xGridGetCell(grid, ln.p2.x, ln.p2.y, ln.p2.z, eclgx, eclgz);

    sclcx = grid->csizex * sclgx, sclcz = grid->csizez * sclgz;
    sclcx += grid->minx, sclcz += grid->minz;
    sclcx += halfclx, sclcz += halfclz;
    eclcx = grid->csizex * eclgx, eclcz = grid->csizez * eclgz;
    eclcx += grid->minx, eclcz += grid->minz;
    eclcx += halfclx, eclcz += halfclz;

    if (ln.p1.z < ln.p2.z) {
        sz = sclgz;
        ez = eclgz;

        if (sclgz > 0 && ln.p1.z < sclcz - quatrclz) {
            sz--;
        }

        if (eclgz < grid->nz - 1 && ln.p2.z > eclcz + quatrclz) {
            ez++;
        }

        for (z = sz; z <= ez; z++) {
            F32 cz = grid->csizez * (z - sz) + sclcz;
            F32 lz = cz - halfclz;
            F32 rz = cz + halfclz, lx, rx;

            if (lz < ln.p1.z) {
                lx = ln.p1.x, lz = ln.p1.z;
            } else {
                lx = dxdz * (lz - ln.p1.z) + ln.p1.x;
            }

            if (rz > ln.p2.z) {
                rx = ln.p2.x, rz = ln.p2.z;
            } else {
                rx = dxdz * (rz - ln.p1.z) + ln.p1.x;
            }

            F32 ly = 0.0f, ry = 0.0f;
            S32 aclgx, aclgz, bclgx, bclgz;
            F32 aclcx, aclcz, bclcx, bclcz;
            S32 ax, bx;

            xGridGetCell(grid, lx, ly, lz, aclgx, aclgz);
            xGridGetCell(grid, rx, ry, rz, bclgx, bclgz);

            aclcx = grid->csizex * aclgx, aclcz = grid->csizez * aclgz;
            aclcx += grid->minx, aclcz += grid->minz;
            aclcx += halfclx, aclcz += halfclz;
            bclcx = grid->csizex * bclgx, bclcz = grid->csizez * bclgz;
            bclcx += grid->minx, bclcz += grid->minz;
            bclcx += halfclx, bclcz += halfclz;

            if (lx < rx) {
                ax = aclgx;
                bx = bclgx;

                if (aclgx > 0 && lx < aclcx - quatrclx) {
                    ax--;
                }

                if (bclgx < grid->nx - 1 && rx > bclcx + quatrclx) {
                    bx++;
                }

                for (x = ax; x <= bx; x++) {
                    xGridBound* cell = xGridIterFirstCell(grid, x, z, it);

                    while (cell) {
                        xBound* cellbound = (xBound*)(cell + 1);
                        if (xQuickCullIsects(qcr, &cellbound->qcd)) {
                            rentcb(sc, r, qcr, (xEnt*)cell->data, data);
                        }
                        cell = xGridIterNextCell(it);
                    }
                }
            } else {
                ax = aclgx;
                bx = bclgx;

                if (bclgx > 0 && rx < bclcx - quatrclx) {
                    bx--;
                }

                if (aclgx < grid->nx - 1 && lx > bclcx + quatrclx) {
                    ax++;
                }

                for (x = bx; x <= ax; x++) {
                    xGridBound* cell = xGridIterFirstCell(grid, x, z, it);

                    while (cell) {
                        xBound* cellbound = (xBound*)(cell + 1);
                        if (xQuickCullIsects(qcr, &cellbound->qcd)) {
                            rentcb(sc, r, qcr, (xEnt*)cell->data, data);
                        }
                        cell = xGridIterNextCell(it);
                    }
                }
            }
        }
    } else {
        sz = sclgz;
        ez = eclgz;

        if (eclgz > 0 && ln.p2.z < eclcz - quatrclz) {
            ez--;
        }

        if (sclgz < grid->nz - 1 && ln.p1.z > sclcz + quatrclz) {
            sz++;
        }

        for (z = ez; z <= sz; z++) {
            F32 cz = grid->csizez * (z - ez) + eclcz;
            F32 lz = cz - halfclz;
            F32 rz = cz + halfclz, lx, rx;

            if (lz < ln.p2.z) {
                lx = ln.p2.x, lz = ln.p2.z;
            } else {
                lx = dxdz * (ln.p2.z - lz) + ln.p2.x;
            }

            if (rz > ln.p1.z) {
                rx = ln.p1.x, rz = ln.p1.z;
            } else {
                rx = dxdz * (ln.p2.z - rz) + ln.p2.x;
            }

            F32 ly = 0.0f, ry = 0.0f;
            S32 aclgx, aclgz, bclgx, bclgz;
            F32 aclcx, aclcz, bclcx, bclcz;
            S32 ax, bx;

            xGridGetCell(grid, lx, ly, lz, aclgx, aclgz);
            xGridGetCell(grid, rx, ry, rz, bclgx, bclgz);

            aclcx = grid->csizex * aclgx, aclcz = grid->csizez * aclgz;
            aclcx += grid->minx, aclcz += grid->minz;
            aclcx += halfclx, aclcz += halfclz;
            bclcx = grid->csizex * bclgx, bclcz = grid->csizez * bclgz;
            bclcx += grid->minx, bclcz += grid->minz;
            bclcx += halfclx, bclcz += halfclz;

            if (lx < rx) {
                ax = aclgx;
                bx = bclgx;

                if (aclgx > 0 && lx < aclcx - quatrclx) {
                    ax--;
                }

                if (bclgx < grid->nx - 1 && rx > bclcx + quatrclx) {
                    bx++;
                }

                for (x = ax; x <= bx; x++) {
                    xGridBound* cell = xGridIterFirstCell(grid, x, z, it);

                    while (cell) {
                        xBound* cellbound = (xBound*)(cell + 1);
                        if (xQuickCullIsects(qcr, &cellbound->qcd)) {
                            rentcb(sc, r, qcr, (xEnt*)cell->data, data);
                        }
                        cell = xGridIterNextCell(it);
                    }
                }
            } else {
                ax = aclgx;
                bx = bclgx;

                if (bclgx > 0 && rx < bclcx - quatrclx) {
                    bx--;
                }

                if (aclgx < grid->nx - 1 && lx > bclcx + quatrclx) {
                    ax++;
                }

                for (x = bx; x <= ax; x++) {
                    xGridBound* cell = xGridIterFirstCell(grid, x, z, it);

                    while (cell) {
                        xBound* cellbound = (xBound*)(cell + 1);
                        if (xQuickCullIsects(qcr, &cellbound->qcd)) {
                            rentcb(sc, r, qcr, (xEnt*)cell->data, data);
                        }
                        cell = xGridIterNextCell(it);
                    }
                }
            }
        }
    }

    xGridBound* cell = xGridIterFirstCell(&grid->other, it);

    while (cell) {
        xBound* cellbound = (xBound*)(cell + 1);
        if (xQuickCullIsects(qcr, &cellbound->qcd)) {
            rentcb(sc, r, qcr, (xEnt*)cell->data, data);
        }
        cell = xGridIterNextCell(it);
    }
}

void xRayHitsEnt(xScene*, xRay3* r, xQCData* qcr, xEnt* ent, void* colldata)
{
    xCollis* coll = (xCollis*)colldata;

    if (!ent->chkby) return;

    if (qcr && !xQuickCullIsects(qcr, &ent->bound.qcd)) {
        return;
    }

    xCollis c;
    xRayHitsBound(r, &ent->bound, &c);

    if (!(c.flags & k_HIT_IT)) {
        return;
    }

    if (c.dist < coll->dist) {
        coll->dist = c.dist;
        coll->oid = ent->id;
        coll->optr = ent;
        coll->mptr = ent->model;
    }
}

void xRayHitsScene(xScene* sc, xRay3* r, xCollis* coll)
{
    coll->dist = 1e38f;

    xQCData q;
    xQuickCullForRay(&q, r);

    xRayHitsGrid(&colls_grid, sc, r, xRayHitsEnt, &q, coll);
    xRayHitsGrid(&colls_oso_grid, sc, r, xRayHitsEnt, &q, coll);
    xRayHitsGrid(&npcs_grid, sc, r, xRayHitsEnt, &q, coll);

    xCollis c;
    c.flags = coll->flags;
    xRayHitsEnv(r, sc->env, &c);

    if (c.dist < coll->dist) {
        coll->dist = c.dist;
        coll->oid = c.oid;
        coll->optr = NULL;
        coll->mptr = NULL;
    }

    if (coll->dist < 1e38f) {
        coll->flags |= k_HIT_IT;

        if (coll->flags & 0x200) {
            coll->norm = c.norm;
        }
    } else {
        coll->flags &= ~k_HIT_IT;
    }
}

namespace {
    struct cb_ray_hits_ent
    {
        const xRay3* ray;
        xCollis* coll;
        U8 chkby;
        U8 collType;
        S32 hit_calc;

        cb_ray_hits_ent(const xRay3& ray, xCollis& coll, U8 chkby, U8 collType) : ray(&ray), coll(&coll), chkby(chkby), collType(collType), hit_calc(coll.flags & ~k_HIT_IT)
        {
        }

        bool operator()(xEnt& ent)
        {
            if (!(ent.chkby & chkby)) {
                return true;
            }

            if (!(ent.collType & collType)) {
                return true;
            }

            if ((coll->flags & 0x4000) && !xEntIsVisible(&ent)) {
                if (ent.model && ent.model->Surf) {
                    zSurfaceProps* prop = (zSurfaceProps*)ent.model->Surf->moprops;

                    if (prop && prop->asset) {
                        if (!prop->asset->swimmable_surface) {
                            return true;
                        }
                    } else {
                        return true;
                    }
                } else {
                    return true;
                }
            }

            xCollis temp_coll;
            temp_coll.flags = ent.collLev == 5 ? 0x100 : hit_calc;
            xRayHitsBound(ray, &ent.bound, &temp_coll);

            if (!(temp_coll.flags & k_HIT_IT)) {
                return true;
            }

            if (temp_coll.dist > coll->dist) {
                return true;
            }

            if (ent.collLev == 5) {
                temp_coll.flags = hit_calc;
                xRayHitsModel(ray, ent.model, &temp_coll);

                if (!(temp_coll.flags & k_HIT_IT)) {
                    return true;
                }

                if (temp_coll.dist > coll->dist) {
                    return true;
                }
            }

            memcpy(coll, &temp_coll, sizeof(xCollis));
            coll->oid = ent.id;
            coll->optr = &ent;
            coll->mptr = ent.model;

            return true;
        }
    };

    // RESTORED: speculative
    struct cb_sphere_hits_ent
    {
        const xSphere* sphere;
        xCollis* coll;
        U8 chkby;
        U8 collType;
        S32 hit_calc;

        cb_sphere_hits_ent(const xSphere& sphere, xCollis& coll, U8 chkby, U8 collType) : sphere(&sphere), coll(&coll), chkby(chkby), collType(collType), hit_calc(coll.flags & ~k_HIT_IT)
        {
        }

        bool operator()(xEnt& ent)
        {
            if (!(ent.chkby & chkby)) {
                return true;
            }

            if (!(ent.collType & collType)) {
                return true;
            }

            if ((coll->flags & 0x4000) && !xEntIsVisible(&ent)) {
                return true;
            }

            xCollis temp_coll;
            temp_coll.flags = ent.collLev == 5 ? 0x100 : hit_calc;
            xSphereHitsBound(sphere, &ent.bound, &temp_coll);

            if (!(temp_coll.flags & k_HIT_IT)) {
                return true;
            }

            if (temp_coll.dist > coll->dist) {
                return true;
            }

            if (ent.collLev == 5) {
                temp_coll.flags = hit_calc;
                xSphereHitsModel(sphere, ent.model, &temp_coll);

                if (!(temp_coll.flags & k_HIT_IT)) {
                    return true;
                }

                if (temp_coll.dist > coll->dist) {
                    return true;
                }
            }

            memcpy(coll, &temp_coll, sizeof(xCollis));
            coll->oid = ent.id;
            coll->optr = &ent;
            coll->mptr = ent.model;

            return true;
        }
    };
    // END RESTORED
}

void xRayHitsSceneFlags(xScene* sc, xRay3* r, xCollis* coll, U8 collType, U8 chk)
{
    coll->dist = 1e38f;
    coll->flags = coll->flags & ~k_HIT_IT | 0x100;

    cb_ray_hits_ent cb(*r, *coll, collType, chk);

    xBound bound;
    bound.type = k_XBOUNDTYPE_BOX;
    xBoxFromRay(bound.box.box, *r);
    xBoxCenter(&bound.box.center, &bound.box.box);
    xQuickCullForRay(&bound.qcd, r);

    if (chk & 0x6) {
        xGridCheckBound(colls_grid, bound, bound.qcd, cb);
        xGridCheckBound(colls_oso_grid, bound, bound.qcd, cb);
    }

    if (chk & 0x8) {
        xGridCheckBound(npcs_grid, bound, bound.qcd, cb);
    }

    if (chk & 0x20) {
        xCollis temp_coll;
        temp_coll.flags = coll->flags;
        temp_coll.dist = 1e38f;
        xRayHitsEnv(r, sc->env, &temp_coll);

        if (temp_coll.dist < coll->dist) {
            memcpy(coll, &temp_coll, sizeof(xCollis));
            coll->optr = NULL;
            coll->mptr = NULL;
        }
    }
}

// RESTORED: from RotU GC
void xSphereHitsScene(xScene* sc, const xSphere* sphere, xCollis* coll, U8 collType, U8 chk)
{
    coll->dist = 1e38f;
    coll->flags = coll->flags & ~k_HIT_IT | 0x100;

    cb_sphere_hits_ent cb(*sphere, *coll, collType, chk);

    xBound bound;
    bound.type = k_XBOUNDTYPE_SPHERE;
    bound.sph = *sphere;
    xQuickCullForSphere(&bound.qcd, sphere);

    if (chk & 0x6) {
        xGridCheckBound(colls_grid, bound, bound.qcd, cb);
        xGridCheckBound(colls_oso_grid, bound, bound.qcd, cb);
    }

    if (chk & 0x8) {
        xGridCheckBound(npcs_grid, bound, bound.qcd, cb);
    }

    if (chk & 0x20) {
        xCollis temp_coll;
        temp_coll.flags = coll->flags;
        temp_coll.dist = 1e38f;
        xSphereHitsEnv(sphere, sc->env, &temp_coll);

        if (temp_coll.dist < coll->dist) {
            xMemoryCopyUpA32(coll, &temp_coll, sizeof(xCollis));
            coll->optr = NULL;
            coll->mptr = NULL;
        }
    }
}
// END RESTORED

static void ProjectTriangle(xVec3* rkD, xVec3* apkTri, F32* rfMin, F32* rfMax)
{
    *rfMin = rkD->x * apkTri[0].x + rkD->y * apkTri[0].y + rkD->z * apkTri[0].z;
    *rfMax = *rfMin;

    F32 fDot = rkD->x * apkTri[1].x + rkD->y * apkTri[1].y + rkD->z * apkTri[1].z;
    if (fDot < *rfMin)
        *rfMin = fDot;
    else if (fDot > *rfMax)
        *rfMax = fDot;

    fDot = rkD->x * apkTri[2].x + rkD->y * apkTri[2].y + rkD->z * apkTri[2].z;
    if (fDot < *rfMin)
        *rfMin = fDot;
    else if (fDot > *rfMax)
        *rfMax = fDot;
}

static void ProjectBox(xVec3* rkD, xBox* rkBox, F32* rfMin, F32* rfMax)
{
    F32 fDdC = 0.5f * (rkD->x * (rkBox->upper.x + rkBox->lower.x) + rkD->y * (rkBox->upper.y + rkBox->lower.y) + rkD->z * (rkBox->upper.z + rkBox->lower.z));
    F32 fR = 0.5f * (xabs(rkD->x * (rkBox->upper.x - rkBox->lower.x)) + xabs(rkD->y * (rkBox->upper.y - rkBox->lower.y)) + xabs(rkD->z * (rkBox->upper.z - rkBox->lower.z)));

    *rfMin = fDdC - fR;
    *rfMax = fDdC + fR;
}

static U32 Mgc_TriBoxTest(xVec3* apkTri, xBox* rkBox)
{
    F32 fMin0, fMax0, fMin1, fMax1;
    xVec3 kD, akE[3], baxis;
    U32 andflag = 0x3F, curflag;

    for (S32 i = 0; i < 3; i++) {
        curflag = 0;

        if (apkTri[i].x > rkBox->upper.x) curflag |= 0x1;
        else if (apkTri[i].x < rkBox->lower.x) curflag |= 0x2;
        if (apkTri[i].y > rkBox->upper.y) curflag |= 0x4;
        else if (apkTri[i].y < rkBox->lower.y) curflag |= 0x8;
        if (apkTri[i].z > rkBox->upper.z) curflag |= 0x10;
        else if (apkTri[i].z < rkBox->lower.z) curflag |= 0x20;

        if (curflag == 0) {
            return TRUE;
        }

        andflag &= curflag;
    }

    if (andflag) {
        return FALSE;
    }

    akE[0].x = apkTri[1].x - apkTri[0].x, akE[0].y = apkTri[1].y - apkTri[0].y, akE[0].z = apkTri[1].z - apkTri[0].z;
    akE[1].x = apkTri[2].x - apkTri[0].x, akE[1].y = apkTri[2].y - apkTri[0].y, akE[1].z = apkTri[2].z - apkTri[0].z;
    kD.x = akE[0].y * akE[1].z - akE[1].y * akE[0].z, kD.y = akE[0].z * akE[1].x - akE[1].z * akE[0].x, kD.z = akE[0].x * akE[1].y - akE[1].x * akE[0].y;
    fMin0 = kD.x * apkTri[0].x + kD.y * apkTri[0].y + kD.z * apkTri[0].z;
    fMax0 = fMin0;
    ProjectBox(&kD, rkBox, &fMin1, &fMax1);
    if (fMax1 < fMin0 || fMax0 < fMin1) {
        return FALSE;
    }

    baxis.x = 0.0f;
    baxis.y = 0.0f;
    baxis.z = 0.0f;
    akE[2].x = akE[1].x - akE[0].x, akE[2].y = akE[1].y - akE[0].y, akE[2].z = akE[1].z - akE[0].z;

    for (S32 i0 = 0; i0 < 3; i0++) {
        for (S32 i1 = 0; i1 < 3; i1++) {
            ((F32*)&baxis)[i1] = 1.0f;
            kD.x = baxis.z * akE[i0].y - baxis.y * akE[i0].z, kD.y = baxis.x * akE[i0].z - baxis.z * akE[i0].x, kD.z = baxis.y * akE[i0].x - baxis.x * akE[i0].y;
            ((F32*)&baxis)[i1] = 0.0f;
            ProjectTriangle(&kD, apkTri, &fMin0, &fMax0);
            ProjectBox(&kD, rkBox, &fMin1, &fMax1);
            if (fMax1 < fMin0 || fMax0 < fMin1)
                return FALSE;
        }
    }

    return TRUE;
}

static xMat4x3* sBoxTestMat;
static bool sUseJSP;
static void* sTestOPtr;
static xModelInstance* sTestMPtr;
static U8 sNearestChk;
static U8 sNearestCollType;
static xBound sNearestBound;
static RpIntersection sSphereIsx;

static RpCollisionTriangle* nearestFloorCB(RpIntersection*, RpCollisionTriangle* collTriangle, RwReal, void* data)
{
    xNearFloorPoly* nfpoly = (xNearFloorPoly*)data;
    F32 currnear = nfpoly->neardist;
    xVec3 currpt = { 0.0f, 0.0f, 0.0f };
    S32 currvert = 0, curredge = 0;
    xVec3 xformVert[4], xformNorm;
    U32 potentialOID;
    S32 i;
    F32 pdx[NUM_TRI_VERTS], pdz[NUM_TRI_VERTS];
    F32 numer, denom, t, testdist2, neardx, neardz, nearmag;

    if (sUseJSP) {
        if (((xClumpCollBSPTriangle*)collTriangle->index)->flags & 0x10) {
            return collTriangle;
        }

        potentialOID = ((xClumpCollBSPTriangle*)collTriangle->index)->matIndex;
    } else {
        potentialOID = collTriangle->index;
    }

    if (sBoxTestMat) {
        xMat4x3Toworld(&xformVert[0], sBoxTestMat, (xVec3*)collTriangle->vertices[0]);
        xMat4x3Toworld(&xformVert[1], sBoxTestMat, (xVec3*)collTriangle->vertices[1]);
        xMat4x3Toworld(&xformVert[2], sBoxTestMat, (xVec3*)collTriangle->vertices[2]);
        xMat3x3RMulVec(&xformNorm, sBoxTestMat, (xVec3*)&collTriangle->normal);
        xVec3Normalize(&xformNorm, &xformNorm);
    } else {
        xformVert[0] = *(xVec3*)collTriangle->vertices[0];
        xformVert[1] = *(xVec3*)collTriangle->vertices[1];
        xformVert[2] = *(xVec3*)collTriangle->vertices[2];
        xformNorm = *(xVec3*)&collTriangle->normal;
    }

    if (xformNorm.y < nfpoly->floorDot) {
        return collTriangle;
    }

    for (i = 0; i < NUM_TRI_VERTS; i++) {
        pdx[i] = nfpoly->center.x - xformVert[i].x;
        pdz[i] = nfpoly->center.z - xformVert[i].z;
    }

    F32 cross0 = pdx[0] * pdz[1] - pdz[0] * pdx[1],
        cross1 = pdx[1] * pdz[2] - pdz[1] * pdx[2],
        cross2 = pdx[2] * pdz[0] - pdz[2] * pdx[0];

    if ((cross0 >= 0.0f && cross1 >= 0.0f && cross2 >= 0.0f) || (cross0 <= 0.0f && cross1 <= 0.0f && cross2 <= 0.0f)) {
        nfpoly->neardist = 0.0f;
        nfpoly->vert[0] = xformVert[0];
        nfpoly->vert[1] = xformVert[1];
        nfpoly->vert[2] = xformVert[2];
        nfpoly->optr = sTestOPtr;
        nfpoly->mptr = sTestMPtr;
        nfpoly->oid = potentialOID;
        return NULL;
    }

    xformVert[3] = xformVert[0];

    for (i = 0; i < 3; i++) {
        xVec3 edgevec, centvec, testpt;

        edgevec.x = xformVert[i + 1].x - xformVert[i].x;
        edgevec.y = xformVert[i + 1].y - xformVert[i].y;
        edgevec.z = xformVert[i + 1].z - xformVert[i].z;
        centvec.x = nfpoly->center.x - xformVert[i].x;
        centvec.y = nfpoly->center.y - xformVert[i].y;
        centvec.z = nfpoly->center.z - xformVert[i].z;
        numer = centvec.x * edgevec.x + centvec.y * edgevec.y + centvec.z * edgevec.z;
        denom = xsqr(edgevec.x) + xsqr(edgevec.y) + xsqr(edgevec.z);

        if (denom < 0.000001f) {
            return collTriangle;
        }

        if (numer <= 0.0f) {
            testdist2 = xsqr(centvec.x) + xsqr(centvec.y) + xsqr(centvec.z);
            if (testdist2 < currnear) {
                currnear = testdist2;
                currpt = xformVert[i];
                currvert = i;
                curredge = -1;
            }
        }
        else if (numer < denom) {
            t = numer / denom;
            testpt.x = t * edgevec.x + xformVert[i].x;
            testpt.y = t * edgevec.y + xformVert[i].y;
            testpt.z = t * edgevec.z + xformVert[i].z;
            testdist2 = xsqr(nfpoly->center.x - testpt.x) + xsqr(nfpoly->center.y - testpt.y) + xsqr(nfpoly->center.z - testpt.z);

            if (testdist2 < currnear) {
                currnear = testdist2;
                currpt = testpt;
                currvert = -1;
                curredge = i;
            }
        }
    }

    if (currnear != nfpoly->neardist) {
        if (nfpoly->facingDot > 0.0f) {
            neardx = currpt.x - nfpoly->center.x;
            neardz = currpt.z - nfpoly->center.z;
            nearmag = xsqrt(xsqr(neardx) + xsqr(neardz));
            if (nearmag < 0.000001f)
                return collTriangle;
            if ((neardx * nfpoly->facingVec.x + neardz * nfpoly->facingVec.z) / nearmag < nfpoly->facingDot) {
                return collTriangle;
            }
        }

        nfpoly->vert[0] = xformVert[0];
        nfpoly->vert[1] = xformVert[1];
        nfpoly->vert[2] = xformVert[2];
        nfpoly->neardist = currnear;
        nfpoly->nearpt = currpt;
        nfpoly->nearvert = currvert;
        nfpoly->nearedge = curredge;
        nfpoly->optr = sTestOPtr;
        nfpoly->mptr = sTestMPtr;
        nfpoly->oid = potentialOID;
    }

    return collTriangle;
}

static RpCollisionTriangle* boxNearestFloorCB(RpIntersection* intersection, RpCollisionTriangle* collTriangle, RwReal distance, void* data)
{
    xNearFloorPoly* nfpoly = (xNearFloorPoly*)data;
    xVec3 xformVert[3];

    if (sBoxTestMat) {
        xMat4x3Toworld(&xformVert[0], sBoxTestMat, (xVec3*)collTriangle->vertices[0]);
        xMat4x3Toworld(&xformVert[1], sBoxTestMat, (xVec3*)collTriangle->vertices[1]);
        xMat4x3Toworld(&xformVert[2], sBoxTestMat, (xVec3*)collTriangle->vertices[2]);
    } else {
        xformVert[0] = *(xVec3*)collTriangle->vertices[0];
        xformVert[1] = *(xVec3*)collTriangle->vertices[1];
        xformVert[2] = *(xVec3*)collTriangle->vertices[2];
    }

    if (Mgc_TriBoxTest(xformVert, &nfpoly->box)) {
        return nearestFloorCB(intersection, collTriangle, distance, data);
    }

    return collTriangle;
}

static RpCollisionTriangle* sectorNearestFloorCB(RpIntersection* intersection, RpWorldSector*, RpCollisionTriangle* collTriangle, RwReal distance, void* data)
{
    return nearestFloorCB(intersection, collTriangle, distance, data);
}

static S32 gridNearestFloorCB(xEnt* ent, void* cbdata)
{
    xNearFloorPoly* nfpoly = (xNearFloorPoly*)cbdata;

    if (!ent->model || !ent->model->Surf || !ent->model->Surf->moprops) {
        return TRUE;
    }

    zSurfaceProps* props = (zSurfaceProps*)ent->model->Surf->moprops;
    bool flag = (props->asset->phys_flags & 0x40) != 0;

    if (!flag) {
        return TRUE;
    }

    if (sNearestChk && !(ent->collType & sNearestChk)) {
        return TRUE;
    }

    if (sNearestCollType && !(ent->chkby & sNearestCollType)) {
        return TRUE;
    }

    xCollis tempC;
    tempC.flags = 0;
    xBoundHitsBound(&sNearestBound, &ent->bound, &tempC);

    if (!(tempC.flags & k_HIT_IT)) {
        return TRUE;
    }

    if (ent->collLev == 5) {
        sTestOPtr = ent;
        sTestMPtr = ent->collModel ? ent->collModel : ent->model;

        RwFrame* frame = RpAtomicGetFrame(sTestMPtr->Data);
        RwFrameTransform(frame, sTestMPtr->Mat, rwCOMBINEREPLACE);
        sBoxTestMat = (xMat4x3*)sTestMPtr->Mat;
        RpAtomicForAllIntersections(sTestMPtr->Data, &sSphereIsx, boxNearestFloorCB, nfpoly);
    }

    return TRUE;
}

U32 xSceneNearestFloorPoly(xScene* sc, xNearFloorPoly* nfpoly, U8 collType, U8 chk)
{
    sNearestBound.type = k_XBOUNDTYPE_BOX;
    sNearestBound.box.box = nfpoly->box;
    xVec3Lerp(&sNearestBound.box.center, &sNearestBound.box.box.upper, &sNearestBound.box.box.lower, 0.5f);
    xQuickCullForBox(&sNearestBound.qcd, &sNearestBound.box.box);

    RpIntersection isx;
    isx.type = rpINTERSECTBOX;
    isx.t.box.sup = *(RwV3d*)&sNearestBound.box.box.upper;
    isx.t.box.inf = *(RwV3d*)&sNearestBound.box.box.lower;

    sSphereIsx.type = rpINTERSECTSPHERE;
    sSphereIsx.t.sphere.radius = xsqrt(0.25f * (xsqr(isx.t.box.sup.x - isx.t.box.inf.x) + xsqr(isx.t.box.sup.y - isx.t.box.inf.y) + xsqr(isx.t.box.sup.z - isx.t.box.inf.z)));
    sSphereIsx.t.sphere.center = *(RwV3d*)&sNearestBound.box.center;

    nfpoly->neardist = 1e38f;
    nfpoly->center = sNearestBound.box.center;

    nfpoly->oid = 0;
    nfpoly->optr = NULL;
    nfpoly->mptr = NULL;

    if (chk & 0x6) {
        sNearestChk = chk;
        sNearestCollType = collType;
        xGridCheckPosition(&colls_grid, &sNearestBound.box.center, &sNearestBound.qcd, gridNearestFloorCB, nfpoly);
        xGridCheckPosition(&colls_oso_grid, &sNearestBound.box.center, &sNearestBound.qcd, gridNearestFloorCB, nfpoly);

        sNearestChk = 0;
        sNearestCollType = 0;
        xGridCheckPosition(&grabbable_grid, &sNearestBound.box.center, &sNearestBound.qcd, gridNearestFloorCB, nfpoly);
    }

    if (chk & 0x20) {
        sTestOPtr = NULL;
        sTestMPtr = NULL;
        sBoxTestMat = NULL;
        sUseJSP = true;

        for (S32 i = 0; i < sc->env->geom->jsp_count; i++) {
            if (xEnvIsJSPActive(sc->env, i)) {
                F32 test_dist = nfpoly->neardist;
                xClumpColl_ForAllIntersections(sc->env->geom->jsp_list[i]->colltree, &isx, sectorNearestFloorCB, nfpoly);

                if (nfpoly->neardist < test_dist) {
                    nfpoly->oid = get_matid(sc->env->geom->jsp_list[i], nfpoly->oid, 0);
                }
            }
        }

        sUseJSP = false;
    }

    if (nfpoly->neardist != 1e38f) {
        nfpoly->neardist = xsqrt(nfpoly->neardist);
        return TRUE;
    }

    return FALSE;
}
