#ifndef XHUDUNITMETER_H
#define XHUDUNITMETER_H

#include "xHudMeter.h"
#include "xHudAsset.h"

namespace xhud {

class unit_meter_widget : public meter_widget
{
public:
    struct widget_chunk;

    unit_meter_asset res;
    xModelInstance* model[6][2];
    F32 anim_time;

    static void load(xBase& data, xDynAsset& asset, size_t asset_size);

    unit_meter_widget(const unit_meter_asset& a);

    void destruct();

    virtual void destroy();
    virtual U32 type() const;
    virtual bool is(U32 id) const;
    virtual void setup();
    virtual void update(F32 dt);
    virtual void render();

#if DEBUG || RELEASE
    virtual const char* category() { return "Unit Meter"; }
#endif
};

struct unit_meter_widget::widget_chunk : xBase
{
    unit_meter_widget w;
};

}

#endif
