#include "xCommon.h"
#include "xRumbleEmitter.h"

#include "xEvent.h"
#include "xRumbleManager.h"
#include "xGlobals.h"
#include "zMain.h"
#include "zPlayer.h"

namespace {

void emitter_EventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32)
{
    switch (toEvent) {
    case eEventStartRumbleEffect:
        xRumble::Manager::Get().Add(xglobals->pad[(U32)toParam[0]], (xRumble::emitterBase*)to);
        break;
    case eEventStopRumbleEffect:
        xRumble::Manager::Get().Stop(xglobals->pad[(U32)toParam[0]], (xRumble::emitterBase*)to);
        break;
    }
}

}

namespace xRumble {

F32 effectAsset::GetIntensity(F32 currentTime, F32 totalTime) const
{
    switch (type) {
    case 0:
        return 1.0f;
    case 1:
    {
        F32 pulsePeriod = param1 + param2;
        if (currentTime - pulsePeriod * (U32)(currentTime / pulsePeriod) < param1) {
            return 1.0f;
        } else {
            return 0.0f;
        }
    }
    case 2:
        return (1.0f - icos(2.0f * PI * param1 * currentTime)) / 2.0f;
    case 3:
        if (totalTime - currentTime <= param1) {
            return (totalTime - currentTime) / param1;
        } else {
            return 1.0f;
        }
    case 4:
        if (currentTime < param1) {
            return currentTime / param1;
        } else {
            return 1.0f;
        }
    default:
        return 0.0f;
    }
}

void effect::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    new (&data) effect((effectAsset*)&asset);
}

effect::effect(effectAsset* pAsset)
    : emitterBase()
{
    xBaseInit(this, pAsset);

    pEffectAsset = pAsset;
    eventFunc = emitter_EventCB;
}

void boxEmitter::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    new (&data) boxEmitter((boxEmitterAsset*)&asset);
}

boxEmitter::boxEmitter(boxEmitterAsset* pAsset)
    : emitterBase()
{
    xBaseInit(this, pAsset);

    U32 size;
    pEffectAsset = (effectAsset*)xSTFindAsset(pAsset->effectID, &size);

    this->pAsset = pAsset;
    eventFunc = emitter_EventCB;
}

inline bool inBox(const xVec3& v, const xVec3& min, const xVec3& max)
{
    return v.x >= min.x && v.y >= min.y && v.z >= min.z &&
           v.x <= max.x && v.y <= max.y && v.z <= max.z;
}

bool boxEmitter::GetIntensity(const xVec3& playerPos, F32& intensity) const
{
    intensity = 0.0f;

    if (pAsset->onlyOnFloor && !(ZPLAYER->collis->colls[0].flags & k_HIT_IT)) {
        return false;
    }

    if (inBox(playerPos, pAsset->minCorner, pAsset->maxCorner)) {
        intensity = pEffectAsset->intensity;
        return true;
    }

    return false;
}

void sphericalEmitter::Init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    new (&data) sphericalEmitter((sphericalEmitterAsset*)&asset);
}

sphericalEmitter::sphericalEmitter(sphericalEmitterAsset* pAsset)
    : emitterBase()
{
    xBaseInit(this, pAsset);

    this->pAsset = pAsset;
    eventFunc = emitter_EventCB;

    U32 size;
    pEffectAsset = (effectAsset*)xSTFindAsset(pAsset->effectID, &size);

    radiusSqrd = xsqr(pAsset->radius);
}

bool sphericalEmitter::GetIntensity(const xVec3& playerPos, F32& intensity) const
{
    intensity = 0.0f;

    F32 distSqrd = xVec3Dist2(&playerPos, &pAsset->position);

    if (distSqrd > radiusSqrd) {
        return false;
    }

    if (pAsset->onlyRumbleOnY && !xeq(pAsset->position.y / playerPos.y, 1.0f, 0.1f)) {
        return false;
    }

    if (pAsset->onlyOnFloor && !ZPLAYER->IsFloorColliding()) {
        return false;
    }

    if (pAsset->fallOff) {
        intensity = pEffectAsset->intensity * (1.0f - xsqrt(distSqrd) / pAsset->radius);
    } else {
        intensity = pEffectAsset->intensity;
    }

    return true;
}

void sphericalEmitter::UpdatePosition(const xVec3& playerPos) const
{
    pAsset->position = playerPos;
}

}
