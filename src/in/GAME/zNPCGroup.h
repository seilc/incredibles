#ifndef ZNPCGROUP_H
#define ZNPCGROUP_H

#include "xDynAsset.h"

namespace zNPC
{
    struct group_asset : xDynAsset
    {
        S32 max_attackers;
    };

    struct group : xBase
    {
        group_asset* asset;
        S32 attacking_count;

        static void EventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                            xBase* toParamWidget, U32 toParamWidgetID);
        static void init(xBase& data, xDynAsset& asset, ulong32);

        void handle_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                          xBase* toParamWidget, U32 toParamWidgetID);

        bool can_attack()
        {
            return attacking_count < asset->max_attackers;
        }
    };
} // namespace zNPC

#endif