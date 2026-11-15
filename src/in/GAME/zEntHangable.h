#ifndef ZENTHANGABLE_H
#define ZENTHANGABLE_H

#include "zEnt.h"
#include "xEntHangableAsset.h"

struct zEntHangable : zEnt
{
    xEntHangableAsset* hangInfo;
    xVec3 pivot;
    xVec3 endpos;
    xVec3 vel;
    xVec3 swingplane;
    F32 grabTimer;
    F32 spin;
    U32 state;
    S32 enabled;
    zEnt* follow;
    S32 moving;
    F32 candle_timer;
    S32 candle_state;
};

void zEntHangable_SetupFX();
void zEntHangable_Init(void* ent, void* asset);
void zEntHangable_Init(zEntHangable* ent, xEntAsset* asset);
void zEntHangable_Update(zEntHangable* ent, xScene*, F32 dt);
void zEntHangableEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32);
void zEntHangable_Save(zEntHangable* ent, xSerial* s);
void zEntHangable_Load(zEntHangable* ent, xSerial* s);
void zEntHangable_Reset(zEntHangable* ent);
void zEntHangable_FollowUpdate(zEntHangable* ent);
void zEntHangable_SetFollow(zEntHangable* ent, zEnt* follow);

#endif
