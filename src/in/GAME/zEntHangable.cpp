#include "xCommon.h"
#include "zEntHangable.h"

#include "xEvent.h"
#include "xMath3.h"
#include "xModel.h"
#include "xUpdateCull.h"
#include "zGlobals.h"
#include "zHashes.h"
#include "zMain.h"

static U32 sChandelierHash;

void zEntHangable_SetupFX()
{
    sChandelierHash = HASH_rlii0006;
}

static void HangableSetup(zEntHangable* ent, xEntAsset* asset)
{
    ent->pflags = 0;
    ent->chkby = k_XENT_COLLTYPE_NONE;
    ent->penby = k_XENT_COLLTYPE_NONE;
    ent->update = (xEntUpdateCallback)zEntHangable_Update;
    ent->move = NULL;
    ent->eventFunc = zEntHangableEventCB;

    xEntHangableAsset* hangAsset = (xEntHangableAsset*)(asset + 1);
    ent->hangInfo = hangAsset;

    xVec3* center = &ent->asset->pos;
    ent->pivot.x = center->x;
    ent->pivot.y = center->y + hangAsset->pivotOffset;
    ent->pivot.z = center->z;
    ent->endpos.x = center->x;
    ent->endpos.y = ent->pivot.y - hangAsset->leverArm;
    ent->endpos.z = center->z;
    ent->vel.x = 0.0f;
    ent->vel.y = 0.0f;
    ent->vel.z = 0.0f;
    ent->grabTimer = 0.0f;
    ent->spin = 0.0f;
    ent->state = 0;
    ent->follow = NULL;
    ent->enabled = 1;
    ent->moving = FALSE;
    ent->candle_timer = 3.0f;
    ent->candle_state = 0;

    xMat3x3 hackMat;
    xMat3x3Euler(&hackMat, ent->asset->ang.x, ent->asset->ang.y, ent->asset->ang.z);
    ent->swingplane.x = hackMat.right.x;
    ent->swingplane.y = hackMat.right.y;
    ent->swingplane.z = hackMat.right.z;
}

void zEntHangable_Init(void* ent, void* asset)
{
    zEntHangable_Init((zEntHangable*)ent, (xEntAsset*)asset);
}

void zEntHangable_Init(zEntHangable* ent, xEntAsset* asset)
{
    zEntInit(ent, asset, 'HANG');

    if (ent->linkCount) {
        ent->link = (xLinkAsset*)((U8*)ent->asset + sizeof(xEntAsset) + sizeof(xEntHangableAsset));
    } else {
        ent->link = NULL;
    }

    HangableSetup(ent, asset);
}

void zEntHangable_Update(zEntHangable*, xScene*, F32)
{
}

static void zEntHangableMountFX(zEntHangable*)
{
}

void zEntHangableEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget, U32)
{
    zEntHangable* ent = (zEntHangable*)to;
    U32 oldvisible = xEntIsVisible(ent);

    switch (toEvent) {
    case eEventVisible:
    case eEventFastVisible:
        xEntShow(ent);
        break;
    case eEventInvisible:
    case eEventFastInvisible:
        xEntHide(ent);
        break;
    case eEventCollisionOn:
        ent->enabled = 1;
        break;
    case eEventCollisionOff:
        ent->enabled = 0;
        break;
    case eEventCollisionReset:
        ent->enabled = 1;
        break;
    case eEventMount:
        zEntHangableMountFX(ent);
        if (ent->asset->modelInfoID == sChandelierHash && ent->candle_state == 0) {
            ent->candle_timer = 3.0f;
        }
        break;
    case eEventDismount:
        ent->enabled = -2;
        break;
    case eEventCollision_Visible_On:
        xEntShow(ent);
        ent->enabled = 1;
        break;
    case eEventCollision_Visible_Off:
        xEntHide(ent);
        ent->enabled = 0;
        break;
    case eEventReset:
        zEntHangable_Reset(ent);
        break;
    case eEventSwingerFollow:
        zEnt* follow = (zEnt*)from;
        zEntHangable_SetFollow(ent, follow);
        break;
    case eEventSetUpdateDistance:
        if (globals.updateMgr) {
            if (toParam[0] <= 0.0f) {
                xUpdateCull_SetCB(globals.updateMgr, ent, xUpdateCull_AlwaysTrueCB, NULL);
            } else {
                FloatAndVoid dist;
                dist.f = xsqr(toParam[0]);
                xUpdateCull_SetCB(globals.updateMgr, ent, xUpdateCull_DistanceSquaredCB, dist.v);
            }
        }
        break;
    case eEventSetLightKit:
        xModelSetLightKit(ent->model, (xLightKit*)toParamWidget);
        break;
    }

    if (!oldvisible && xEntIsVisible(ent)) {
        ent->vel.x = 0.0f;
        ent->vel.y = 0.0f;
        ent->vel.z = 0.0f;
        ent->endpos.x = ent->pivot.x;
        ent->endpos.y = ent->pivot.y - ent->hangInfo->leverArm;
        ent->endpos.z = ent->pivot.z;
    }
}

void zEntHangable_Save(zEntHangable* ent, xSerial* s)
{
    zEntSave(ent, s);
}

void zEntHangable_Load(zEntHangable* ent, xSerial* s)
{
    zEntLoad(ent, s);
}

void zEntHangable_Reset(zEntHangable* ent)
{
    zEntHangable_SetFollow(ent, NULL);
    xEntReset(ent);
    HangableSetup(ent, ent->asset);
}

void zEntHangable_FollowUpdate(zEntHangable* ent)
{
    if (!ent->follow) {
        return;
    }
}

void zEntHangable_SetFollow(zEntHangable* ent, zEnt* follow)
{
    if (follow) {
        ent->follow = follow;
        zEntHangable_FollowUpdate(ent);
    } else if (ent->follow) {
        ent->follow = NULL;
    }
}
