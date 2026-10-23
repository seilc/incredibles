#ifndef XPOINTER_H
#define XPOINTER_H

#include "xDynAsset.h"
#include "xMath3.h"

struct pointer_asset : xDynAsset
{
    xVec3 loc;
    F32 yaw;
    F32 pitch;
    F32 roll;
};

#endif