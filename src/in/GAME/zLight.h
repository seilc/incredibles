#ifndef ZLIGHT_H
#define ZLIGHT_H

#include "xBase.h"
#include "zLightAsset.h"
#include "iLight.h"

typedef struct _zLight : xBase
{
    U32 flags;
    zLightAsset* tasset;
    iLight light;
    xBase* attached_to;
    S32 true_idx;
    F32* reg;
    S32 effect_idx;
} zLight;

// zLight::flags
#define f_ZLIGHT_ON (1u<<0)

struct xEnv;
struct xEnt;
struct xScene;
struct zVolume;

void zLightResetAll(xEnv* env);
void zLightInit(void* b, void* tasset);
void zLightInit(xBase* b, zLightAsset* tasset);
void zLightResolveLinks();
void zLightDestroyAll();
void zLightDestroy(zLight* zl);
void zLightReset(zLight* zl);
void zLightSave(zLight* ent, xSerial* s);
void zLightLoad(zLight* ent, xSerial* s);
void zLightEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);
void zLightUpdate(xBase* to, xScene*, F32 dt);
void zLightAddLocalEnv();
void zLightAddLocal(xEnt* ent);
void zLightRemoveLocalEnv();
void zLightSetVolume(zVolume* vol);
void zLightEffectSet(zLight* zl, S32 idx);
void zLightOn(zLight* zl, S32 on);

#endif
