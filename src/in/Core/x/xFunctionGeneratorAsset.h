#ifndef XFUNCTIONGENERATORASSET_H
#define XFUNCTIONGENERATORASSET_H

#include "xDynAsset.h"

struct xFunctionGeneratorAsset : xDynAsset
{
    F32 startCycleWidth;
    F32 middleCycleWidth;
    F32 endCycleWidth;
    F32 startPulseWidth;
    F32 middlePulseWidth;
    F32 endPulseWidth;
    F32 middleTime;
    F32 endTime;
    bool middleEnabled;
    bool endEnabled;
};

#endif
