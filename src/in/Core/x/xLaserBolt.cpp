#include "xCommon.h"
#include "xLaserBolt.h"

#include "xCam.h"
#include "xCollide.h"
#include "xDecal.h"
#include "xFXHighDynamicRange.h"
#include "xGlobals.h"
#include "xIM.h"
#include "xMath.h"
#include "xMemory.h"
#include "xScene.h"
#include "xstransvc.h"
#include "zCombat.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zPlayer.h"
#include "zReactiveAnimation.h"
#include "zRenderState.h"

#include <string.h>

void xLaserBoltEmitter::init(size_t max_bolts, const char* debug_name, bool)
{
    owner = NULL;
    bolt_raster = NULL;
    memset(fxsize, 0, sizeof(fxsize));
    bolts.init(max_bolts);
    debug_init(debug_name);
}

void xLaserBoltEmitter::set_texture(U32 id)
{
    set_texture((RwTexture*)xSTFindAsset(id, NULL));
}

void xLaserBoltEmitter::set_texture(RwTexture* texture)
{
    if (!texture) {
        bolt_raster = NULL;
    } else {
        set_texture(RwTextureGetRaster(texture));
    }
}

void xLaserBoltEmitter::set_texture(RwRaster* raster)
{
    bolt_raster = raster;
}

void xLaserBoltEmitter::reset()
{
    for (static_queue<bolt>::iterator it = bolts.begin(); it != bolts.end(); ++it) {
        bolt& b = *it;
        effect_data* itfx = fx[FX_WHEN_KILL];
        effect_data* endfx = itfx + fxsize[FX_WHEN_KILL];
        for (; itfx != endfx; ++itfx) {
            emit_fx(FX_WHEN_KILL, *itfx, b, b.hit_dist, b.hit_dist, SECS_PER_VBLANK);
        }
    }

    bolts.clear();
    start_collide = 0;

    for (S32 i = 0; i < MAX_FX_WHEN; i++) {
        reset_fx((fx_when_enum)i);
    }
}

void xLaserBoltEmitter::refresh_config()
{
    ialpha = cfg.kill_dist <= cfg.fade_dist ? 0.0f : 255.0f / (cfg.kill_dist - cfg.fade_dist);
    ilength = 1.0f / cfg.length;
}

void xLaserBoltEmitter::emit(const xVec3& loc, const xVec3& dir)
{
    if (bolts.full()) {
        bolts.pop_back();
    }

    bolt& b = bolts.push_front();
    b.origin = b.loc = loc;
    b.dir = dir;
    b.dist = b.prev_dist = b.prev_check_dist = 0.0f;
    b.hit_dist = cfg.kill_dist;
    b.hit_ent = NULL;
    b.emitted = 0.0f;
    b.context = NULL;
    b.sound = XSNDMGR_INVALID_SND_HANDLE;

    if (cfg.rand_ang > 0.0f) {
        perturb_dir(b.dir, cfg.rand_ang);
    }

    if (cfg.hit_interval > 0) {
        pre_collide(b);
    }

    start_collide++;

    effect_data* itfx = fx[FX_WHEN_LAUNCH];
    effect_data* endfx = itfx + fxsize[FX_WHEN_LAUNCH];
    for (; itfx != endfx; ++itfx) {
        emit_fx(FX_WHEN_LAUNCH, *itfx, b, 0.0f, 0.0f, SECS_PER_VBLANK);
    }
}

void xLaserBoltEmitter::update(F32 dt)
{
    debug_update(dt);

    S32 ci = cfg.hit_interval > 0 ? start_collide % cfg.hit_interval : -1000000000;

    for (static_queue<bolt>::iterator it = bolts.begin(); it != bolts.end(); ++it) {
        bolt& b = *it;
        bool collided = b.dist >= b.hit_dist;
        F32 prev_dist = b.dist;

        update(b, dt);

        if (++ci >= cfg.hit_interval) {
            ci -= cfg.hit_interval;
            if (b.dist < b.hit_dist && b.dist >= cfg.safe_dist) {
                collide_update(b);
            }
        }

        b.loc = b.origin + b.dir * (b.dist >= b.hit_dist ? b.hit_dist : b.dist);

        if (b.dist >= cfg.kill_dist) {
            if (b.sound != XSNDMGR_INVALID_SND_HANDLE) {
                xSndMgrStop(b.sound);
            }

            effect_data* itfx = fx[FX_WHEN_KILL];
            effect_data* endfx = itfx + fxsize[FX_WHEN_KILL];
            for (; itfx != endfx; ++itfx) {
                emit_fx(FX_WHEN_KILL, *itfx, b, b.hit_dist, b.hit_dist, dt);
            }

            bolts.erase(it, bolts.end());
            break;
        }

        update_fx(b, prev_dist, dt);

        if (!collided && b.dist >= b.hit_dist) {
            effect_data* itfx = fx[FX_WHEN_IMPACT];
            effect_data* endfx = itfx + fxsize[FX_WHEN_IMPACT];
            for (; itfx != endfx; ++itfx) {
                emit_fx(FX_WHEN_IMPACT, *itfx, b, b.hit_dist, b.hit_dist, dt);
            }

            if (b.hit_ent) {
                apply_damage(b);
                b.hit_ent = NULL;
            }
        }
    }

    start_collide++;
}

void xLaserBoltEmitter::render()
{
    debug_render();

    S32 bolts_size = bolts.size();
    if (bolts_size <= 0) {
        return;
    }

    xIMLockContext lock;
    xIMLock(lock, XIM_TRILIST, bolts_size * 6);

    xVec3* pos = lock.pos;
    xVec2* uv = lock.uv;
    U32* color = (U32*)lock.color;
    xMat4x3& view_mat = *(xMat4x3*)&globals.screen->icam->viewMatrix;
    F32 ialpha = this->ialpha;
    F32 thickness = 0.5f * cfg.radius;

    for (S32 i = 0; i < bolts_size; i++) {
        bolt& b = bolts[i];

        F32 dist0 = b.prev_dist - cfg.length;
        if (dist0 < 0.0f) {
            dist0 = 0.0f;
        }

        F32 dist1 = b.dist <= b.hit_dist ? b.dist : b.hit_dist;
        if (dist0 >= dist1) {
            xMemorySetV32A32(pos, 0, 6 * sizeof(xVec3));
            pos += 6;
            uv += 6;
            color += 6;
            continue;
        }

        F32 Ox = b.origin.x, Oy = b.origin.y, Oz = b.origin.z;
        F32 Vx = b.dir.x, Vy = b.dir.y, Vz = b.dir.z;
        F32 Ax = Ox + Vx * dist0, Ay = Oy + Vy * dist0, Az = Oz + Vz * dist0;
        F32 Bx = Ox + Vx * dist1, By = Oy + Vy * dist1, Bz = Oz + Vz * dist1;

        xVec3 loc0 = { Ax, Ay, Az };
        xVec3 loc1 = { Bx, By, Bz };
        xVec3 screen_loc0, screen_loc1;
        xMat4x3Toworld(&screen_loc0, &view_mat, &loc0);
        xMat4x3Toworld(&screen_loc1, &view_mat, &loc1);

        F32 rz0 = xfeq0(screen_loc0.z) ? 1.0f : 1.0f / screen_loc0.z;
        F32 rz1 = xfeq0(screen_loc1.z) ? 1.0f : 1.0f / screen_loc1.z;
        F32 sRy = screen_loc1.y * rz1 - screen_loc0.y * rz0;
        F32 sRx = screen_loc0.x * rz0 - screen_loc1.x * rz1;

        F32 Rx = view_mat.right.x * sRy + view_mat.right.y * sRx;
        F32 Ry = view_mat.up.x * sRy + view_mat.up.y * sRx;
        F32 Rz = view_mat.at.x * sRy + view_mat.at.y * sRx;
        F32 Rmag2 = Rx * Rx + Ry * Ry + Rz * Rz;
        if (Rmag2 < 0.0001f * 0.0001f) {
            Rx = thickness;
            Rz = Ry = 0.0f;
        } else {
            F32 Rscale = thickness * xinvsqrt(Rmag2);
            Rx *= Rscale;
            Ry *= Rscale;
            Rz *= Rscale;
        }

        pos[0].assign(Ax - Rx, Ay - Ry, Az - Rz);
        pos[1].assign(Bx - Rx, By - Ry, Bz - Rz);
        pos[2].assign(Ax + Rx, Ay + Ry, Az + Rz);
        pos[3].assign(Ax + Rx, Ay + Ry, Az + Rz);
        pos[4].assign(Bx - Rx, By - Ry, Bz - Rz);
        pos[5].assign(Bx + Rx, By + Ry, Bz + Rz);
        pos += 6;

        U32 ucolor;
        if (b.dist <= cfg.fade_dist) {
            ucolor = 0xFFFFFFFF;
        } else {
            ucolor = 0xFFFFFF00 + (U32)(255.9f - ialpha * (b.dist - cfg.fade_dist));
        }

        color[0] = ucolor;
        color[1] = ucolor;
        color[2] = ucolor;
        color[3] = ucolor;
        color[4] = ucolor;
        color[5] = ucolor;
        color += 6;

        uv[0].assign(0.0f, 0.0f);
        uv[1].assign(1.0f, 0.0f);
        uv[2].assign(0.0f, 1.0f);
        uv[3].assign(0.0f, 1.0f);
        uv[4].assign(1.0f, 0.0f);
        uv[5].assign(1.0f, 1.0f);
        uv += 6;
    }

    zRenderState(SDRS_LaserBolt);

    if (cfg.flags & FLAG_BRIGHT) {
        xFXHighDynamicRangeBrighten(true, false);
    }

    if (cfg.blend_src != rwBLENDSRCALPHA) {
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)cfg.blend_src);
    }

    if (cfg.blend_dst != rwBLENDINVSRCALPHA) {
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)cfg.blend_dst);
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, bolt_raster);
    xIMRender(lock, NULL);
    xIMUnlock(lock);

    if (cfg.blend_src != rwBLENDSRCALPHA) {
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    }

    if (cfg.blend_dst != rwBLENDINVSRCALPHA) {
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    }

    if (cfg.flags & FLAG_BRIGHT) {
        xFXHighDynamicRangeBrighten(false, false);
    }
}

void xLaserBoltEmitter::attach_effects(fx_when_enum when, effect_data* fx, size_t fxsize)
{
    this->fx[when] = fx;
    this->fxsize[when] = fxsize;
    reset_fx(when);
    debug_refresh_effects(when);
}

void xLaserBoltEmitter::pre_collide(bolt& b)
{
    if (cfg.safe_dist >= b.hit_dist) {
        return;
    }

    xScene& scene = *xglobals->sceneCur;
    xRay3 ray;
    ray.origin.x = b.origin.x + b.dir.x * cfg.safe_dist;
    ray.origin.y = b.origin.y + b.dir.y * cfg.safe_dist;
    ray.origin.z = b.origin.z + b.dir.z * cfg.safe_dist;
    ray.dir = b.dir;
    ray.min_t = 0.0f;
    ray.max_t = b.hit_dist - cfg.safe_dist;
    ray.flags = XRAY3_USE_MIN | XRAY3_USE_MAX;

    xCollis coll;
    coll.flags = 0x300;
    xRayHitsSceneFlags(&scene, &ray, &coll, k_XENT_COLLTYPE_PC, k_XENT_COLLTYPE_ENV);

    if (coll.flags & k_HIT_IT) {
        b.hit_dist = coll.dist + cfg.safe_dist;
        b.hit_norm = coll.norm;
        b.hit_ent = NULL;
    }

    log_collide_statics(coll.flags & k_HIT_IT);
}

namespace
{
    inline F32 ray_vs_sphere(F32 ox, F32 oy, F32 oz, F32 dx, F32 dy, F32 dz, F32 cx, F32 cy, F32 cz, F32 r)
    {
        F32 px = ox - cx, py = oy - cy, pz = oz - cz;
        F32 a = dx * dx + dy * dy + dz * dz;
        F32 b = px * dx + py * dy + pz * dz;
        F32 c = px * px + py * py + pz * pz - r * r;
        F32 d = b * b - a * c;

        if (d < 0.0f) {
            return -HUGE;
        }

        return -(b + xsqrt(d)) / a;
    }

    inline F32 ray_vs_sphere(const xVec3& origin, const xVec3& dir, const xVec3& center, F32 radius)
    {
        return ray_vs_sphere(origin.x, origin.y, origin.z, dir.x, dir.y, dir.z, center.x, center.y, center.z, radius);
    }

    bool ray_hits_player(const xRay3& ray, F32& dist, xVec3& norm)
    {
        const xSphere* hit = NULL;
        dist = ray.max_t;

        const xSphere* player_spheres[5];
        S32 player_spheres_size = ZPLAYER->GetCollisionSpheres(player_spheres);
        const xSphere** o = player_spheres;
        const xSphere** endo = o + player_spheres_size;
        for (; o != endo; ++o) {
            F32 t = ray_vs_sphere(ray.origin, ray.dir, (*o)->center, (*o)->r);
            if (t >= ray.min_t && t < dist) {
                dist = t;
                hit = *o;
            }
        }

        if (hit) {
            norm.x = ray.origin.x + ray.dir.x * dist - hit->center.x;
            norm.y = ray.origin.y + ray.dir.y * dist - hit->center.y;
            norm.z = ray.origin.z + ray.dir.z * dist - hit->center.z;
            norm.normalize();
            return true;
        }

        return false;
    }
}

void xLaserBoltEmitter::collide_update(bolt& b)
{
    xScene& scene = *xglobals->sceneCur;
    if (b.dist <= 0.0f) {
        return;
    }

    xRay3 ray;
    ray.origin = b.origin;
    ray.dir = b.dir;
    ray.min_t = b.prev_check_dist - cfg.length;
    ray.max_t = b.dist;
    ray.flags = XRAY3_USE_MIN | XRAY3_USE_MAX;
    if (ray.min_t < cfg.safe_dist) {
        ray.min_t = cfg.safe_dist;
    }

    xVec3 player_hit_norm;
    bool player_hit = ray_hits_player(ray, ray.max_t, player_hit_norm);

    xCollis scene_coll;
    scene_coll.flags = 0x300;
    xRayHitsSceneFlags(&scene, &ray, &scene_coll, k_XENT_COLLTYPE_PC,
                       k_XENT_COLLTYPE_STAT | k_XENT_COLLTYPE_DYN | k_XENT_COLLTYPE_NPC);
    if (scene_coll.flags & k_HIT_IT) {
        ray.max_t = scene_coll.dist;
    }

    if (scene_coll.flags & k_HIT_IT) {
        b.hit_dist = ray.max_t;
        b.hit_norm = scene_coll.norm;
        b.hit_ent = (xEnt*)scene_coll.optr;
    } else if (player_hit) {
        b.hit_dist = ray.max_t;
        b.hit_norm = player_hit_norm;
        b.hit_ent = ZPLAYER;
    }

    zEntSimpleObj* reactive_hits[8];
    S32 reactive_hits_size = zReactiveHitTestRay(reactive_hits, 8, ray);
    for (S32 i = 0; i < reactive_hits_size; i++) {
        zReactiveDamage(*reactive_hits[i], zHS_LASER, cfg.damage);
    }

    b.prev_check_dist = b.dist;
    log_collide_dynamics((scene_coll.flags & k_HIT_IT) || player_hit);
}

void xLaserBoltEmitter::apply_damage(bolt& b)
{
    zCombatDamageInfo com;
    com.from = owner;
    com.damage = (S32)cfg.damage;
    com.source = zHS_LASER;
    com.target = zHT_GENERAL;
    com.hitLocation.x = b.origin.x + b.dir.x * b.hit_dist;
    com.hitLocation.y = b.origin.y + b.dir.y * b.hit_dist;
    com.hitLocation.z = b.origin.z + b.dir.z * b.hit_dist;
    com.direction = b.dir;

    if (cfg.knockback <= 0.0f) {
        com.knockback.assign(0.0f, 0.0f, 0.0f);
    } else {
        F32 yaw = xatan2(b.dir.x, b.dir.z);
        F32 pitch = cfg.knockback_pitch;
        F32 sinyaw = isin(yaw);
        F32 cosyaw = icos(yaw);
        F32 sinpitch = isin(pitch);
        F32 cospitch = icos(pitch);
        com.knockback.assign(cospitch * (cfg.knockback * sinyaw), cfg.knockback * sinpitch,
                             cospitch * (cfg.knockback * cosyaw));
    }

    zCombatDamage(b.hit_ent, com);
}

void xLaserBoltEmitter::reset_fx(fx_when_enum when)
{
    effect_data* itfx = fx[when];
    effect_data* endfx = itfx + fxsize[when];
    for (; itfx != endfx; ++itfx) {
        itfx->irate = itfx->rate > 0.0f ? 1.0f / itfx->rate : 0.0f;
    }
}

void xLaserBoltEmitter::update_fx(bolt& b, F32 prev_dist, F32 dt)
{
    F32 tail_dist = b.dist - cfg.length;
    F32 head_dist = b.dist <= b.hit_dist ? b.dist : b.hit_dist;

    if (b.dist < cfg.length) {
        effect_data* itfx = fx[FX_WHEN_BIRTH];
        effect_data* endfx = itfx + fxsize[FX_WHEN_BIRTH];
        for (; itfx != endfx; ++itfx) {
            emit_fx(FX_WHEN_BIRTH, *itfx, b, 0.0f, 0.0f, dt);
        }
    } else if (tail_dist < b.hit_dist) {
        effect_data* itfx = fx[FX_WHEN_TAIL];
        effect_data* endfx = itfx + fxsize[FX_WHEN_TAIL];
        F32 from_dist = prev_dist - cfg.length;
        F32 to_dist = tail_dist;
        if (from_dist < 0.0f) {
            from_dist = 0.0f;
        }

        for (; itfx != endfx; ++itfx) {
            emit_fx(FX_WHEN_TAIL, *itfx, b, from_dist, to_dist, dt);
        }
    }

    if (prev_dist < b.hit_dist) {
        effect_data* itfx = fx[FX_WHEN_HEAD];
        effect_data* endfx = itfx + fxsize[FX_WHEN_HEAD];
        for (; itfx != endfx; ++itfx) {
            emit_fx(FX_WHEN_HEAD, *itfx, b, prev_dist, head_dist, dt);
        }
    }

    if (b.dist >= b.hit_dist && tail_dist < b.hit_dist) {
        effect_data* itfx = fx[FX_WHEN_DEATH];
        effect_data* endfx = itfx + fxsize[FX_WHEN_DEATH];
        for (; itfx != endfx; ++itfx) {
            emit_fx(FX_WHEN_DEATH, *itfx, b, b.hit_dist, b.hit_dist, dt);
        }

        if (b.sound != XSNDMGR_INVALID_SND_HANDLE) {
            xSndMgrSetVolume(b.sound, ilength * (b.hit_dist - tail_dist), false);
        }
    } else if (b.dist >= b.hit_dist && b.sound != XSNDMGR_INVALID_SND_HANDLE) {
        xSndMgrStop(b.sound);
        b.sound = XSNDMGR_INVALID_SND_HANDLE;
    }
}

void xLaserBoltEmitter::emit_decal(effect_data& effect, bolt& b, F32, F32 to_dist, F32)
{
    xMat4x3 mat;

    switch (effect.orient) {
    case FX_ORIENT_DEFAULT:
    case FX_ORIENT_PATH:
    case FX_ORIENT_IPATH:
        xMat3x3LookVec3(mat, b.dir);
        break;
    case FX_ORIENT_HIT_NORM:
        xMat3x3LookVec3(mat, b.hit_norm);
        break;
    case FX_ORIENT_HIT_REFLECT:
        break;
    }

    mat.pos = b.origin + b.dir * to_dist;
    effect.data.decal->emit(mat, -1);
}

void xLaserBoltEmitter::emit_decal_dist(effect_data& effect, bolt& b, F32 from_dist, F32 to_dist, F32)
{
    F32 start_dist = (1.0f - b.emitted) * effect.irate;
    start_dist = from_dist + start_dist;
    b.emitted += effect.rate * (to_dist - from_dist);
    S32 total = (S32)b.emitted;
    b.emitted -= total;

    if (total <= 0) {
        return;
    }

    xMat4x3 mat;

    switch (effect.orient) {
    case FX_ORIENT_DEFAULT:
    case FX_ORIENT_PATH:
    case FX_ORIENT_IPATH:
        xMat3x3LookVec3(mat, b.dir);
        break;
    case FX_ORIENT_HIT_NORM:
        xMat3x3LookVec3(mat, b.hit_norm);
        break;
    case FX_ORIENT_HIT_REFLECT:
        break;
    }

    xVec3 dloc = b.dir * effect.irate;
    mat.pos = b.origin + b.dir * start_dist;

    for (S32 i = 0; i < total; i++, mat.pos += dloc) {
        if (effect.data.decal->full()) {
            break;
        }

        effect.data.decal->emit(mat, -1);
    }
}

void xLaserBoltEmitter::emit_sound(fx_when_enum when, effect_data& effect, bolt& b, F32, F32, F32)
{
    switch (when) {
    case FX_WHEN_LAUNCH:
    case FX_WHEN_IMPACT:
        xSndMgrPlay(effect.data.sound, 0, &b.loc, NULL, NULL, NULL, NULL);
        break;
    case FX_WHEN_HEAD:
        if (b.sound == XSNDMGR_INVALID_SND_HANDLE) {
            b.sound = xSndMgrPlay(effect.data.sound, 0x800, &b.loc, NULL, NULL, NULL, NULL);
        }
        break;
    case FX_WHEN_BIRTH:
    case FX_WHEN_DEATH:
    case FX_WHEN_TAIL:
    case FX_WHEN_KILL:
        break;
    }
}
