#ifndef XENTMOTION_H
#define XENTMOTION_H

#include "xBound.h"
#include "xEntMotionAsset.h"
#include "xMath3.h"

struct xEnt;
struct xEntFrame;
struct xMovePoint;
struct xScene;
struct xSpline3;

namespace x
{
    struct NURBS;
}

enum xEntMechState
{
    k_XENTMECHSTATE_SLIDE_FORTH,
    k_XENTMECHSTATE_ROT_FORTH,
    k_XENTMECHSTATE_WAIT_RETURN,
    k_XENTMECHSTATE_SLIDE_BACK,
    k_XENTMECHSTATE_ROT_BACK,
    k_XENTMECHSTATE_WAIT_AGAIN,
    k_XENTMECHSTATE_DONE,
    k_XENTMECHSTATE_UNDONE,
    k_XENTMECHSTATE_SCALE_FORTH,
    k_XENTMECHSTATE_SCALE_BACK
};

struct xEntERData
{
    xVec3 a;
    xVec3 b;
    xVec3 dir;
    F32 et;
    F32 wet;
    F32 rt;
    F32 wrt;
    F32 p;
    F32 brt;
    F32 ert;
    S32 state;
};

struct xEntOrbitData
{
    xVec3 orig;
    xVec3 c;
    F32 a;
    F32 b;
    F32 p;
    F32 w;
};

struct xEntSpeedData
{
    F32 speed;
    F32 acceleration;
    F32 dest_speed;
};

struct xEntSplineData
{
    xEntSpeedData speed_data;
    x::NURBS* spline;
    F32 u;
    U8 arrived;
    F32 position_blend_v;
    F32 orient_blend_v;
    xQuat current_orient;
};

struct xEntMPData
{
    xEntSpeedData speed_data;
    F32 curdist;
    xMovePoint* dest;
    xMovePoint* src;
    xSpline3* spl;
    F32 dist;
    U32 padalign;
    xQuat aquat;
    xQuat bquat;
};

struct xEntMechData
{
    xBound original_bound;
    xVec3 original_position;
    xVec3 destination_position;
    xVec3 direction;
    F32 original_rotation;
    F32 destination_rotation;
    F32 sliding_speed;
    F32 rotation_speed;
    S32 state;
    F32 sliding_forth_decel_time;
    F32 rotation_forth_decel_time;
    F32 sliding_back_decel_time;
    F32 rotating_back_decel_time;
    F32 scale_speed;
    F32 current_scale;
    F32* entity_rotation_angle;
};

struct xEntPenData
{
    xVec3 top;
    F32 w;
    xMat4x3 omat;
};

struct xEntMotion
{
    xEntMotionAsset* asset;
    U8 type;
    U8 pad;
    U16 ent_motion_flags;
    F32 t;
    F32 tmr;
    F32 d;
    union
    {
        xEntERData er;
        xEntOrbitData orb;
        xEntSplineData spl;
        xEntMPData mp;
        xEntMechData mech;
        xEntPenData pen;
    };
    xEnt* owner;
    xEnt* target;
};

void xEntMotionInit(xEntMotion* motion, xEnt* owner, xEntMotionAsset* asset);
void xEntMotionSetup(xEntMotion* motion, xScene* sc);
void xEntMotionReset(xEntMotion* motion, xScene* sc);
void xEntMotionMove(xEntMotion* motion, xScene* sc, F32 dt, xEntFrame* frame);
void xEntMechForward(xEntMotion* motion);
void xEntMechReverse(xEntMotion* motion);
void xEntMotionTranslate(xEntMotion* motion, const xVec3* dpos, xMat4x3* dmat);

inline void xEntMotionStop(xEntMotion* motion)
{
    motion->ent_motion_flags |= k_XENTMOTION_STOPPED;
}

inline void xEntMotionRun(xEntMotion* motion)
{
    motion->ent_motion_flags &= (U16)~k_XENTMOTION_STOPPED;
}

inline U32 xEntERIsExtending(const xEntMotion* motion)
{
    return motion->t < motion->er.et;
}

inline U32 xEntERIsExtended(const xEntMotion* motion)
{
    return motion->t >= motion->er.et && motion->t < motion->er.brt;
}

inline U32 xEntERIsRetracting(const xEntMotion* motion)
{
    return motion->t >= motion->er.brt && motion->t < motion->er.ert;
}

inline U32 xEntERIsRetracted(const xEntMotion* motion)
{
    return motion->t >= motion->er.ert;
}

inline void xEntMPSetSpeed(xEntMotion* motion, F32 speed)
{
    motion->mp.speed_data.speed = MAX(0.0f, speed);
}

inline bool xEntMPAtEnd(xEntMotion* motion)
{
    xEntMPData* mp = &motion->mp;
    return mp->dest == mp->src || !mp->dest;
}

#endif
