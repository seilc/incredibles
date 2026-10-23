#ifndef ZNPC_H
#define ZNPC_H

#include "xEnt.h"
#include "xFactory.h"
#include "xMovePoint.h"

struct xEntNPCAsset
{
    S32 npcFlags;
    S32 npcModel;
    S32 npcProps;
    U32 movepoint;
    U32 taskWidgetPrime;
    U32 taskWidgetSecond;
};

struct xEntNPCAssetIN : xEntNPCAsset
{
    U32 navigation_mesh_id;
    U32 settings;
};

struct zAnimFxSoundGroup
{
    U32 ID;
    U8 count;
};

namespace zNPC
{
    struct base : xEnt, xFactoryInst
    {
        S16 bound_bone;
        U16 sound_id_offset;
        U16 global_parameters_size;
        U16 local_parameters_size;
        U32 type;
        xModelAssetParam* global_parameters;
        xModelAssetParam* local_parameters;
        union
        {
            xMovePoint* movepoint;
            U32 movepoint_asset_id;
        };
        xEntNPCAssetIN* npc_asset;
        xModelAssetInfo* model_asset;
        F32 shadow_strength;
        F32 shadow_cache_fudge_factor;

        // clang-format off
        base() {}
        virtual ~base() {}
        virtual void init_npc(xEntAsset* asset) {}
        virtual void update_npc(F32 dt);
        virtual void update_bounds();
        virtual void setup() {}
        virtual void scene_setup() {}
        virtual void render_extra() {}
        virtual bool system_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                                  xBase* toParamWidget, U32 toParamWidgetID) { return true; }
        virtual iSndHandle play_sound(zAnimFxSoundGroup* group);
        virtual void destroy() {}
        virtual xAnimTable* CreateAnimTable(xAnimTable*) { return NULL; }
        virtual void change_bounds(F32 scale, xVec3 offset, bool use_bounding_box);
        // clang-format on

        xVec3 bound_offset;

        void initialize_bounds(F32 modelScale);
        xVec3 get_bone_world_direction(S32 bone, const xVec3& bone_local_offset);
        xVec3 get_bone_world_position(S32 bone);
        xVec3 get_bone_world_position_offset(S32 bone, const xVec3& bone_local_offset);
        bool get_parameter(const char* name, U8* value, U8 default_value);
        bool get_parameter(const char* name, S32* value, S32 default_value);
        bool get_parameter(const char* name, S16* value, S16 default_value);
        bool get_parameter(const char* name, bool* value, bool default_value);
        bool get_parameter(const char* name, F32* value, F32 default_value);
        bool get_parameter(const char* name, xVec3* value, const xVec3& default_value);
        bool get_parameter(const char* name, const char** value, const char* default_value);

        U32 get_type() const
        {
            return type;
        }
    };

    void destroy_npc(xFactoryInst* inst);
} // namespace zNPC

#endif