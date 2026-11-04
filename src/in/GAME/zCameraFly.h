#ifndef ZCAMERAFLY_H
#define ZCAMERAFLY_H

#include "xDynAsset.h"

struct CameraFly_asset : xDynAsset
{
    U32 flyID;
};

struct zCameraFly : xBase
{
    CameraFly_asset* casset;
};

struct zFlyKey
{
    S32 frame;
    F32 matrix[12];
    F32 aperture[2];
    F32 focal;
};

struct xScene;

void zCameraFly_Init(xBase& data, xDynAsset& asset, size_t);
void zCameraFly_Init(zCameraFly* fly, CameraFly_asset* asset);
void zCameraFly_Setup(zCameraFly* fly);
void zCameraFly_Update(xBase*, xScene*, F32);
void zCameraFly_Save(zCameraFly* fly, xSerial* s);
void zCameraFly_Load(zCameraFly* fly, xSerial* s);
void zCameraFlyEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif