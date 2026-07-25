#ifndef ZCAMPRESET_H
#define ZCAMPRESET_H

#include "xDynAsset.h"
#include "xMath3.h"

struct zCamPreset : xBase
{
    struct asset_type : xDynAsset
    {
        U32 flags;
        U32 checkpoint;
        xVec3 loc;

        struct
        {
            F32 yaw;
            F32 pitch;
            F32 roll;
        } dir;
    };

    const asset_type* asset;
    S32 flags;

    static void load(xBase& data, xDynAsset& asset, ulong32);
    static void update_all(F32);
    static void apply_on_scene_prepare();
    static void apply_on_reset();
    static zCamPreset* find_preset(bool scene_prepare);
    static void event_handler_callback(xBase* from, xBase* to, U32 toEvent,
                                       const F32* toParam, xBase* toParamWidget,
                                       U32 toParamWidgetID);

    void load(const asset_type& a);
    bool get_checkpoint(xVec3& loc, F32& yaw) const;
    bool at_checkpoint() const;
    F32 distance2(const xVec3& loc) const;
    void apply();
    bool get_relative(xVec3& loc, F32& yaw) const;
    void get_matrix(xMat4x3& mat) const;
    void event_handler(xBase* from, U32 toEvent, const F32*, xBase*, U32);

    void debug_init()
    {
    }
};

#endif