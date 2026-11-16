#ifndef ZCAMERARBANDASSET_H
#define ZCAMERARBANDASSET_H

#include "xDynAsset.h"

struct zCameraRBandAsset : xDynAsset
{
    F32 spring_len;
    F32 spring_const;
    F32 damp_const;
    F32 vertical_offset;
};

#endif
