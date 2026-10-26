#ifndef XHUDMETER_H
#define XHUDMETER_H

#include "xHud.h"
#include "xSndMgr.h"

namespace xhud {

struct meter_asset;

class meter_widget : public widget
{
public:
    const meter_asset* res;
    F32 value;
    F32 min_value;
    F32 max_value;
    F32 inc_value;
    F32 end_value;
    F32 inc_accel;
    F32 ping_delay;

    meter_widget(const meter_asset& a);

    // RESTORED: from Rat MAP
    ~meter_widget() {}
    // END RESTORED

    void destruct();
    
    virtual U32 type() const;
    virtual bool is(U32 id) const;

    virtual void update(F32 dt)
    {
        updater(dt);
    }

    void updater(F32 dt);

protected:
    sound_queue<4> pings;
};

}

#endif
