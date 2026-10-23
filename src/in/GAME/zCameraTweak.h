#ifndef ZCAMERATWEAK_H
#define ZCAMERATWEAK_H

#include "xDynAsset.h"

struct CameraTweak_asset : xDynAsset
{
    S32 priority;
    F32 time;
    F32 pitch_adjust;
    F32 dist_adjust;
};

struct zCameraTweak : xBase
{
    CameraTweak_asset* casset;
};

extern F32 zcam_near_d;
extern F32 zcam_near_h;
extern F32 zcam_near_pitch;
extern F32 zcam_far_d;
extern F32 zcam_far_h;
extern F32 zcam_far_pitch;

void zCameraTweakGlobal_Init();
void zCameraTweakGlobal_Add(U32 owner, F32 priority, F32 time, F32 pitch,
                            F32 distMult);
void zCameraTweakGlobal_Remove(U32 owner);
void zCameraTweakGlobal_Reset();
void zCameraTweak_Init(xBase& data, xDynAsset& asset, ulong32);
void zCameraTweak_Init(zCameraTweak* tweak, CameraTweak_asset* asset);
void zCameraTweak_Save(zCameraTweak* tweak, xSerial* s);
void zCameraTweak_Load(zCameraTweak* tweak, xSerial* s);
void zCameraTweak_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif