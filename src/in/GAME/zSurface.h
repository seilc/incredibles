#ifndef ZSURFACE_H
#define ZSURFACE_H

#include "xSurface.h"
#include "xMath3.h"
#include "zHitParameters.h"

#include <rwcore.h>

struct zSurfMatFX
{
    U32 flags;
    U32 bumpmapID;
    U32 envmapID;
    F32 shininess;
    F32 bumpiness;
    U32 dualmapID;
};

struct zSurfColorFX
{
    U16 flags;
    U16 mode;
    F32 speed;
};

struct zSurfTextureAnim
{
    U16 pad;
    U16 mode;
    U32 group;
    F32 speed;
};

struct zSurfUVFX
{
    S32 mode;
    F32 rot;
    F32 rot_spd;
    xVec3 trans;
    xVec3 trans_spd;
    xVec3 scale;
    xVec3 scale_spd;
    xVec3 min;
    xVec3 max;
    xVec3 minmax_spd;
};

struct zSurfAssetBase : xBaseAsset
{
    U8 game_damage_type;
    U8 game_sticky;
    U8 game_damage_flags;
    U8 surf_type;
    U8 phys_pad;
    U8 sld_start;
    U8 sld_stop;
    U8 phys_flags;
    F32 friction;
    zSurfMatFX matfx;
    zSurfColorFX colorfx;
    U32 texture_anim_flags;
    zSurfTextureAnim texture_anim[2];
    U32 uvfx_flags;
    zSurfUVFX uvfx[2];
    U8 on;
    U8 surf_pad[3];
    F32 oob_delay;
    F32 walljump_scale_xz;
    F32 walljump_scale_y;
    F32 damage_timer;
    F32 damage_bounce;
    U32 impact_sound;
};

struct zFootstepsData
{
    U32 particle_emitter;
    U32 sound;
    U32 texture;
    F32 duration;
};

struct zSurfAssetIN : zSurfAssetBase
{
    U8 dash_impact_type;
    F32 dash_impact_throw_back;
    F32 dash_spray_magnitude;
    F32 dash_cool_rate;
    F32 dash_cool_amount;
    F32 dash_pass;
    F32 dash_ramp_max_distance;
    F32 dash_ramp_min_distance;
    F32 dash_ramp_key_speed;
    F32 dash_ramp_height;
    U32 dash_ramp_target_movepoint_id;
    S32 damage_amount;
    zHitSource damage_type;
    zFootstepsData off_surface;
    zFootstepsData on_surface;
    zHitDecalData hit_decal_data[3];
    F32 off_surface_time;
    U8 swimmable_surface;
    U8 dash_fall;
    U8 need_button_press;
    U8 dash_attach;
    U8 footstep_decals;
    U8 pad1;
    U8 pad2;
    U8 pad3;
    U8 pad4;
    U8 driving_surface_type;
};

struct zSurfacePropTexAnim
{
    U16 mode;
    F32 speed;
    F32 frame;
    U32 group;
    U32 group_idx;
    xBase* group_ptr;
    RwTexture** txtr_animList;
};

struct zSurfacePropUVFX
{
    S32 mode;
    F32 rot;
    F32 rot_spd;
    F32 minmax_timer[2];
    xVec3 trans;
    xVec3 trans_spd;
    xVec3 scale;
    xVec3 scale_spd;
    xVec3 min;
    xVec3 max;
    xVec3 minmax_spd;
};

struct zSurfaceProps
{
    zSurfAssetIN* asset;
    U32 texanim_flags;
    zSurfacePropTexAnim texanim[2];
    U32 uvfx_flags;
    zSurfacePropUVFX uvfx[2];
    RwTexture* txtr_matFXDualMap;
};

#define zSurfaceGetProps(surf) ((zSurfaceProps*)((surf)->moprops))

void zSurfaceRegisterMapper(U32 assetId);
xSurface* zSurfaceGetSurface(U32 mat_id);

#endif