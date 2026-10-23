#ifndef XHUDFONTMETER_H
#define XHUDFONTMETER_H

#include "xHudMeter.h"
#include "xFont.h"

namespace xhud {

struct font_meter_asset;

class font_meter_widget : public meter_widget
{
public:
    struct widget_chunk;

    font_context font;
    font_context start_font;
    S32 precision;

    static void load(xBase& data, xDynAsset& asset, size_t asset_size);

    font_meter_widget(const font_meter_asset& a);

    void destruct();

    virtual void destroy();
    virtual U32 type() const;
    virtual bool is(U32 id) const;
    virtual void update(F32 dt);
    virtual void render();

#if DEBUG || RELEASE
    virtual const char* category() { return "Font Meter"; }
#endif

protected:
    char buffer[12];
    S32 last_value;
    xfont xf;
};

struct font_meter_widget::widget_chunk : xBase
{
    font_meter_widget w;
};

}

#endif
