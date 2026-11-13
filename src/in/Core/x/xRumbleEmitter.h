#ifndef XRUMBLEEMITTER_H
#define XRUMBLEEMITTER_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xVec3.h"

namespace xRumble {

struct effectAsset : xDynAsset
{
    F32 time;
    F32 intensity;
    U32 id;
    U8 priority;
    U8 type;
    U8 rumbleInPause;
    U8 pad;
    F32 param1;
    F32 param2;
    F32 shakeMagnitude;
    F32 shakeCycleMax;
    F32 shakeRotationalMagnitude;
    bool shakeY;

    F32 GetIntensity(F32 currentTime, F32 totalTime) const;
};

struct emitterBase : xBase
{
    effectAsset* pEffectAsset;

    virtual bool GetIntensity(const xVec3& playerPos, F32& intensity) const = 0;

    virtual void SetIntensity(F32 intensity) const
    {
        pEffectAsset->intensity = intensity;
    }
};

struct effect : emitterBase
{
    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);

    effect(effectAsset* pAsset);

    virtual bool GetIntensity(const xVec3& playerPos, F32& intensity) const
    {
        intensity = pEffectAsset->intensity;
        return true;
    }
};

struct boxEmitterAsset : xDynAsset
{
    U32 effectID;
    xVec3 minCorner;
    xVec3 maxCorner;
    bool onlyOnFloor;
};

struct boxEmitter : emitterBase
{
    boxEmitterAsset* pAsset;

    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);

    boxEmitter(boxEmitterAsset* pAsset);

    virtual bool GetIntensity(const xVec3& playerPos, F32& intensity) const;
};

struct sphericalEmitterAsset : xDynAsset
{
    U32 effectID;
    F32 radius;
    xVec3 position;
    bool onlyRumbleOnY;
    bool fallOff;
    bool onlyOnFloor;
};

struct sphericalEmitter : emitterBase
{
    sphericalEmitterAsset* pAsset;
    F32 radiusSqrd;

    static void Init(xBase& data, xDynAsset& asset, size_t asset_size);

    sphericalEmitter(sphericalEmitterAsset* pAsset);

    virtual bool GetIntensity(const xVec3& playerPos, F32& intensity) const;
    virtual void UpdatePosition(const xVec3& playerPos) const;
};

}

#endif
