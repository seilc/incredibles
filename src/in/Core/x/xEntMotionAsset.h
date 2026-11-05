#ifndef XENTMOTIONASSET_H
#define XENTMOTIONASSET_H

#include "xVec3.h"

enum xEntMotionType
{
    k_XENTMOTIONTYPE_ER,
    k_XENTMOTIONTYPE_ORBIT,
    k_XENTMOTIONTYPE_SPLINE,
    k_XENTMOTIONTYPE_MP,
    k_XENTMOTIONTYPE_MECH,
    k_XENTMOTIONTYPE_PEN,
    k_XENTMOTIONTYPE_NONE
};

enum xEntMotionFlags
{
    k_XENTMOTION_ORIENT = 0x1,
    k_XENTMOTION_STOPPED = 0x4,
    k_XENTMOTION_LOOP = 0x10
};

enum xEntMotionMPFlags
{
    k_XENTMOTIONMP_STOP_AT_END = 0x1
};

enum xEntMotionMechType
{
    k_XENTMOTIONMECH_SLIDE,
    k_XENTMOTIONMECH_ROT,
    k_XENTMOTIONMECH_SLIDE_ROT,
    k_XENTMOTIONMECH_SLIDE_THEN_ROT,
    k_XENTMOTIONMECH_ROT_THEN_SLIDE,
    k_XENTMOTIONMECH_SCALE,
    k_XENTMOTIONMECH_SLIDE_SCALE
};

enum xEntMotionMechFlags
{
    k_XENTMOTIONMECH_RETURNS = 0x1,
    k_XENTMOTIONMECH_ONCE = 0x2
};

struct xEntMotionERData
{
    xVec3 ret_pos;
    xVec3 ext_dpos;
    F32 ext_tm;
    F32 ext_wait_tm;
    F32 ret_tm;
    F32 ret_wait_tm;
};

struct xEntMotionOrbitData
{
    xVec3 center;
    F32 w;
    F32 h;
    F32 period;
};

struct xEntMotionSplineData
{
    U32 spline_id;
    F32 speed;
    F32 lean_modifier;
};

struct xEntMotionMPData
{
    U32 flags;
    U32 mp_id;
    F32 speed;
};

struct xEntMotionMechData
{
    U8 type;
    U8 flags;
    U8 sld_axis;
    U8 rot_axis;
    U8 scale_axis;
    F32 sld_dist;
    F32 sld_tm;
    F32 sld_acc_tm;
    F32 sld_dec_tm;
    F32 rot_dist;
    F32 rot_tm;
    F32 rot_acc_tm;
    F32 rot_dec_tm;
    F32 ret_delay;
    F32 post_ret_delay;
    F32 scale_amount;
    F32 scale_duration;
};

struct xEntMotionPenData
{
    U8 flags;
    U8 plane;
    U8 pad[2];
    F32 len;
    F32 range;
    F32 period;
    F32 phase;
};

struct xEntMotionAsset
{
    U8 type;
    U8 use_banking;
    U16 ent_motion_flags;
    union
    {
        xEntMotionERData er;
        xEntMotionOrbitData orb;
        xEntMotionSplineData spl;
        xEntMotionMPData mp;
        xEntMotionMechData mech;
        xEntMotionPenData pen;
    };
};

#endif
