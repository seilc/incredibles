#ifndef ZTURRET_H
#define ZTURRET_H

#include "xBase.h"

struct xEnt;
struct zTurretAsset;
struct pointer_asset;

struct zTurret : xBase
{
    zTurretAsset* asset;
    xEnt* baseEnt;
    xEnt* gunEnt;
    F32 initYaw;
    F32 yaw;
    F32 initPitch;
    F32 pitch;
    F32 animTime;
    F32 heatFraction;
    S16 currentHitPoints;
    U8 nextGunId;
    U8 shootingState;
    pointer_asset* finalPointer;
};

#endif
