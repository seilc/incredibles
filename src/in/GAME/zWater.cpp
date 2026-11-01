#include "zWater.h"

#include "zBase.h"
#include "xEvent.h"

struct xIniValue;

namespace {

struct motion_config;

void stay_hidden_dammit(xEnt* ent);
void load_motion_config(motion_config& cfg, const xIniValue* values, S32 values_size);
void load_motion_configs();

}

void zWaterBody::load(xBase& data, xDynAsset& asset, size_t)
{
    ((zWaterBody&)data).load((asset_type&)asset);
}

void zWaterBody::show()
{
    visible = true;
}

void zWaterBody::hide()
{
    visible = false;
}

void zWaterBody::load(const asset_type& a)
{
    asset = &a;

    xBaseInit(this, asset);

    baseType = eBaseTypeWaterBody;
    eventFunc = cb_dispatch;

    if (linkCount)
    {
        link = (xLinkAsset*)(asset + 1);
    }

    body_id = -1;
    visible = (asset->flags & ZWATERBODY_VISIBLE);
}

U32 zWaterBody::get_bodyID()
{
    return asset->body;
}

void zWaterBody::cb_dispatch(xBase*, xBase* to, U32 event, const F32*, xBase*, U32)
{
    zWaterBody& e = *(zWaterBody*)to;

    switch (event)
    {
    case eEventVisible:
        e.show();
        break;
    case eEventInvisible:
        e.hide();
        break;
    }
}