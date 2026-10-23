#include "xHudFontMeter.h"

#include "xHudAsset.h"

#include <stdio.h>

namespace xhud {
    
void font_meter_widget::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate(14, asset, font_meter_asset, font_meter_asset::type_name());

    init_base(data, asset, sizeof(widget_chunk));

    widget_chunk& wc = (widget_chunk&)data;
    new (&wc.w) font_meter_widget((font_meter_asset&)asset);
}

font_meter_widget::font_meter_widget(const font_meter_asset& a)
    : meter_widget(a)
    , font(a.font)
    , start_font(a.font)
{
    last_value = (S32)value - 20;
    xf.id = 0;
    xf.width = font.w;
    xf.height = font.h;
    xf.space = font.space;
    xf.color = font.c;
    xf.clip = screen_bounds;
}

void font_meter_widget::destruct()
{
    meter_widget::destruct();
}

void font_meter_widget::destroy()
{
    destruct();
}

U32 font_meter_widget::type() const
{
    static U32 myid = xStrHash(font_meter_asset::type_name());
    return myid;
}

bool font_meter_widget::is(U32 id) const
{
    return id == font_meter_widget::type() || meter_widget::is(id);
}

void font_meter_widget::update(F32 dt)
{
    updater(dt);

    xf.id = font.id;
    xf.space = font.space;
    xf.width = font.w = rc.size.x;
    xf.height = font.h = rc.size.y;

    F32 a = 0.5f + rc.a * start_font.c.a;
    font.c.a = ((a <= 0) ? 0 : ((a >= 255) ? 255 : (U8)a));

    a = 0.5f + rc.a * start_font.drop_c.a;
    font.drop_c.a = ((a <= 0) ? 0 : ((a >= 255) ? 255 : (U8)a));

    S32 new_value = 0.5f + value;
    if (last_value != new_value) {
        last_value = new_value;
        sprintf(buffer, "%d", new_value);
    }
}

void font_meter_widget::render()
{
    if (font.drop_c.a > 0) {
        xf.color = font.drop_c;
        xf.render(buffer, rc.loc.x + font.drop_x, rc.loc.y + font.drop_y);
    }
    if (font.c.a > 0) {
        xf.color = font.c;
        xf.shadowColor = g_BLACK;
        xf.shadowOffsetX = 2.0f;
        xf.shadowOffsetY = 2.0f;
        xf.render(buffer, rc.loc.x, rc.loc.y);
    }
}

}
