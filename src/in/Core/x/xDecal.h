#ifndef XDECAL_H
#define XDECAL_H

#include "xMath2.h"
#include "xMath3.h"
#include "xColor.h"
#include "containers.h"

#include <rwcore.h>
#include <rpworld.h>

struct xEnt;
struct xModelInstance;
struct xResponseCurve;

namespace xDecal
{
    enum
    {
        BLEND_MODE_INVALID = -1,
        BLEND_MODE_AVERAGE,
        BLEND_MODE_ADDITIVE,
        BLEND_MODE_BRIGHT,
        MAX_BLEND_MODE
    };

    struct animix
    {
        F32 r;
        F32 g;
        F32 b;
        F32 a;
        F32 scale_x;
        F32 scale_y;
    };

    struct curve_node
    {
        F32 t;
        animix ani;
    };

    extern char* animix_labels[6];

    struct emit_context
    {
        // emit_context::flags
        enum
        {
            FLAG_NO_MOUNT = 0x1,
            FLAG_PERMANENT = 0x2,
            FLAG_CYCLE = 0x4,
            FLAG_ANIM_COLOR = 0x8,
            FLAG_ANIM_UV = 0x10,
            FLAG_LIT = 0x20,
            FLAG_PAUSE = 0x40,
            FLAG_SKIP_DESTRUCTIBLE = 0x80,
            FLAG_SKINNED = 0x10000,
            FLAG_ANIMATED = 0x20000,
            FLAG_HIDDEN = 0x40000
        };

        U32 owner;
        S32 flags;
        S32 priority;
        S32 max_owned;
        S32 hard_max_owned;
        F32 life;
        F32 pause_time;
        F32 accel_aging;
        F32 max_dist;
        F32 lod_dist;
        xVec2 size;
        S32 blend_mode;
        RwTexture* texture;
        xVec2 uv[2];
        const xResponseCurve* curve;
        void (*emitAtomicCB)(RpAtomic*, xMat4x3*);
        void (*destAtomicCB)(RpAtomic*);
    };

    void scene_enter();
    void scene_exit();
    void reset();
    void update(F32 dt);
    void render_all();
    void set_default_emit_context(emit_context& context);
    void emit(const emit_context& ec, const xMat4x3& mat, S32 check_flags);
    void sphere_emit(const emit_context& ec, const xVec3& center, F32 radius, S32 check_flags);
    void emit(const emit_context& ec, const xMat4x3& mat, const xModelInstance& model, bool moving_model, xEnt* driver_ent);
    void emit(const emit_context& ec, const xMat4x3& mat, RpAtomic* model, const xMat4x3* model_mat, bool moving_model,
              xEnt* driver_ent);
    void emit_env(const emit_context& ec, const xMat4x3& mat);
}

void xDecalInit();
void xDecalUpdate(F32 dt);

struct xDecalEmitter
{
    // config::flags
    enum
    {
        FLAG_NOZTEST = 0x1,
        FLAG_ORIENT_CAMERA = 0x2,
        FLAG_BRIGHT = 0x4
    };

    enum texture_mode
    {
        TM_DEFAULT,
        TM_RANDOM,
        TM_CYCLE,
        MAX_TM,
        FORCE_INT_TM = -1
    };

    struct config
    {
        U32 flags;
        F32 life_time;
        U32 blend_src;
        U32 blend_dst;
        struct
        {
            xVec2 uv[2];
            U8 rows;
            U8 cols;
            texture_mode mode;
        } texture;
    };

    struct curve_node
    {
        F32 time;
        xColor color;
        F32 scale;
    };

    struct unit_data
    {
        U8 flags;
        U8 curve_index;
        U8 u;
        U8 v;
        F32 frac;
        F32 age;
        F32 cull_size;
        xMat4x3 mat;
    };

    void init(S32 max_size, const char* debug_name);
    void set_default_config();
    void set_texture(const char* name);
    void set_texture(U32 id);
    void set_texture(RwTexture* texture);
    void refresh_config();
    void set_curve(const curve_node* curve, size_t size);
    void emit(const xMat4x3& mat, S32 texture_index);
    void emit(const xMat4x3& mat, const xVec3& scale, S32 texture_index);
    void update(F32 dt);
    bool valid() const { return curve_size != 0 && cfg.life_time > 0.0f; }
    bool need_update() const { return !units.empty() || debug_need_update(); }
    bool full() const { return units.full(); }

    config cfg;
    struct
    {
        RwTexture* asset;
        size_t units;
        xVec2 size;
        xVec2 isize;
        S32 prev;
    } texture;
    static_queue<unit_data> units;
    const curve_node* curve;
    size_t curve_size;
    size_t curve_index;
    F32 ilife;

    void update_frac(unit_data& unit);
    void get_render_data(const unit_data& unit, F32 scale, xColor& color, xMat4x3& mat, xVec2& uv0, xVec2& uv1,
                         S32 min_alpha);
    S32 select_texture_unit();
    void debug_init(const char* debug_name);
    void debug_update(F32 dt);
    void debug_update_curve();
    bool debug_need_update() const;
};

inline bool xDecalEmitter::debug_need_update() const { return false; }
inline void xDecalEmitter::debug_init(const char*) {}
inline void xDecalEmitter::debug_update(F32) {}

inline void xDecalEmitter::debug_update_curve() {}

#endif
