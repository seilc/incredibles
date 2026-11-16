#ifndef ZUIMOTIONASSET_H
#define ZUIMOTIONASSET_H

#include "xBaseAsset.h"

struct zUIMotionAsset : xBaseAsset
{
    U8 cmdCount;
    U8 in;
    U8 pad[2];
    U32 cmdSize;
    F32 totalTime;
    F32 loopTime;
};

#endif
