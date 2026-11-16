#include "xCommon.h"
#include "zNPCGroup.h"

namespace zNPC {

void group::EventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                    U32 toParamWidgetID)
{
    ((group*)to)->handle_event(from, to, toEvent, toParam, toParamWidget, toParamWidgetID);
}

void group::init(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xBaseInit(&data, &asset);

    group* new_group = (group*)&data;
    new_group->asset = (const group_asset*)&asset;
    new_group->attacking_count = 0;
    new_group->eventFunc = EventCB;
}

void group::handle_event(xBase*, xBase*, U32, const F32*, xBase*, U32)
{
}

}
