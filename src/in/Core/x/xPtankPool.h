#ifndef XPTANKPOOL_H
#define XPTANKPOOL_H

#include "xMath2.h"
#include "xMath3.h"
#include "xColor.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rpptank.h>

enum ptank_group_type
{
    PGT_COLOR_MAT,
    PGT_COLOR_MAT_UV2,
    PGT_POS_COLOR_SIZE,
    PGT_POS_COLOR_SIZE_UV2,
    PGT_POS_COLOR_SIZE_ROT,
    PGT_POS_COLOR_SIZE_ROT_UV2,
    MAX_PGT
};

struct ptank_pool
{
    enum
    {
        FLAG_NOZTEST = 0x1,
        FLAG_ZWRITE = 0x2,
        FLAG_ORDERED = 0x4,
        FLAG_BRIGHT = 0x8
    };

    struct render_state
    {
        RwTexture* texture;
        U32 src_blend;
        U32 dst_blend;
        S32 flags;
    };

    render_state rs;
    U32 order_group;
    S32 order_index;
    size_t used;
    RpAtomic* ptank;

    bool valid() const { return ptank != NULL; }

    void reset()
    {
        ptank = NULL;
        used = 0xffff;
    }

    void flush()
    {
        if (valid())
            unlock_block();
    }

    void unlock_block()
    {
        RpPTankAtomicUnlock(ptank);
        RpPTankAtomicSetActiveParticlesCount(ptank, used);
    }
};

struct ptank_pool__color_mat : ptank_pool
{
    xColor* color;
    xMat4x3* mat;
    RwInt32 stride_color;
    RwInt32 stride_mat;
};

struct ptank_pool__color_mat_uv2 : ptank_pool
{
    xColor* color;
    xMat4x3* mat;
    xVec2* uv;
    RwInt32 stride_color;
    RwInt32 stride_mat;
    RwInt32 stride_uv;
};

struct ptank_pool__pos_color_size : ptank_pool
{
    xVec3* pos;
    xColor* color;
    xVec2* size;
    RwInt32 stride_pos;
    RwInt32 stride_color;
    RwInt32 stride_size;
};

struct ptank_pool__pos_color_size_uv2 : ptank_pool
{
    xVec3* pos;
    xColor* color;
    xVec2* size;
    xVec2* uv;
    RwInt32 stride_pos;
    RwInt32 stride_color;
    RwInt32 stride_size;
    RwInt32 stride_uv;
};

struct ptank_pool__pos_color_size_rot : ptank_pool
{
    xVec3* pos;
    xColor* color;
    xVec2* size;
    F32* rot;
    RwInt32 stride_pos;
    RwInt32 stride_color;
    RwInt32 stride_size;
    RwInt32 stride_rot;
};

struct ptank_pool__pos_color_size_rot_uv2 : ptank_pool
{
    xVec3* pos;
    xColor* color;
    xVec2* size;
    xVec2* uv;
    F32* rot;
    RwInt32 stride_pos;
    RwInt32 stride_color;
    RwInt32 stride_size;
    RwInt32 stride_rot;
    RwInt32 stride_uv;
};

#endif
