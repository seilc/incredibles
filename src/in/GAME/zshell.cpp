#include "xCommon.h"
#include "zshell.h"

#include "xCollideSweptSphere.h"
#include "xGlobals.h"
#include "xMemMgr.h"
#include "xModel.h"
#include "xModelBucket.h"
#include "xString.h"
#include "xstransvc.h"
#include "zBomb.h"
#include "zEnt.h"
#include "zMain.h"
#include "zShrapnel.h"

#include <string.h>

namespace z
{
    RpAtomic* find_model(const char* model_name, U32& modelID)
    {
        modelID = xStrHashCat(xStrHash(model_name), ".minf");
        if (xSTFindAsset(modelID, NULL)) {
            return NULL;
        }

        modelID = xStrHashCat(xStrHash(model_name), ".dff");
        RpAtomic* model = (RpAtomic*)xSTFindAsset(modelID, NULL);
        if (!model) {
            modelID = xStrHash(model_name);
            model = (RpAtomic*)xSTFindAsset(modelID, NULL);
        }

        return model;
    }

    xModelInstance* get_model(const char* model_name)
    {
        U32 modelID;
        RpAtomic* model = find_model(model_name, modelID);
        if (model) {
            return xModelInstanceAlloc(model, NULL, 0, 0, NULL);
        }

        return zEntRecurseModelInfo(xSTFindAsset(modelID, NULL), NULL);
    }

    void free_model(xModelInstance* model)
    {
        if (model) {
            xModelInstanceFree(model);
        }
    }

    shell_type* shell_type::create(const char* model_name)
    {
        shell_type* type = (shell_type*)xMEMALLOC(sizeof(shell_type), 0);
        type->shell_model_name = model_name;

        U32 modelID;
        find_model(type->shell_model_name, modelID);

        U16 parameters_size;
        xModelAssetParam* parameters = zEntGetModelParams(modelID, &parameters_size);

        const char* shrapnel_name = zParamGetString(parameters, parameters_size, "Shrapnel", NULL);
        if (shrapnel_name) {
            type->shrapnel = (zShrapnelAsset*)xSTFindAsset(xStrHash(shrapnel_name), NULL);
        } else {
            type->shrapnel = NULL;
        }

        type->away_knockback = zParamGetFloat(parameters, parameters_size, "KnockbackAwaySpeed", 0.0f);
        type->up_knockback = zParamGetFloat(parameters, parameters_size, "KnockbackUpSpeed", 0.0f);
        type->damage = zParamGetInt(parameters, parameters_size, "Damage", 10);
        type->max_dist = zParamGetFloat(parameters, parameters_size, "MaxDist", 50.0f);
        type->blast_radius = zParamGetFloat(parameters, parameters_size, "BlastRadius", 3.0f);
        zParamGetVector(parameters, parameters_size, "GravityDirection", -xVec3::m_Null, &type->gravity_direction);

        return type;
    }

    void shell::setup()
    {
    }

    void shell::destroy()
    {
        free_model(model);
    }

    void shell::update(F32 dt)
    {
        xSweptSphere sws;
        memset(&sws, 0, sizeof(sws));

        velocity -= type->gravity_direction * gravity_speed * dt;

        xVec3 end = position + velocity * dt;
        xSweptSpherePrepare(&sws, &position, &end, 0.5f);

        S32 rc = xSweptSphereToModel(&sws, ZPLAYER->model->Data, ZPLAYER->model->Mat) |
                 (xSweptSphereToStatDyn(&sws, xglobals->sceneCur, NULL, k_XENT_COLLTYPE_NPC, 0) |
                  xSweptSphereToEnv(&sws, xglobals->sceneCur->env));
        if (rc) {
            xSweptSphereGetResults(&sws);

            if (!xfeq0(type->blast_radius)) {
                sphere_damage(owner, sws.worldContact, type->blast_radius, type->damage, type->away_knockback, type->up_knockback);
            }

            if (type->shrapnel) {
                xMat4x3* frame = xModelGetFrame(model);
                set_mat(frame);
                type->shrapnel->initCB(type->shrapnel, model, NULL, NULL);
            }

            set_used(false);
        }

        distance += velocity.length() * dt;
        if (distance > type->max_dist) {
            set_used(false);
        }

        position += velocity * dt;
    }

    void shell::set_mat(xMat4x3* mat) const
    {
        mat->pos = position;
        mat->at = velocity;
        mat->at.normalize();

        xVec3 up = { 0.0f, 1.0f, 0.0f };
        mat->up = up - mat->at * mat->at.dot(up);
        mat->up.normalize();
        mat->right = mat->up.cross(mat->at);
    }

    void shell::render() const
    {
        if (model) {
            xMat4x3* frame = xModelGetFrame(model);
            set_mat(frame);
            xModelBucket_Add(model);
        }
    }

    shell::shell(const xVec3& position, const xVec3& velocity, xEnt* owner, F32 gravity_speed, shell_type* type)
        : lightweight<shell, 15>(true), owner(owner), distance(0.0f), gravity_speed(gravity_speed), position(position),
          velocity(velocity), type(type)
    {
        model = get_model(type->shell_model_name);
    }
} // namespace z
