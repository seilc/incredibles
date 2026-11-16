#ifndef ZTRAINPATHJUNCTIONASSET_H
#define ZTRAINPATHJUNCTIONASSET_H

#include "xDynAsset.h"

struct zTrainPathJunctionAsset : xDynAsset
{
    U32 in_spline;
    bool in_is_from_forward;
    U32 out1_spline;
    bool out1_is_forward;
    U32 out2_spline;
    bool out2_is_forward;
};

#endif
