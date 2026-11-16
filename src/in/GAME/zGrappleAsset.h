#ifndef ZGRAPPLEASSET_H
#define ZGRAPPLEASSET_H

#include "xDynAsset.h"
#include "xVec3.h"

struct zGrappleAsset : xDynAsset
{
    U32 object;
    xVec3 offset;
    U32 grapFlags;
};

#endif
