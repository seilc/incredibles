#ifndef ZOTHERASSET_H
#define ZOTHERASSET_H

#include "xDynAsset.h"
#include "xVec3.h"

struct location_asset : xDynAsset
{
    xVec3 loc;

    static char* type_name() { return "location"; }
};

struct pointer_asset : xDynAsset
{
    xVec3 loc;
    F32 yaw;
    F32 pitch;
    F32 roll;

    static char* type_name() { return "pointer"; }
};

#endif
