#include "xCommon.h"
#include "xFX.h"

#include "xDebugTweak.h"
#include "xFXHighDynamicRange.h"
#include "xCam.h"
#include "xGlobals.h"
#include "xIM.h"
#include "xMath.h"
#include "xMemMgr.h"
#include "xRenderBuffer.h"
#include "xScene.h"
#include "xString.h"
#include "xstransvc.h"
#include "iFX.h"
#include "zFX.h"
#include "zRenderState.h"
#include "zSurface.h"

#include "decomp.h"

#include <string.h>
#include <rpmatfx.h>

#define MAX_RINGS 8
#define MAX_STRIP_JOINTS 240

static void LightResetFrame(RpLight* light);
static RpMaterial* MaterialSetEnvMap(RpMaterial* material, void* data);
static RpMaterial* MaterialSetBumpMap(RpMaterial* material, void* data);
static RpMaterial* MaterialSetBumpEnvMap(RpMaterial* material, RwTexture* env, F32 env_coeff, RwTexture* bump, F32 bump_coeff);

RpAtomicCallBackRender gAtomicRenderCallBack;
RpLight* MainLight;
static U32 num_fx_atomics;
static RwFrame* g_matfx_frame;

U32 xfx_initted;

void xFXInit()
{
    if (!xfx_initted) {
        xfx_initted = TRUE;

        g_matfx_frame = RwFrameCreate();

        RpLight* light = RpLightCreate(rpLIGHTDIRECTIONAL);
        if (light) {
            RwFrame* frame = RwFrameCreate();
            if (frame) {
                RpLightSetFrame(light, frame);
                LightResetFrame(light);
                MainLight = light;
            } else {
                RpLightDestroy(light);
            }
        }

        xFXanimUVCreate();

        xFXRingInit();
    }
}

xFXRing ringlist[MAX_RINGS];

inline RwRaster* xFXRingFindRWRaster(U32 aid_texture)
{
    if (!aid_texture)
        return NULL;
    RwTexture* txtr = (RwTexture*)xSTFindAsset(aid_texture, NULL);
    if (!txtr)
        return NULL;
    RwRaster* rast = RwTextureGetRaster(txtr);
    return rast;
}

inline void xFXRingInit()
{
    for (S32 i = 0; i < MAX_RINGS; i++) {
        xFXRing* ringdata = &ringlist[i];
        ringdata->time = -1.0f;
        ringdata->rast_ring = xFXRingFindRWRaster(ringdata->texture);
    }
}

DECOMP_FORCEBLOCK((const RwIm3DVertex* verts, U32 u, S32 s) {
    *(F32*)0 = 0.0f;
    *(F32*)0 = 1.0f;
    *(F32*)0 = 2.0f * PI;
    *(F32*)0 = u;
    *(F32*)0 = s;
    xIMRenderLegacy(verts, 0, NULL, rwPRIMTYPETRILIST);
})

xFXRing* xFXRingCreate(const xVec3* pos, const xFXRing* params)
{
    xFXRing* ring = &ringlist[0];

    if (!pos || !params) {
        return NULL;
    }

    for (S32 i = 0; i < MAX_RINGS; i++, ring++) {
        if (ring->time <= 0.0f) {
            memcpy(ring, params, sizeof(xFXRing));
            ring->rast_ring = xFXRingFindRWRaster(ring->texture);
            ring->time = 0.001f;
            ring->pos = *pos;
            ring->ring_radius_delta *= 1.0f / ring->lifetime;
            ring->ring_height_delta *= 1.0f / ring->lifetime;
            ring->ring_tilt_delta *= 1.0f / ring->lifetime;
            return ring;
        }
    }

    return NULL;
}

static void xFXRingUpdate(F32 dt)
{
    xFXRing* ring = &ringlist[0];
    if (xabs(dt) < 0.001f) {
        return;
    }

    for (S32 i = 0; i < MAX_RINGS; i++, ring++) {
        if (ring->time <= 0.0f) {
            continue;
        }

        F32 lifetime = (ring->lifetime < dt) ? dt : ring->lifetime;
        ring->time += dt;
        F32 t = ring->time / lifetime;
        if (t > 1.0f) {
            ring->time = 0.0f;
            if (ring->parent) {
                *ring->parent = NULL;
            }
            ring->parent = NULL;
        }
    }
}

xTWEAKABLE F32 shiny_factor = 0.5f;

void xFX_SceneEnter(RpWorld* world)
{
    S32 i;
    RwInt32 num = RpWorldGetNumMaterials(world);

    for (i = 0; i < num; i++) {
        const xSurface* sp = zSurfaceGetSurface(i);
        zSurfaceProps* pp = (zSurfaceProps*)sp->moprops;
        if (!pp || !pp->asset) {
            continue;
        }

        zSurfMatFX* fxp = &pp->asset->matfx;

        if (fxp->flags) {
            if (fxp->flags == 0x10) {
                fxp->flags |= 0x1;
            }

            RpMaterial* mp = RpWorldGetMaterial(world, i);
            RwTexture* mptex = RpMaterialGetTexture(mp);
            if (!mptex) {
                continue;
            }

            gFXSurfaceFlags = fxp->flags;

            if (fxp->flags & 0x1) {
                RwTexture* env = (RwTexture*)xSTFindAsset(fxp->envmapID, NULL);
                if (!env) {
                    continue;
                }
                MaterialSetEnvMap(mp, env);
                RpMatFXMaterialSetEnvMapCoefficient(mp, shiny_factor * fxp->shininess);
            }

            if (fxp->flags & 0x2) {
                RwTexture* bump = (RwTexture*)xSTFindAsset(fxp->bumpmapID, NULL);
                if (!bump) {
                    continue;
                }
                MaterialSetBumpMap(mp, bump);
                RpMatFXMaterialSetBumpMapCoefficient(mp, fxp->bumpiness);
            }

            if (fxp->flags & 0x4) {
                RwTexture* env = (RwTexture*)xSTFindAsset(fxp->envmapID, NULL);
                RwTexture* bump = (RwTexture*)xSTFindAsset(fxp->bumpmapID, NULL);
                if (!env || !bump) {
                    continue;
                }
                MaterialSetBumpEnvMap(mp, env, fxp->shininess, bump, fxp->bumpiness);
            }
        }
    }

    xScene* sc = xglobals->sceneCur;

    for (i = 0; i < sc->num_act_ents; i++) {
        xEnt* ent = sc->act_ents[i];

        if (!gAtomicRenderCallBack && ent->model) {
            RpAtomicCallBackRender tmp = RpAtomicGetRenderCallBack(ent->model->Data);
            RpAtomicSetRenderCallBack(ent->model->Data, NULL);
            gAtomicRenderCallBack = RpAtomicGetRenderCallBack(ent->model->Data);
            RpAtomicSetRenderCallBack(ent->model->Data, tmp);
        }
    }

    num_fx_atomics = 0;
}

void xFX_SceneExit(RpWorld*)
{
}

void xFXUpdate(F32 dt)
{
    xFXRingUpdate(dt);
    xFXRibbon::update_all(dt);
}

static void LightResetFrame(RpLight* light)
{
    RwFrame* frame;
    RwV3d xAxis = { 1.0f, 0.0f, 0.0f };
    RwV3d yAxis = { 0.0f, 1.0f, 0.0f };

    frame = RpLightGetFrame(light);

    RwFrameRotate(frame, &xAxis, 45.0f, rwCOMBINEREPLACE);
    RwFrameRotate(frame, &yAxis, 45.0f, rwCOMBINEPOSTCONCAT);
}

static RpMaterial* MaterialDisableMatFX(RpMaterial* material, void*)
{
    RpMatFXMaterialSetEffects(material, rpMATFXEFFECTNULL);
    return material;
}

RpAtomic* AtomicDisableMatFX(RpAtomic* atomic)
{
    RpMatFXAtomicEnableEffects(atomic);

    RpGeometry* geometry = RpAtomicGetGeometry(atomic);
    if (geometry) {
        RpGeometryForAllMaterials(geometry, MaterialDisableMatFX, NULL);
    }

    return atomic;
}

static RpMaterial* MaterialSetShininess(RpMaterial* material, void*); // UNUSED

static RpMaterial* MaterialSetEnvMap(RpMaterial* material, void* data)
{
    if (!data) {
        return NULL;
    }

    RwTexture* matTexture = RpMaterialGetTexture(material);

    if (matTexture) {
        RwChar* textureName;
        RwTexture* texture = (RwTexture*)data;
        textureName = RwTextureGetName(texture);
        RwFrame* frame;

        if (texture) {
            if (gFXSurfaceFlags & 0x10) {
                if (xglobals->screen->icam)
                    frame = RwCameraGetFrame(xglobals->screen->icam);
                else
                    frame = RpLightGetFrame(MainLight);
            } else {
                frame = RpLightGetFrame(MainLight);
            }

            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTENVMAP);
            RpMatFXMaterialSetupEnvMap(material, texture, frame, FALSE, 1.0f);
        } else {
            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTNULL);
        }
    }

    return material;
}

static RpMaterial* MatUpdateFrame(RpMaterial* material, void* data); // UNUSED

DECOMP_FORCEACTIVE("spec3", "update frame\n")

DECOMP_FORCEBLOCK((xMat3x3* mat, const xVec3* from, const xVec3* to) {
    xMat3x3LookAt(mat, from, to);
})

static RpMaterial* MaterialSetBumpiness(RpMaterial* material, void*); // UNUSED

static RpMaterial* MaterialSetBumpMap(RpMaterial* material, void* data)
{
    if (!data) {
        return NULL;
    }

    RwTexture* matTexture = RpMaterialGetTexture(material);

    if (matTexture) {
        RwTexture* texture = (RwTexture*)data;

        if (texture) {
            RwFrame* frame = RpLightGetFrame(MainLight);

            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTBUMPMAP);
            RpMatFXMaterialSetupBumpMap(material, texture, frame, 1.0f);
        } else {
            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTNULL);
        }
    }

    return material;
}

static RpMaterial* MaterialSetBumpEnvMap(RpMaterial* material, RwTexture* env, F32 env_coeff, RwTexture* bump, F32 bump_coeff)
{
    if (!env || !bump) {
        return NULL;
    }

    RpMatFXMaterialSetEffects(material, rpMATFXEFFECTBUMPENVMAP);

    RwFrame* frame;
    if (gFXSurfaceFlags & 0x10) {
        frame = RwCameraGetFrame(xglobals->screen->icam);
    } else {
        frame = RpLightGetFrame(MainLight);
    }

    RpMatFXMaterialSetupEnvMap(material, env, frame, TRUE, env_coeff);

    frame = RpLightGetFrame(MainLight);
    RpMatFXMaterialSetupBumpMap(material, bump, frame, bump_coeff);

    return material;
}

static RpMaterial* MaterialResetEffects(RpMaterial* material, void*); // UNUSED
static RpAtomic* AtomicResetMaterialEffects(RpAtomic* atomic, void* data); // UNUSED

static RxPipeline* xFXanimUVPipeline;
F32 xFXanimUVRotMat0[2] = { 1.0f, 0.0f };
F32 xFXanimUVRotMat1[2] = { 0.0f, 1.0f };
F32 xFXanimUVTrans[2] = { 0.0f, 0.0f };
F32 xFXanimUVScale[2] = { 1.0f, 1.0f };

void xFXanimUVSetTranslation(const xVec3* trans)
{
    xFXanimUVTrans[0] = trans->x;
    xFXanimUVTrans[1] = trans->y;
}

void xFXanimUVSetScale(const xVec3* scale)
{
    xFXanimUVScale[0] = scale->x;
    xFXanimUVScale[1] = scale->y;
}

void xFXanimUVSetAngle(F32 angle)
{
    F32 s = isin(angle);
    F32 c = icos(angle);

    xFXanimUVRotMat0[0] = c; xFXanimUVRotMat0[1] = -s;
    xFXanimUVRotMat1[0] = s; xFXanimUVRotMat1[1] = c;
}

F32 xFXanimUV2PRotMat0[2] = { 1.0f, 0.0f };
F32 xFXanimUV2PRotMat1[2] = { 0.0f, 1.0f };
F32 xFXanimUV2PTrans[2] = { 0.0f, 0.0f };
F32 xFXanimUV2PScale[2] = { 1.0f, 1.0f };
RwTexture* xFXanimUV2PTexture;

void xFXanimUV2PSetTranslation(const xVec3* trans)
{
    xFXanimUV2PTrans[0] = trans->x;
    xFXanimUV2PTrans[1] = trans->y;
}

void xFXanimUV2PSetScale(const xVec3* scale)
{
    xFXanimUV2PScale[0] = scale->x;
    xFXanimUV2PScale[1] = scale->y;
}

void xFXanimUV2PSetAngle(F32 angle)
{
    F32 s = isin(angle);
    F32 c = icos(angle);

    xFXanimUV2PRotMat0[0] = c; xFXanimUV2PRotMat0[1] = -s;
    xFXanimUV2PRotMat1[0] = s; xFXanimUV2PRotMat1[1] = c;
}

void xFXanimUV2PSetTexture(RwTexture* texture)
{
    xFXanimUV2PTexture = texture;
}

static RwMatrix gFXUVs[2];

RpAtomic* xFXanimUVAtomicSetup(RpAtomic* atomic, bool dualPass)
{
    if (atomic) {
        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        if (geom) {
            RwInt32 numMaterials = RpGeometryGetNumMaterials(geom);
            for (S32 i = 0; i < numMaterials; i++) {
                RpMaterial* material = RpGeometryGetMaterial(geom, i);
                if (material) {
                    RwMatrixSetIdentity(&gFXUVs[0]);

                    F32 rx = xFXanimUVRotMat0[0] * xFXanimUVScale[0];
                    F32 ry = xFXanimUVRotMat0[1] * xFXanimUVScale[1];
                    F32 ux = xFXanimUVRotMat1[0] * xFXanimUVScale[0];
                    F32 uy = xFXanimUVRotMat1[1] * xFXanimUVScale[1];
                    F32 px = xFXanimUVTrans[0];
                    F32 py = xFXanimUVTrans[1];

                    gFXUVs[0].right.x = rx;
                    gFXUVs[0].right.y = ry;
                    gFXUVs[0].up.x = ux;
                    gFXUVs[0].up.y = uy;
                    gFXUVs[0].pos.x = px;
                    gFXUVs[0].pos.y = py;

                    if (dualPass) {
                        if (RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTDUALUVTRANSFORM) {
                            return NULL;
                        }

                        RwMatrixSetIdentity(&gFXUVs[1]);

                        rx = xFXanimUV2PRotMat0[0] * xFXanimUV2PScale[0];
                        ry = xFXanimUV2PRotMat0[1] * xFXanimUV2PScale[1];
                        ux = xFXanimUV2PRotMat1[0] * xFXanimUV2PScale[0];
                        uy = xFXanimUV2PRotMat1[1] * xFXanimUV2PScale[1];
                        px = xFXanimUV2PTrans[0];
                        py = xFXanimUV2PTrans[1];

                        gFXUVs[1].right.x = rx;
                        gFXUVs[1].right.y = ry;
                        gFXUVs[1].up.x = ux;
                        gFXUVs[1].up.y = uy;
                        gFXUVs[1].pos.x = px;
                        gFXUVs[1].pos.y = py;

                        RpMatFXMaterialSetupDualTexture(material, xFXanimUV2PTexture, rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
                        RpMatFXMaterialSetUVTransformMatrices(material, &gFXUVs[0], &gFXUVs[1]);
                    } else {
                        if (RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTUVTRANSFORM) {
                            return NULL;
                        }

                        RpMatFXMaterialSetEffects(material, rpMATFXEFFECTUVTRANSFORM);
                        RpMatFXMaterialSetUVTransformMatrices(material, &gFXUVs[0], NULL);
                    }
                }
            }
        }
    }

    return atomic;
}

S32 xFXanimUVCreate()
{
    if (!xFXanimUVPipeline) {
        xFXanimUVPipeline = iFXanimUVCreatePipe();
    }

    return xFXanimUVPipeline ? TRUE : FALSE;
}

namespace xFX
{
    inline void lerp(F32& v, F32 frac, F32 v0, F32 v1)
    {
        v = v0 + frac * (v1 - v0);
    }

    inline void lerp(U8& v, F32 frac, U8 v0, U8 v1)
    {
        v = (U8)(v0 + frac * ((F32)v1 - (F32)v0) + 0.5f);
    }
}

tier_queue_allocator xFXRibbon::joint_alloc;
xFXRibbon::activity_data* xFXRibbon::activities[MAX_ACTIVITIES];
S32 xFXRibbon::activities_used;
bool xFXRibbon::need_sort;

void xFXRibbon::create(S32 disableAutomaticDebugTweaks)
{
    texture_offset = 0.0f;
    texture_increment_scale = 1.0f;

    act = NULL;
    cfg = NULL;
}

void xFXRibbon::clear()
{
    if (!act) return;

    act->joints.clear();
    deactivate();
}

void xFXRibbon::load_default_config(config_type& cfg)
{
    cfg.life_time = 1.0f;
    cfg.blend_src = rwBLENDSRCALPHA;
    cfg.blend_dst = rwBLENDINVSRCALPHA;
    cfg.pivot = 0.5f;
    cfg.flags = 0;
    cfg.raster = get_raster("fx_streak1");

    static const curve_node default_curve[] =
    {
        { 0.0f, { 0xFF, 0xFF, 0xFF, 0xFF }, 1.0f },
        { 1.0f, { 0xFF, 0xFF, 0xFF, 0xFF }, 1.0f }
    };

    cfg.curve = default_curve;
    cfg.curve_size = 2;
}

void xFXRibbon::set_config(const config_type* cfg)
{
    this->cfg = cfg;

    debug_refresh_config();

    if (!act) return;

    act->ilife = 1.0f / cfg->life_time;
    act->mlife = 1000.0f * cfg->life_time;
    need_sort = true;
}

void xFXRibbon::insert(const xVec3& loc, const xVec3& norm, F32 scale, F32 alpha, S32 flags)
{
    activate();
    if (!act) {
        return;
    }

    while (act->joints.front_full() && !act->joints.empty()) act->joints.pop_back();
    if (act->joints.front_full()) return;
    act->joints.push_front();
    joint_data& joint = act->joints.front();
    joint.flags = flags & 0x1;
    joint.loc = loc;
    joint.norm = norm;
    joint.born = act->mtime;
    joint.scale = scale;
    joint.alpha = alpha;
}

void xFXRibbon::activate()
{
    if (act) return;

    if (activities_used >= MAX_ACTIVITIES) {
        return;
    }

    act = activities[activities_used];
    activities_used++;

    act->owner = this;
    act->mtime = 0;
    set_config(cfg);
}

void xFXRibbon::deactivate()
{
    if (!act) {
        return;
    }

    activities_used--;

    activity_data** move_act_position = act->position;
    activity_data* move_act = *move_act_position;
    activity_data** tail_act_position = &activities[activities_used];
    activity_data* tail_act = *tail_act_position;

    *move_act_position = tail_act;
    (*move_act_position)->position = move_act_position;
    *tail_act_position = move_act;
    (*tail_act_position)->position = tail_act_position;

    act = NULL;
    need_sort = true;
}

void xFXRibbon::update(F32 dt)
{
    if (!act) {
        return;
    }

    debug_update(dt);

    act->mtime += (U32)(1000.0f * dt);

    while (!act->joints.empty()) {
        joint_data& joint = act->joints.back();
        U32 age = act->mtime - joint.born;
        if (age < act->mlife) break;
        joint.born = act->mtime - act->mlife;
        if (act->joints.size() == 1) { act->joints.clear(); break; }
        const joint_data& joint2 = *(act->joints.end() - 2);
        U32 age2 = act->mtime - joint2.born;
        if (age2 < act->mlife) break;
        act->joints.pop_back();
    }

    if (!need_update()) deactivate();
}

void xFXRibbon::render()
{
    if (!act) {
        return;
    }

    RwIm3DVertex* verts = gRenderBuffer.m_vertex;
    xFXHighDynamicRangeBrighten((cfg->flags >> 2) & 1, false);

    act->curve_index = cfg->curve_size - 2;
    S32 size = act->joints.size();
    for (S32 it = size; it > 1; ) {
        S32 subsize = it;
        subsize = (subsize > MAX_STRIP_JOINTS) ? MAX_STRIP_JOINTS : subsize;

        S32 next_it = it - subsize + 1;
        for (S32 break_it = it - 1; break_it >= next_it; ) {
            break_it--;
            if (act->joints[break_it].flags & 0x1) {
                next_it = break_it + 1;
                subsize = it - next_it;
                break;
            }
        }

        if (subsize > 1)
            render_strip(verts, act->joints.begin() + it, subsize);

        it = next_it;
    }
}

RwRaster* xFXRibbon::get_raster(const char* name)
{
    return get_raster(xStrHash(name));
}

RwRaster* xFXRibbon::get_raster(U32 id)
{
    if (!id) return NULL;
    RwTexture* texture = (RwTexture*)xSTFindAsset(id, NULL);
    return (texture == NULL) ? NULL : RwTextureGetRaster(texture);
}

static inline void set_vert(RwIm3DVertex& vert, F32 x, F32 y, F32 z, xColor color)
{
    RwIm3DVertexSetPos(&vert, x, y, z);
    RwIm3DVertexSetRGBA(&vert, color.r, color.g, color.b, color.a);
}

static inline void set_vert_uv(RwIm3DVertex& vert, F32 u, F32 v)
{
    vert.u = u;
    vert.v = v;
}

void xFXRibbon::get_normal(xVec3& norm, const xVec3& dir, F32 orient)
{
    F32 a = isin(orient), b = icos(orient);

    F32 ax = xabs(dir.x), ay = xabs(dir.y), az = xabs(dir.z);
    if (ax < ay && ax < az) {
        norm.x = -a * (dir.y * dir.y + dir.z * dir.z);
        norm.y = a * dir.y * dir.z + b * dir.z;
        norm.z = a * dir.x * dir.z - b * dir.y;
        norm *= 1.0f / xsqrt(dir.y * dir.y + dir.z * dir.z);
    }
    else if (ay < az) {
        norm.x = a * dir.x * dir.y - b * dir.z;
        norm.y = -a * (dir.x * dir.x + dir.z * dir.z);
        norm.z = a * dir.y * dir.z + b * dir.x;
        norm *= 1.0f / xsqrt(dir.x * dir.x + dir.z * dir.z);
    } else {
        norm.x = a * dir.x * dir.z + b * dir.y;
        norm.y = a * dir.y * dir.z - b * dir.x;
        norm.z = -a * (dir.x * dir.x + dir.y * dir.y);
        norm *= 1.0f / xsqrt(dir.x * dir.x + dir.y * dir.y);
    }
}

void xFXRibbon::refresh_joint(joint_data& joint, const joint_iterator& it)
{
    if (!act) return;

    xVec3* p[2];
    if (!(joint.flags & 0x20000)) return;

    joint.flags &= ~0x20000;

    if (it == act->joints.begin()) {
        p[0] = &joint.loc;
    } else {
        p[0] = &(it - 1)->loc;
    }

    if (it + 1 == act->joints.end()) {
        p[1] = &joint.loc;
    } else {
        p[1] = &(it + 1)->loc;
    }

    xVec3 offset = *p[0] - *p[1];
    F32 mag2 = offset.length2();
    if (xfeq0(mag2)) {
        joint.norm.x = icos(joint.orient);
        joint.norm.y = isin(joint.orient);
        joint.norm.z = 0.0f;
    } else {
        xVec3 dir = offset * (1.0f / xsqrt(mag2));
        get_normal(joint.norm, dir, joint.orient);
    }
}

void xFXRibbon::eval_joint(const joint_data& joint, xColor& color, F32& width)
{
    if (!act) {
        return;
    }

    F32 frac = act->ilife * get_age(joint);
    if (frac > 1.0f) {
        frac = 1.0f;
    }

    const curve_node* curve = cfg->curve;

    for (; act->curve_index > 0; act->curve_index--) {
        if (frac >= curve[act->curve_index].time && frac <= curve[act->curve_index + 1].time) break;
    }

    const curve_node& node0 = curve[act->curve_index], & node1 = curve[act->curve_index + 1];
    F32 ifrac = 1.0f / (node1.time - node0.time), subfrac = ifrac * (frac - node0.time);

    xFX::lerp(color.r, subfrac, node0.color.r, node1.color.r);
    xFX::lerp(color.g, subfrac, node0.color.g, node1.color.g);
    xFX::lerp(color.b, subfrac, node0.color.b, node1.color.b);

    F32 alpha = 0.0f;
    xFX::lerp(alpha, subfrac, node0.color.a, node1.color.a);
    color.a = (U8)(alpha * joint.alpha + 0.5f);

    xFX::lerp(width, subfrac, node0.scale, node1.scale);
    width *= joint.scale;
}

void xFXRibbon::render_strip(RwIm3DVertex* verts, joint_iterator first, size_t size)
{
    if (!act) {
        return;
    }

    RwIm3DVertex* v = verts;
    for (joint_iterator it = first, last = first - size; it != last; ) {
        --it;
        joint_data& joint = *it;
        refresh_joint(joint, it);

        xColor color;
        color.a = color.b = color.g = color.r = 0;
        F32 width = 0.0f;
        eval_joint(joint, color, width);

        F32 off1 = cfg->pivot * width, off2 = (cfg->pivot - 1.0f) * width;
        F32 cx = joint.loc.x, cy = joint.loc.y, cz = joint.loc.z;
        F32 nx = joint.norm.x, ny = joint.norm.y, nz = joint.norm.z;
        set_vert(*v++, cx + off1 * nx, cy + off1 * ny, cz + off1 * nz, color);
        set_vert(*v++, cx + off2 * nx, cy + off2 * ny, cz + off2 * nz, color);
    }

    RwIm3DVertex* endv = v;
    v = verts;
    F32 ulookup[2] = { 0.0f, 1.0f };
    switch (cfg->flags & 0x3) {
    case 0: {
        S32 back = first.global_index() & 1;
        for (; v != endv; back ^= 1) {
            F32 u = ulookup[back];
            set_vert_uv(*v++, u, 0.0f);
            set_vert_uv(*v++, u, 1.0f);
        }
        break;
    }
    case 1: {
        F32 du = -texture_increment_scale / size;
        F32 u = 1.0f - texture_offset;
        while (v != endv) {
            set_vert_uv(*v++, u, 0.0f);
            set_vert_uv(*v++, u, 1.0f);
            u += du;
        }
        break;
    }
    case 2: {
        S32 back = first.global_index() & 1;
        for (; v != endv; back ^= 1) {
            F32 u = ulookup[back];
            set_vert_uv(*v++, 0.0f, u);
            set_vert_uv(*v++, 1.0f, u);
        }
        break;
    }
    case 3: {
        F32 du = -texture_increment_scale / size;
        F32 u = 1.0f - texture_offset;
        while (v != endv) {
            set_vert_uv(*v++, 0.0f, u);
            set_vert_uv(*v++, 1.0f, u);
            u += du;
        }
    }
    }

    xIMRenderLegacy(verts, v - verts, NULL, rwPRIMTYPETRISTRIP);
}

S32 xFXRibbon::compare_ribbons(const void* e1, const void* e2)
{
    if (e1 == e2) return 0;
    if (!e1) return 1;
    if (!e2) return -1;
    const xFXRibbon* r1 = (*(activity_data* const*)e1)->owner;
    const xFXRibbon* r2 = (*(activity_data* const*)e2)->owner;
    if (r1->cfg->raster < r2->cfg->raster) return -1;
    if (r1->cfg->raster > r2->cfg->raster) return 1;
    if (r1->cfg->blend_src < r2->cfg->blend_src) return -1;
    if (r1->cfg->blend_src > r2->cfg->blend_src) return 1;
    if (r1->cfg->blend_dst < r2->cfg->blend_dst) return -1;
    if (r1->cfg->blend_dst > r2->cfg->blend_dst) return 1;
    return 0;
}

void xFXRibbon::sort_active_ribbons()
{
    if (!need_sort || activities_used <= 0)
        return;
    qsort(activities, activities_used, sizeof(activity_data*), compare_ribbons);
    activity_data** act = activities, ** end_act = act + activities_used;
    for (; act != end_act; act++)
        (*act)->position = act;

    need_sort = false;
}

void xFXRibbon::scene_enter()
{
    joint_alloc.init(sizeof(joint_data), 32, 32);

    activities_used = 0;

    activity_data** act = activities, ** end_act = act + MAX_ACTIVITIES;
    for (; act != end_act; act++) {
        *act = new (eMemStaticType_0, 0) activity_data;
        (*act)->position = act;
        (*act)->joints.init(joint_alloc);
    }

    debug_scene_enter();
}

void xFXRibbon::update_all(F32 dt)
{
    activity_data** act = activities, ** end_act = act + activities_used;
    for (; act != end_act; act++) {
        xFXRibbon* ribbon = (*act)->owner;
        ribbon->update(dt);
    }
}

void xFXRibbon::render_all()
{
    if (activities_used <= 0) return;

    sort_active_ribbons();

    zRenderState(SDRS_Ribbon);

    RwRaster* raster = (RwRaster*)-1;
    U32 blend_src = rwBLENDSRCALPHA, blend_dst = rwBLENDINVSRCALPHA;
    activity_data** act = activities, ** end_act = act + activities_used;
    for (; act != end_act; act++) {
        xFXRibbon* ribbon = (*act)->owner;
        if (ribbon->cfg->blend_src != blend_src) {
            blend_src = ribbon->cfg->blend_src;
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)blend_src);
        }

        if (ribbon->cfg->blend_dst != blend_dst) {
            blend_dst = ribbon->cfg->blend_dst;
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)blend_dst);
        }

        if (ribbon->cfg->raster != raster) {
            raster = ribbon->cfg->raster;
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }

        ribbon->render();
    }

    if (blend_src != rwBLENDSRCALPHA) {
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    }

    if (blend_dst != rwBLENDINVSRCALPHA) {
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    }

    xFXHighDynamicRangeBrighten(false, false);
}
