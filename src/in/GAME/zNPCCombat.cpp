#include "xCommon.h"
#include "zNPCCombat.h"

#include "xHierarchyBound.h"
#include "xString.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zAnimCache.h"
#include "zCombat.h"

struct zNPCCombatInitData
{
    xHierarchyBoundInitData* hierarchy;
    F32 masterRadius;
};

struct zNPCCombatInitDataIndex
{
    char* name;
    zNPCCombatInitData* initData;
    U32 id;
};

namespace
{
    xHierarchyBoundInitData zNPC_BadGuyMedium_Bounds[9] = {
        { 18, 0.55f, zHT_LEFT },
        { 9, 0.55f, zHT_RIGHT },
        { 5, 0.46f, zHT_FRONT },
        { 29, 0.4f, zHT_LEFT },
        { 30, 0.3f, zHT_LEFT },
        { 25, 0.4f, zHT_RIGHT },
        { 26, 0.3f, zHT_RIGHT },
        { 3, 0.48f, zHT_FRONT },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_Hench_Bounds[9] = {
        { 10, 0.55f, zHT_LEFT },
        { 5, 0.55f, zHT_RIGHT },
        { 13, 0.46f, zHT_FRONT },
        { 28, 0.4f, zHT_LEFT },
        { 29, 0.3f, zHT_LEFT },
        { 16, 0.4f, zHT_RIGHT },
        { 17, 0.3f, zHT_RIGHT },
        { 3, 0.48f, zHT_FRONT },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_BadGuyShield_Bounds[7] = {
        { 13, 0.36f, zHT_FRONT },
        { 20, 0.3f, zHT_LEFT },
        { 21, 0.25f, zHT_LEFT },
        { 16, 0.3f, zHT_RIGHT },
        { 17, 0.25f, zHT_RIGHT },
        { 3, 0.48f, zHT_FRONT },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_Turret_Top_Bounds[2] = {
        { 3, 2.0f, zHT_GENERAL },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_Turret_Bunker_Bounds[2] = {
        { 32, 2.0f, zHT_GENERAL },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_Turret_Building_Bounds[2] = {
        { 1, 2.0f, zHT_GENERAL },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_Turret_Hoverpod_Bounds[2] = {
        { 0, 2.0f, zHT_GENERAL },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_Turret_Tower_Bounds[2] = {
        { 0, 2.0f, zHT_GENERAL },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_Turret_Beach_Bounds[2] = {
        { 0, 2.0f, zHT_GENERAL },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_Velocipod_Bounds[2] = {
        { 1, 1.3f, zHT_GENERAL },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_Omnidroid08_Bounds[7] = {
        { 1, 5.0f, zHT_GENERAL },
        { 17, 1.0f, zHT_GENERAL },
        { 28, 1.0f, zHT_GENERAL },
        { 61, 1.0f, zHT_GENERAL },
        { 39, 1.0f, zHT_GENERAL },
        { 50, 1.0f, zHT_GENERAL },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_BombVoyage_Bounds[2] = {
        { 1, 1.0f, zHT_GENERAL },
        { -2, 0.0f, 0 },
    };

    xHierarchyBoundInitData zNPC_BombVoyageHeli_Bounds[2] = {
        { 1, 5.0f, zHT_GENERAL },
        { -2, 0.0f, 0 },
    };

    zNPCCombatInitData ZNPC_Default = { NULL, 0.0f };
    zNPCCombatInitData ZNPC_BadGuyMedium = { zNPC_BadGuyMedium_Bounds, 1.3f };
    zNPCCombatInitData ZNPC_BadGuyShield = { zNPC_BadGuyShield_Bounds, 1.3f };
    zNPCCombatInitData ZNPC_BadGuyFat = { NULL, 1.3f };
    zNPCCombatInitData ZNPC_Mine = { NULL, 1.0f };
    zNPCCombatInitData ZNPC_Oracle = { NULL, 1.0f };
    zNPCCombatInitData ZNPC_Turret_Top = { zNPC_Turret_Top_Bounds, 1.7f };
    zNPCCombatInitData ZNPC_Turret_Bunker = { zNPC_Turret_Bunker_Bounds, 2.0f };
    zNPCCombatInitData ZNPC_Turret_Building = { zNPC_Turret_Building_Bounds, 2.0f };
    zNPCCombatInitData ZNPC_Turret_Hoverpod = { zNPC_Turret_Hoverpod_Bounds, 2.0f };
    zNPCCombatInitData ZNPC_Turret_Tower = { zNPC_Turret_Tower_Bounds, 2.0f };
    zNPCCombatInitData ZNPC_Turret_Beach = { zNPC_Turret_Beach_Bounds, 2.0f };
    zNPCCombatInitData ZNPC_Car = { NULL, 1.0f };
    zNPCCombatInitData ZNPC_Lobber = { zNPC_Hench_Bounds, 1.3f };
    zNPCCombatInitData ZNPC_Velocipod = { zNPC_Velocipod_Bounds, 1.3f };
    zNPCCombatInitData ZNPC_Omnidroid08 = { zNPC_Omnidroid08_Bounds, 5.0f };
    zNPCCombatInitData ZNPC_BombVoyage = { zNPC_BombVoyage_Bounds, 1.0f };
    zNPCCombatInitData ZNPC_BombVoyageHeli = { zNPC_BombVoyageHeli_Bounds, 5.0f };

    zNPCCombatInitDataIndex initData[18] = {
        { "ZNPC_Default", &ZNPC_Default, 0 },
        { "ZNPC_BadGuyMedium", &ZNPC_BadGuyMedium, 0 },
        { "ZNPC_BadGuyShield", &ZNPC_BadGuyShield, 0 },
        { "ZNPC_BadGuyFat", &ZNPC_BadGuyFat, 0 },
        { "ZNPC_Mine", &ZNPC_Mine, 0 },
        { "ZNPC_Oracle", &ZNPC_Oracle, 0 },
        { "ZNPC_Turret_Top", &ZNPC_Turret_Top, 0 },
        { "ZNPC_Turret_Bunker", &ZNPC_Turret_Bunker, 0 },
        { "ZNPC_Turret_Building", &ZNPC_Turret_Building, 0 },
        { "ZNPC_Turret_Hoverpod", &ZNPC_Turret_Hoverpod, 0 },
        { "ZNPC_Turret_Tower", &ZNPC_Turret_Tower, 0 },
        { "ZNPC_Turret_Beach", &ZNPC_Turret_Beach, 0 },
        { "ZNPC_Car", &ZNPC_Car, 0 },
        { "ZNPC_Lobber", &ZNPC_Lobber, 0 },
        { "ZNPC_Velocipod", &ZNPC_Velocipod, 0 },
        { "ZNPC_Omnidroid08", &ZNPC_Omnidroid08, 0 },
        { "ZNPC_BombVoyage", &ZNPC_BombVoyage, 0 },
        { "ZNPC_BombVoyageHeli", &ZNPC_BombVoyageHeli, 0 },
    };
}

void zNPCCombatInit()
{
    for (S32 i = 0; i < ARRAY_LENGTH(initData); i++) {
        initData[i].id = xStrHash(initData[i].name);
    }

    zNPCCombatReset();
}

void zNPCCombatReset()
{
}

void zNPCCombatSetCombat(xEnt* ent, U32 id, const char* root, U32 atkTblID, U32, U16 hitPoints,
                         F32 scale)
{
    ent->user_data = xMEMALLOC(sizeof(zCombat), 0);
    zCombat* combat = zCombatGetFrom(ent);
    U32 size;
    zAttackTableState* pAttackTableStates = NULL;
    U32 stateCount = 0;

    if (atkTblID != 0) {
        zAttackTableAsset* pAsset = (zAttackTableAsset*)xSTFindAsset(atkTblID, &size);
        if (pAsset != NULL) {
            stateCount = pAsset->stateCount;
            pAttackTableStates = (zAttackTableState*)(
                (zAttackTableTransition*)(
                    (zAttackTableEntry*)((zAttackTableSection*)(pAsset + 1) + pAsset->sectionCount) +
                    pAsset->entryCount) +
                pAsset->transitionCount);
        }
    }

    if (pAttackTableStates != NULL) {
        zAnimCacheCreateData(pAttackTableStates, stateCount, ent->model->Anim);
    }

    for (U32 i = 0; i < ARRAY_LENGTH(initData); i++) {
        if (id == initData[i].id) {
            zNPCCombatInitData* init = initData[i].initData;
            combat->Init(init->hierarchy, init->masterRadius, pAttackTableStates, stateCount,
                         hitPoints, root, NULL, ent->model->BoneCount, scale);
            combat->envCollisionCB = NULL;
            return;
        }
    }
}
