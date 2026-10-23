#ifndef XFUNCTIONGENERATOR_H
#define XFUNCTIONGENERATOR_H

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

struct xFunctionGenerator : xBase
{
    const xFunctionGeneratorAsset* asset;
    bool started;
    bool currentState;
    bool override;
    F32 currentCycleWidth;
    F32 currentPulseWidth;
    F32 currentTime;
    F32 lastUpTime;
    F32 lastDownTime;

    xFunctionGenerator(const xFunctionGeneratorAsset* asset);

    bool IsEnabled() const
    {
        return (baseFlags & k_XBASE_IS_ENABLED) && started;
    }

    void Update(F32 dt);
    void HandleEvent(xBase*, U32 toEvent, const F32* toParam, xBase*, U32);
    void Start();
    void Stop();
    void Reset();
};

void xFunctionGenerator_Init(xBase& data, xDynAsset& asset, size_t);

#endif