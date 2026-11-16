#ifndef ZNPCGROUP_H
#define ZNPCGROUP_H

#include "xBase.h"
#include "xDynAsset.h"

namespace zNPC {

struct group_asset : xDynAsset
{
    S32 max_attackers;
};

class group : public xBase
{
public:
    static void init(xBase& data, xDynAsset& asset, size_t asset_size);

    bool can_attack()
    {
        return attacking_count < asset->max_attackers;
    }

    inline void attack();
    inline void finished_attack();

protected:
    const group_asset* asset;
    S32 attacking_count;

    static void EventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                        U32 toParamWidgetID);
    void handle_event(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* toParamWidget,
                      U32 toParamWidgetID);
};

}

#endif
