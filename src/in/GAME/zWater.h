#ifndef ZWATER_H
#define ZWATER_H

#include "xDynAsset.h"
#include "xEnt.h"

struct zWaterBody : xBase
{
    struct asset_type : xDynAsset
    {
        U32 flags;
        U32 motion_type;
        U32 body;
        U32 facade_refract;
        U32 facade_reflect;
        U32 light_dir;
    };

    S32 motion_type;
    const asset_type* asset;
    xEnt* body_ent;
    xEnt** refract_ents;
    S32 refract_ents_size;
    xEnt** reflect_ents;
    S32 reflect_ents_size;
    S32 body_id;
    bool visible;

    static void load(xBase& data, xDynAsset& asset, size_t);
    static void cb_dispatch(xBase*, xBase* to, U32 event, const F32*, xBase*, U32);

    void show();
    void hide();
    void load(const asset_type& a);
    U32 get_bodyID();
};

#define ZWATERBODY_VISIBLE 0x1

#endif