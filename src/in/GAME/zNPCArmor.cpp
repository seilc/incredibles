#include "xCommon.h"
#include "zNPCArmor.h"

#include "zNPCTypes.h"
#include "zShrapnel.h"
#include "zGlobals.h"
#include "xString.h"
#include "xstransvc.h"
#include "xRand.h"
#include "xEvent.h"

#include "decomp.h"

#include <new>
#include <stdio.h>
#include <string.h>

namespace zNPC
{
    xVec3 tankArmorTranslation[] = {
        { -0.01f, 2.616f, -1.039f },  { 0.029f, -2.921f, 0.55f },   { -0.009f, 2.848f, -0.037f },
        { 0.062f, -0.24f, -0.006f },  { 0.018f, 0.229f, 2.896f },   { -0.015f, 1.202f, 2.569f },
        { 0.01f, 2.594f, 0.97f },     { -0.05f, 0.0f, 0.0f },       { -0.025f, -1.752f, 2.232f },
        { 0.018f, 0.156f, -2.893f },  { -0.027f, -2.499f, 1.491f }, { -0.356f, 0.092f, 2.369f },
        { -0.028f, -2.466f, -1.454f }, { -0.015f, 1.145f, -2.54f }, { 0.022f, -0.866f, -2.78f },
        { 0.028f, -2.466f, -1.454f }, { -0.023f, -0.214f, -2.532f },
    };

    S32 Armor::numArmorNPC;

    void Armor::AddArmor(common* owner)
    {
        S32 armorBone;
        owner->get_parameter("ArmorBone1", &armorBone, -1);
        if (armorBone == -1) {
            return;
        }

        const char* shrapnelName;
        owner->get_parameter("ArmorShrapnel", &shrapnelName, NULL);

        zShrapnelAsset* shrapnelAsset = NULL;
        if (shrapnelName) {
            shrapnelAsset = (zShrapnelAsset*)xSTFindAsset(xStrHash(shrapnelName), NULL);
        }

        Armor* armor = new (xMEMALLOC(sizeof(Armor), 0)) Armor;
        owner->add_behavior(armor, 126);

        const char* modelName;
        owner->get_parameter("ArmorModel1", &modelName, NULL);

        armor->breakingArmorSound = xSndMgrGetSoundGroup(xStrHash("BreakingArmorGroup"));

        char modelBaseName[100], parameterName[100];
        S32 bone = 0;
        while (true) {
            armor->armorInstance[bone].bone = armorBone;

            strcpy(modelBaseName, modelName);
            strtok(modelBaseName, ".");
            RpAtomic* model = (RpAtomic*)xSTFindAsset(xStrHash(modelBaseName), NULL);
            armor->armorInstance[bone].modelInstance = xModelInstanceAlloc(model, NULL, 0, 0, NULL);

            if (globals.sceneCur->zen->easset->objectLightKit) {
                xLightKit* objLightKit =
                    (xLightKit*)xSTFindAsset(globals.sceneCur->zen->easset->objectLightKit, NULL);
                xModelSetLightKit(armor->armorInstance[bone].modelInstance, objLightKit);
            }

            armor->armorInstance[bone].shrapnelAsset = shrapnelAsset;
            armor->armorInstance[bone].isActive = true;
            bone++;

            sprintf(parameterName, "ArmorBone%d", bone + 1);
            owner->get_parameter(parameterName, &armorBone, -1);
            if (armorBone == -1) {
                break;
            }

            sprintf(parameterName, "ArmorModel%d", bone + 1);
            owner->get_parameter(parameterName, &modelName, NULL);
        }

        armor->numArmor = bone;

        owner->get_parameter("ArmorHitPoints", &armor->armorHitPointsInitial, -1);
        if (armor->armorHitPointsInitial == -1) {
            armor->armorHitPointsInitial = 40;
        }

        armor->reset();

        if (++numArmorNPC == 1) {
            xAnimTable* table = (xAnimTable*)xSTFindAsset(xStrHash("hench_NI_club_bind"), NULL);
            armor->_add_states(table);
            armor->_add_transitions(table);
        }
    }

    void Armor::add_states(xAnimTable*)
    {
    }

    void Armor::_add_states(xAnimTable*)
    {
    }

    void Armor::add_transitions(xAnimTable*)
    {
    }

    void Armor::_add_transitions(xAnimTable*)
    {
    }

    DECOMP_FORCEACTIVE("", "ERROR")

    void Armor::setup()
    {
    }

    void Armor::init()
    {
        armorHitPoints = armorHitPointsInitial;
        numArmorRemaining = numArmor;
    }

    void Armor::reset()
    {
        armorHitPoints = armorHitPointsInitial;
        numArmorRemaining = numArmor;

        for (S32 i = 0; i < numArmor; i++) {
            armorInstance[i].isActive = true;
        }

        finishCount = 0;
        isExclusive = false;
        isEnabled = true;
        isFinished = false;
        startFallBack = false;
    }

    bool Armor::system_event(xBase*, xBase*, U32 toEvent, const F32*, xBase*, U32)
    {
        switch (toEvent) {
        case eEventOn:
            isEnabled = true;
            break;
        case eEventOff:
            isEnabled = false;
            break;
        }

        return false;
    }

    bool Armor::damage(zCombatDamageInfo& damageInfo)
    {
        if (!isEnabled) {
            return false;
        }

        if (armorHitPoints == 0) {
            return false;
        }

        if (armorHitPoints <= damageInfo.damage) {
            damageInfo.damage = damageInfo.damage - armorHitPoints + 1;
            armorHitPoints = 0;
            isEnabled = false;
        } else {
            armorHitPoints -= damageInfo.damage;
            damageInfo.damage = 0;
            damageInfo.knockback /= 2.0f;
        }

        xSndMgrPlay(breakingArmorSound, 0, &((xMat4x3*)owner->model->Mat)->pos, NULL, NULL, NULL, NULL);

        S32 numNow = numArmor * ((F32)armorHitPoints / armorHitPointsInitial) + 0.999999f;
        S32 numToRemove = numArmorRemaining - numNow;
        numArmorRemaining = numNow;

        S32 i, iStart = xurand() * numArmor;
        i = iStart;
        do {
            if (numToRemove == 0) {
                break;
            }

            if (armorInstance[i].isActive) {
                zShrapnelAsset* shrapnelAsset = armorInstance[i].shrapnelAsset;
                if (shrapnelAsset && shrapnelAsset->initCB) {
                    ((zFragProjectileAsset*)(shrapnelAsset + 1))->modelFile =
                        armorInstance[i].modelInstance->Data;
                    shrapnelAsset->initCB(shrapnelAsset, armorInstance[i].modelInstance, NULL, NULL);
                }

                armorInstance[i].isActive = false;
                numToRemove--;
            }

            i++;
            if (i == numArmor) {
                i = 0;
            }
        } while (i != iStart);

        return false;
    }

    void Armor::update(F32)
    {
        if (startFallBack) {
            set_state("AR_T_FallBack01", 0.15f, NULL);
            isExclusive = true;
            startFallBack = false;
        }

        if (isFinished) {
            isEnabled = false;
        }
    }

    void Armor::render()
    {
        if (armorHitPoints == 0) {
            return;
        }

        xMat4x3 boneMat, offsetMat = g_I3;

        for (S32 i = 0; i < numArmor; i++) {
            if (armorInstance[i].modelInstance && armorInstance[i].isActive) {
                S32 bone = armorInstance[i].bone;
                if (owner->get_type() == NPC_TYPE_TANK) {
                    offsetMat.pos.x = tankArmorTranslation[i].x;
                    offsetMat.pos.y = tankArmorTranslation[i].y;
                    offsetMat.pos.z = tankArmorTranslation[i].z;

                    xMat4x3Mul(&boneMat, &offsetMat, (xMat4x3*)&owner->model->Mat[bone + 1]);
                    xMat4x3Mul(&armorInstance[i].mat, &boneMat, (xMat4x3*)owner->model->Mat);
                } else {
                    xMat4x3Mul(&armorInstance[i].mat, (xMat4x3*)&owner->model->Mat[bone + 1],
                               (xMat4x3*)owner->model->Mat);
                }

                *(xMat4x3*)armorInstance[i].modelInstance->Mat = armorInstance[i].mat;
                xModelEval(armorInstance[i].modelInstance);
                xModelRender(armorInstance[i].modelInstance);
            }
        }
    }

    void Armor::render_extra()
    {
    }

    bool Armor::runnable(F32)
    {
        return isEnabled;
    }
} // namespace zNPC
