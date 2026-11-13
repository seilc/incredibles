#ifndef ZPLATFORM_H
#define ZPLATFORM_H

#include "xEntDrive.h"
#include "xEntMotion.h"
#include "zEnt.h"
#include "iSnd.h"

struct xMovePoint;
struct xPlatformAsset;
struct zDestructible;
struct zPlatFMRunTime;

struct zPlatform : zEnt
{
    xPlatformAsset* plat_asset;
    xEntMotion motion;
    U16 state;
    U16 plat_flags;
    F32 tmr;
    S32 ctr;
    xMovePoint* src;
    xModelInstance* am;
    xModelInstance* bm;
    S32 moving;
    xEntDrive drv;
    zPlatFMRunTime* fmrt;
    zDestructible* destructible;
    F32 elapsedTime;
    xEntOpacity opacity;
    iSndHandle sndID;
};

#endif
