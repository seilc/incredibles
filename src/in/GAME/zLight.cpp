#include "xCommon.h"
#include "zLight.h"

#include "zLightEffect.h"
#include "zScene.h"
#include "zVolume.h"
#include "xEvent.h"
#include "xEnt.h"
#include "xPartition.h"
#include "xShadow.h"
#include "xString.h"
#include "xMath.h"

#include <string.h>

#define k_MAX_ZLIGHT_REGS 8

static zLight* sLight[32];
static S32 sLightTotal;
static Partition sLightPart;
static zVolume* sPartitionVolume;
static S32 gNumTemporaryLights;
static zLight* gTemporaryLights[32];

void (*sEffectFuncs[18])(zLight*, F32) = {
    NULL,
    NULL,
    zLightEffectFlickerSlow,
    zLightEffectFlicker,
    zLightEffectFlickerErratic,
    zLightEffectStrobeSlow,
    zLightEffectStrobe,
    zLightEffectStrobeFast,
    zLightEffectDimSlow,
    zLightEffectDim,
    zLightEffectDimFast,
    zLightEffectHalfDimSlow,
    zLightEffectHalfDim,
    zLightEffectHalfDimFast,
    zLightEffectRandomColSlow,
    zLightEffectRandomCol,
    zLightEffectRandomColFast,
    zLightEffectCauldron,
};

void (*sEffectInitFuncs[18])(zLight*) = {
    NULL,
    NULL,
    zLightEffectInitFlicker,
    zLightEffectInitFlicker,
    zLightEffectInitFlicker,
    zLightEffectInitStrobe,
    zLightEffectInitStrobe,
    zLightEffectInitStrobe,
    zLightEffectInitDim,
    zLightEffectInitDim,
    zLightEffectInitDim,
    zLightEffectInitHalfDim,
    zLightEffectInitHalfDim,
    zLightEffectInitHalfDim,
    zLightEffectInitRandomCol,
    zLightEffectInitRandomCol,
    zLightEffectInitRandomCol,
    zLightEffectInitCauldron,
};

static xVec3 sDefaultShadowVec = { 0.0f, 1.0f, 0.0f };

void zLightResetAll(xEnv* env)
{
    memset(sLight, 0, sizeof(sLight));
    sLightTotal = 0;

    if (sPartitionVolume) {
        xPartitionVolume(&sLightPart, sPartitionVolume, 10, 1, 10);
    } else {
        xPartitionWorld(&sLightPart, env, 10, 1, 10);
    }
}

void zLightInit(void* b, void* tasset)
{
    zLightInit((xBase*)b, (zLightAsset*)tasset);
}

void zLightInit(xBase* b, zLightAsset* tasset)
{
    zLight* t = NULL;

    xBaseInit(b, tasset);

    t = (zLight*)b;
    t->eventFunc = zLightEventCB;
    t->tasset = tasset;

    if (t->linkCount) {
        t->link = (xLinkAsset*)(t->tasset + 1);
    } else {
        t->link = NULL;
    }

    U32 itype = 1;
    switch (tasset->lightType) {
    case 0:
        itype = 1;
        break;
    case 1:
        itype = 2;
        break;
    case 2:
        itype = 1;
        break;
    case 3:
        itype = 1;
        break;
    }

    if (iLightCreate(&t->light, itype)) {
        sLight[sLightTotal++] = t;

        t->light.sph.center.x = tasset->lightSphere.center.x;
        t->light.sph.center.y = tasset->lightSphere.center.y;
        t->light.sph.center.z = tasset->lightSphere.center.z;
        t->light.sph.r = tasset->lightSphere.r;
        t->light.color.r = tasset->lightColor[0];
        t->light.color.g = tasset->lightColor[1];
        t->light.color.b = tasset->lightColor[2];
        t->light.color.a = tasset->lightColor[3];
        t->light.coneangle = tasset->lightConeAngle;
        t->light.radius_sq = xsqr(t->light.sph.r);
        iLightModify(&t->light, 0x1F);

        t->flags = 0;
        if (t->tasset->lightFlags & 0x20) {
            t->flags |= f_ZLIGHT_ON;
        }

        if (t->tasset->lightEffect) {
            t->reg = (F32*)xMEMALLOC(k_MAX_ZLIGHT_REGS * sizeof(F32), 0);
            zLightEffectSet(t, t->tasset->lightEffect);
        } else {
            t->reg = NULL;
        }

        t->true_idx = xPartitionInsert(&sLightPart, t, &t->light.sph.center);
    }
}

void zLightResolveLinks()
{
    for (S32 i = 0; i < sLightTotal; i++) {
        zLight* zl = sLight[i];

        if (zl->tasset->attachID) {
            zl->attached_to = zSceneFindObject(zl->tasset->attachID);
        } else {
            zl->attached_to = NULL;
        }
    }
}

void zLightDestroyAll()
{
    S32 total = sLightTotal;
    for (S32 i = 0; i < total; i++) {
        zLightDestroy(sLight[i]);
        sLightTotal--;
    }
    sLightTotal = 0;
}

void zLightDestroy(zLight* zl)
{
    iLightDestroy(&zl->light);
}

void zLightReset(zLight* zl)
{
    xBaseReset(zl, zl->tasset);

    zl->flags = 0;
    if (zl->tasset->lightFlags & 0x20) {
        zl->flags |= f_ZLIGHT_ON;
    }

    zLightEffectSet(zl, zl->tasset->lightEffect);
}

void zLightSave(zLight* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zLightLoad(zLight* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

void zLightEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32)
{
    zLight* t = (zLight*)to;

    switch (toEvent) {
    case eEventOn:
        t->flags |= f_ZLIGHT_ON;
        break;
    case eEventOff:
        t->flags &= ~f_ZLIGHT_ON;
        break;
    case eEventReset:
        zLightReset(t);
        break;
    }
}

void zLightUpdate(xBase* to, xScene*, F32 dt)
{
    zLight* t = (zLight*)to;

    if ((t->flags & f_ZLIGHT_ON) && t->attached_to) {
        xVec3 pos = *xEntGetPos((xEnt*)t->attached_to);
        pos.y += 1.0f;
        iLightSetPos(&t->light, &pos);
        t->light.sph.center = pos;
        t->true_idx = xPartitionUpdate(&sLightPart, t, t->true_idx, &pos);
    }

    if (sEffectFuncs[t->effect_idx]) {
        sEffectFuncs[t->effect_idx](t, dt);
    }
}

void zLightAddLocalEnv()
{
    for (S32 i = 0; i < sLightTotal; i++) {
        zLight* zlight = sLight[i];

        if (zlight->flags & f_ZLIGHT_ON) {
            if (zlight->tasset->lightFlags & 0x8) {
                iLight* light = &zlight->light;
                iLightEnv(light, 1);

                light->hw->inWorld.prev = gLightWorld->directionalLightList.link.prev;
                light->hw->inWorld.next = &gLightWorld->directionalLightList.link;
                gLightWorld->directionalLightList.link.prev->next = &light->hw->inWorld;
                gLightWorld->directionalLightList.link.prev = &light->hw->inWorld;

                gTemporaryLights[gNumTemporaryLights] = zlight;
                gNumTemporaryLights++;
            }
        }
    }
}

void zLightAddLocal(xEnt* ent)
{
    xVec3 default_light_pos = *xEntGetPos(ent);
    default_light_pos.y += 1.0f;

    if (!ent->entShadow) {
        ent->entShadow = (xEntShadow*)xMEMALLOC(sizeof(xEntShadow), 0);
        ent->entShadow->pos = default_light_pos;
        ent->entShadow->vec = sDefaultShadowVec;
    }

    xShadowSetLight(&ent->entShadow->pos, &ent->entShadow->vec, 1.0f);
}

void zLightRemoveLocalEnv()
{
    for (S32 i = 0; i < gNumTemporaryLights; i++) {
        RwLLLink* link = rwLinkListGetLastLLLink(&gLightWorld->directionalLightList);
        rwLinkListRemoveLLLink(link);
    }
    gNumTemporaryLights = 0;
}

void zLightSetVolume(zVolume* vol)
{
    if (!vol) {
        sPartitionVolume = NULL;
    } else {
        U32 lp_id = xStrHash("LIGHT_PARTITION");
        if (vol->id == lp_id) {
            sPartitionVolume = vol;
        }
    }
}

void zLightEffectSet(zLight* zl, S32 idx)
{
    if (zl->reg) {
        zl->effect_idx = idx;
        if (sEffectInitFuncs[zl->effect_idx]) {
            sEffectInitFuncs[zl->effect_idx](zl);
        }
    }
}

void zLightOn(zLight* zl, S32 on)
{
    if (on) {
        zl->flags |= f_ZLIGHT_ON;
    } else {
        zl->flags &= ~f_ZLIGHT_ON;
    }
}
