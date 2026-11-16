#ifndef XVOLUMEASSET_H
#define XVOLUMEASSET_H

#include "xBaseAsset.h"
#include "xBound.h"

struct xVolumeAsset : xBaseAsset
{
    U32 flags;
    xBound bound;
    F32 rot;
    F32 xpivot;
    F32 zpivot;
};

#endif
