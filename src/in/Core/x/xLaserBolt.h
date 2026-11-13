#ifndef XLASERBOLT_H
#define XLASERBOLT_H

#include "xMath2.h"
#include "xMath3.h"
#include "xRand.h"
#include "xSndMgr.h"
#include "containers.h"

#include <rwcore.h>

struct xBase;
struct xEnt;
struct xDecalEmitter;

struct xLaserBoltEmitter
{
    // config::flags
    enum
    {
        FLAG_BRIGHT = 0x1
    };

    struct config
    {
        S32 flags;
        F32 radius;
        F32 length;
        F32 vel;
        F32 fade_dist;
        F32 kill_dist;
        F32 safe_dist;
        F32 hit_radius;
        F32 rand_ang;
        F32 scar_life;
        xVec2 bolt_uv[2];
        S32 hit_interval;
        F32 damage;
        F32 knockback;
        F32 knockback_pitch;
        U32 blend_src;
        U32 blend_dst;
    };

    struct bolt
    {
        xVec3 origin;
        xVec3 dir;
        xVec3 loc;
        xVec3 hit_norm;
        F32 dist;
        F32 hit_dist;
        F32 prev_dist;
        F32 prev_check_dist;
        xEnt* hit_ent;
        F32 emitted;
        void* context;
        iSndHandle sound;
    };

    enum fx_when_enum
    {
        FX_WHEN_LAUNCH,
        FX_WHEN_IMPACT,
        FX_WHEN_BIRTH,
        FX_WHEN_DEATH,
        FX_WHEN_HEAD,
        FX_WHEN_TAIL,
        FX_WHEN_KILL,
        MAX_FX_WHEN
    };

    enum fx_type_enum
    {
        FX_TYPE_PARTICLE,
        FX_TYPE_DECAL,
        FX_TYPE_DECAL_DIST,
        FX_TYPE_SOUND,
        FX_TYPE_CALLBACK
    };

    enum fx_orient_enum
    {
        FX_ORIENT_DEFAULT,
        FX_ORIENT_PATH,
        FX_ORIENT_IPATH,
        FX_ORIENT_HIT_NORM,
        FX_ORIENT_HIT_REFLECT,
        MAX_FX_ORIENT,
        FORCE_INT_FX_ORIENT = -1
    };

    struct effect_data
    {
        fx_type_enum type;
        fx_orient_enum orient;
        F32 rate;
        union
        {
            xDecalEmitter* decal;
            xSndGroupHandle sound;
            struct
            {
                void (*fp)(bolt&, void*);
                void* context;
            } callback;
        } data;
        F32 irate;
    };

    config cfg;
    static_queue<bolt> bolts;
    xBase* owner;
    F32 ialpha;
    F32 ilength;
    RwRaster* bolt_raster;
    S32 start_collide;
    effect_data* fx[MAX_FX_WHEN];
    size_t fxsize[MAX_FX_WHEN];

    void init(size_t max_bolts, const char* debug_name, bool);
    void set_texture(U32 id);
    void set_texture(RwTexture* texture);
    void set_texture(RwRaster* raster);
    void reset();
    void refresh_config();
    void emit(const xVec3& loc, const xVec3& dir);
    void update(F32 dt);
    void render();
    void attach_effects(fx_when_enum when, effect_data* fx, size_t fxsize);
    bool visible() const;
    void set_owner(xBase* owner);

    void perturb_dir(xVec3& dir, F32 rand_ang)
    {
        xVec3 ang = { rand_ang * (xurand() - 0.5f), rand_ang * (xurand() - 0.5f), rand_ang * (xurand() - 0.5f) };
        xMat3x3 mat;
        xMat3x3Euler(&mat, &ang);
        xMat3x3LMulVec(&dir, &mat, &dir);
    }

    void update(bolt& b, F32 dt)
    {
        b.prev_dist = b.dist;
        b.dist += cfg.vel * dt;
    }

    void pre_collide(bolt& b);
    void collide_update(bolt& b);
    void apply_damage(bolt& b);
    void reset_fx(fx_when_enum when);
    void update_fx(bolt& b, F32 prev_dist, F32 dt);
    void emit_fx(fx_when_enum when, effect_data& effect, bolt& b, F32 from_dist, F32 to_dist, F32 dt);
    void emit_decal(effect_data& effect, bolt& b, F32 from_dist, F32 to_dist, F32 dt);
    void emit_decal_dist(effect_data& effect, bolt& b, F32 from_dist, F32 to_dist, F32 dt);
    void emit_sound(fx_when_enum when, effect_data& effect, bolt& b, F32 from_dist, F32 to_dist, F32 dt);
    void debug_init(const char* debug_name);
    void debug_update(F32 dt);
    void debug_render();
    void debug_refresh_effects(fx_when_enum when);
    void log_collide_statics(bool collided);
    void log_collide_dynamics(bool collided);
};

inline void xLaserBoltEmitter::emit_fx(fx_when_enum when, effect_data& effect, bolt& b, F32 from_dist, F32 to_dist,
                                       F32 dt)
{
    switch (effect.type) {
    case FX_TYPE_DECAL:
        emit_decal(effect, b, from_dist, to_dist, dt);
        break;
    case FX_TYPE_DECAL_DIST:
        emit_decal_dist(effect, b, from_dist, to_dist, dt);
        break;
    case FX_TYPE_SOUND:
        emit_sound(when, effect, b, from_dist, to_dist, dt);
        break;
    case FX_TYPE_CALLBACK:
        effect.data.callback.fp(b, effect.data.callback.context);
        break;
    }
}

inline void xLaserBoltEmitter::debug_init(const char*) {}
inline void xLaserBoltEmitter::debug_update(F32) {}
inline void xLaserBoltEmitter::debug_render() {}
inline void xLaserBoltEmitter::debug_refresh_effects(fx_when_enum) {}
inline void xLaserBoltEmitter::log_collide_statics(bool) {}
inline void xLaserBoltEmitter::log_collide_dynamics(bool) {}

#endif
