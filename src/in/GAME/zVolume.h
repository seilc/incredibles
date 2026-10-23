#ifndef ZVOLUME_H
#define ZVOLUME_H

#include "xVolume.h"

struct zVolume : xVolume
{
};

struct PreCalcOcclude
{
    xVec4 DepthVec;
    xVec4 FrustVec[4];
};

extern S32 gOccludeCount;
extern zVolume* gOccludeList[10];
extern S32 gOccludeCalcCount;
extern PreCalcOcclude gOccludeCalc[10];

void zVolumeInit();
void zVolumeSetup();
zVolume* zVolumeGetVolume(U16 n);
void zVolume_OccludePrecalc(xVec3* camPos);
void zVolumeEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif