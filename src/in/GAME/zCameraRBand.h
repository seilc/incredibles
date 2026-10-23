#ifndef ZCAMERARBAND_H
#define ZCAMERARBAND_H

#include "xDynAsset.h"
#include "zSlideCamera.h"

struct zCameraRBandAsset : xDynAsset
{
    F32 spring_len;
    F32 spring_const;
    F32 damp_const;
    F32 vertical_offset;
};

struct zCameraRBandObject : xBase
{
    zCameraRBandAsset* asset;
};

extern zSlideCam* rbCam;
extern bool zRBAND_CAMERA_ACTIVE;

void zCameraRBandAsset_Init(xBase& data, xDynAsset& asset, ulong32);
void zCameraRBandAsset_Init(zCameraRBandObject* object, zCameraRBandAsset* asset);
void zCameraRBandAsset_EventCB(xBase*, xBase*, U32 toEvent, const F32*, xBase*, U32);

#endif