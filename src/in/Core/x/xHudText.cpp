#include "xHudText.h"

#include "xHudAsset.h"
#include "zTextBox.h"
#include "zScene.h"
#include "xTextAsset.h"
#include "zMain.h"

#include "decomp.h"

#include <string.h>

namespace xhud {
    
void text_widget::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate_L(15, asset, text_asset, text_asset::type_name());

    init_base(data, asset, sizeof(widget_chunk));
    
    widget_chunk& wc = (widget_chunk&)data;
    new (&wc.w) text_widget((text_asset&)asset);
}

text_widget::text_widget(const text_asset& a)
    : widget(a)
{
    text[0] = '\0';
}

void text_widget::destruct() { widget::destruct(); }

void text_widget::setup()
{
    presetup();

    const text_asset& ta = *(text_asset*)a;
    ztextbox* ztb = (ztextbox*)zSceneFindObject(ta.text_box);

    xASSERT_FMT_L(35, ztb != 0, "HUD Text %s: Cannot find text-box %s", xSTAssetNameOrID((void*)a), xSTAssetNameOrID(ta.text_box));

    if (ztb == 0) {
        tb = xtextbox::create();
        return;
    }
    
    ztb->refresh();
    tb = ztb->tb;
    ztb->get_text(text, sizeof(text));

    if (ta.text != 0) {
        U32 len;
        const char* s = xTextFindString(ta.text, &len);

        xASSERT_FMT_L(50, s != 0, "HUD Text %s: Cannot find text %s", xSTAssetNameOrID((void*)a), xSTAssetNameOrID(ta.text));

        if (s) {
            memcpy(text, s, len);
            text[len] = '\0';
        }
    }

    rc.r = (U8)(tb.font.color.r / 255.0f);
    rc.g = (U8)(tb.font.color.g / 255.0f);
    rc.b = (U8)(tb.font.color.b / 255.0f);
    rc.a = (U8)(tb.font.color.a / 255.0f);

    start_rc.r = rc.r;
    start_rc.g = rc.g;
    start_rc.b = rc.b;
    start_rc.a = rc.a;
}

void text_widget::destroy()
{
    destruct();
}

U32 text_widget::type() const
{
    static U32 myid = xStrHash(text_asset::type_name());
    return myid;
}

bool text_widget::is(U32 id) const
{
    return id == text_widget::type() || widget::is(id);
}

DECOMP_FORCEFLOAT(0.0f)

void text_widget::update(F32 dt)
{
    updater(dt);

    tb.set_text(text);

    tb.bounds.x = rc.loc.x;
    tb.bounds.y = rc.loc.y;
    tb.bounds.w = rc.size.x;
    tb.bounds.h = rc.size.y;

    if (rc.size.x < 1.0f) {
        tb.bounds.w = 640.0f * rc.size.x / FB_XRES;
    }

    if (rc.size.y < 1.0f) {
        tb.bounds.h = 480.0f * rc.size.y / FB_YRES;
    }

    tb.font.clip = tb.bounds;
}

void text_widget::render()
{
    xASSERT_L(125, tb.font.width);
    xASSERT_L(126, tb.font.height);
    tb.render(true);
}

void text_widget::set_text(const char* intext)
{
    xASSERT_FMT_L(133, (intext), "String is not set !");

    size_t textchars = strlen(intext);
    size_t copychars = xmin(textchars, sizeof(text) - 1);

    memcpy(text, intext, copychars);
    text[copychars] = '\0';
}

bool scale_text_motive_reset(widget& w, motive& m)
{
    text_widget& textWid = (text_widget&)w;

    if (m.orig_height != 0.0f) {
        textWid.tb.font.height = m.orig_height;
        textWid.tb.font.width = m.orig_width;
    }

    xASSERT(textWid.tb.font.width);
    xASSERT(textWid.tb.font.height);

    textWid.tb.font.color.a = 0xFF;
    textWid.tb.font.shadowColor.a = 0xFF;
    w.disable();
    return false;
}

bool scale_text_motive_update(widget& w, motive& m, F32 dt)
{
    text_widget& textWid = (text_widget&)w;

    if (m.orig_height == 0.0f) {
        m.orig_height = textWid.tb.font.height;
        m.orig_width = textWid.tb.font.width;
        m.current_scale = m.begin_scale;
        xASSERT(m.orig_height);
        xASSERT(m.orig_width);
    }

    F32 diff = dt * (m.final_scale - m.begin_scale) / m.delta;
    bool result;

    xASSERT(m.current_scale != 0);

    F32 remaining = m.final_scale - m.current_scale;

    if ((diff >= 0.0f && diff >= remaining) || (diff < 0.0f && diff <= remaining)) {
        m.current_scale = m.final_scale;
        result = false;
    } else {
        m.current_scale += diff;
        result = true;
    }

    textWid.tb.font.width = m.orig_width * m.current_scale;
    textWid.tb.font.height = m.orig_height * m.current_scale;

    xASSERT(textWid.tb.font.width);
    xASSERT(textWid.tb.font.height);

    F32 fPot = (m.current_scale - m.begin_scale) / (m.final_scale - m.begin_scale);
    F32 fAlpha = fPot * m.end_alpha + (1.0f - fPot) * m.begin_alpha;
    textWid.tb.font.color.a = (U8)(0.5f + fAlpha);
    textWid.tb.font.shadowColor.a = textWid.tb.font.color.a;

    if (!result) {
        scale_text_motive_reset(w, m);
        return false;
    }

    return true;
}

bool blink_text_motive_reset(widget& w, motive& m)
{
    text_widget& textWid = (text_widget&)w;

    textWid.tb.font.color = m.off_blink_color;
    textWid.tb.font.shadowColor.a = textWid.tb.font.color.a;
    return false;
}

bool blink_text_motive_update(widget& w, motive& m, F32 dt)
{
    xColor curr_color;

    if (m.current_blink_time > m.time_to_blink && !m.hold) {
        if (m.fp_finish) {
            m.fp_finish(w, m);
        }
        return false;
    }

    F32 percent_done = m.current_blink_time / m.time_to_blink;
    percent_done = xclamp(percent_done, 0.0f, 1.0f);
    F32 curr_blink_period = percent_done * m.end_blink_period + (1.0f - percent_done) * m.start_blink_period;
    F32 on_time = 0.5f * curr_blink_period;

    if (m.period_blink_time > on_time + m.fade_blink_time) {
        curr_color = m.off_blink_color;
    } else if (m.period_blink_time > on_time) {
        F32 fade_time = m.fade_blink_time;
        F32 fade_percent = (m.period_blink_time - on_time) / fade_time;
        curr_color.a = (U8)(0.5f + (fade_percent * m.off_blink_color.a + (1.0f - fade_percent) * m.on_blink_color.a));
        curr_color.r = (U8)(0.5f + (fade_percent * m.off_blink_color.r + (1.0f - fade_percent) * m.on_blink_color.r));
        curr_color.g = (U8)(0.5f + (fade_percent * m.off_blink_color.g + (1.0f - fade_percent) * m.on_blink_color.g));
        curr_color.b = (U8)(0.5f + (fade_percent * m.off_blink_color.b + (1.0f - fade_percent) * m.on_blink_color.b));
    } else {
        curr_color = m.on_blink_color;
    }

    m.current_blink_time += dt;
    m.period_blink_time += dt;

    if (m.period_blink_time >= curr_blink_period) {
        m.period_blink_time = 0.0f;
    }

    text_widget& textWid = (text_widget&)w;
    textWid.tb.font.color = curr_color;
    textWid.tb.font.shadowColor.a = curr_color.a;
    return true;
}

}
