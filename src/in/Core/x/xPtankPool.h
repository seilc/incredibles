#ifndef XPTANKPOOL_H
#define XPTANKPOOL_H

#include "xMath2.h"
#include "xMath3.h"
#include "xColor.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rpptank.h>

enum { BLOCK_SIZE = 80 };

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
    // render_state::flags
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

    void grab_block(ptank_group_type type);
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

    void next()
    {
        if (used >= BLOCK_SIZE) {
            if (valid()) {
                unlock_block();
            }
            grab_block(PGT_COLOR_MAT_UV2);
            if (!valid()) {
                return;
            }
            lock_block();
        } else {
            color = (xColor*)((U8*)color + stride_color);
            mat = (xMat4x3*)((U8*)mat + stride_mat);
            uv = (xVec2*)((U8*)uv + stride_uv);
        }

        used++;
    }

    void lock_block()
    {
        used = 0;

        RpPTankLockStruct ls_color, ls_mat, ls_uv;
        RpPTankAtomicLock(ptank, &ls_color, rpPTANKLFLAGCOLOR, rpPTANKLOCKWRITE);
        RpPTankAtomicLock(ptank, &ls_mat, rpPTANKLFLAGMATRIX, rpPTANKLOCKWRITE);
        RpPTankAtomicLock(ptank, &ls_uv, rpPTANKLFLAGVTX2TEXCOORDS, rpPTANKLOCKWRITE);

        color = (xColor*)ls_color.data;
        mat = (xMat4x3*)ls_mat.data;
        uv = (xVec2*)ls_uv.data;
        stride_color = ls_color.stride;
        stride_mat = ls_mat.stride;
        stride_uv = ls_uv.stride;
    }
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

    void next()
    {
        if (used >= BLOCK_SIZE) {
            if (valid()) {
                unlock_block();
            }
            grab_block(PGT_POS_COLOR_SIZE_ROT);
            if (!valid()) {
                return;
            }
            lock_block();
        } else {
            pos = (xVec3*)((U8*)pos + stride_pos);
            color = (xColor*)((U8*)color + stride_color);
            size = (xVec2*)((U8*)size + stride_size);
            rot = (F32*)((U8*)rot + stride_rot);
        }

        used++;
    }

    void SetRot(F32 newRot)
    {
        *rot = newRot;
    }

    void lock_block()
    {
        used = 0;

        RpPTankLockStruct ls_color, ls_pos, ls_size, ls_rot;
        RpPTankAtomicLock(ptank, &ls_pos, rpPTANKLFLAGPOSITION, rpPTANKLOCKWRITE);
        RpPTankAtomicLock(ptank, &ls_color, rpPTANKLFLAGCOLOR, rpPTANKLOCKWRITE);
        RpPTankAtomicLock(ptank, &ls_size, rpPTANKLFLAGSIZE, rpPTANKLOCKWRITE);
        RpPTankAtomicLock(ptank, &ls_rot, rpPTANKLFLAG2DROTATE, rpPTANKLOCKWRITE);

        pos = (xVec3*)ls_pos.data;
        color = (xColor*)ls_color.data;
        size = (xVec2*)ls_size.data;
        rot = (F32*)ls_rot.data;
        stride_pos = ls_pos.stride;
        stride_color = ls_color.stride;
        stride_size = ls_size.stride;
        stride_rot = ls_rot.stride;
    }
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

void xPTankPoolSceneEnter();
void xPTankPoolSceneExit();
void xPTankPoolRender();

#endif
