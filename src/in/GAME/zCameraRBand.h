#ifndef ZCAMERARBAND_H
#define ZCAMERARBAND_H

#include "xDynAsset.h"
#include "zCameraRBandAsset.h"
#include "zSlideCamera.h"

struct zCameraRBandObject : xBase
{
    zCameraRBandAsset* asset;
};

extern zSlideCam* rbCam;
extern bool zRBAND_CAMERA_ACTIVE;

void zCameraRBandAsset_Init(xBase& data, xDynAsset& asset, size_t);
void zCameraRBandAsset_Init(zCameraRBandObject* object, zCameraRBandAsset* asset);
void zCameraRBandAsset_EventCB(xBase*, xBase*, U32 toEvent, const F32*, xBase*, U32);

#endif