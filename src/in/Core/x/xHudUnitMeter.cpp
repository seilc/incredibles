#include "xHudUnitMeter.h"

#include "xDebugTweak.h"
#include "xFont.h"
#include "xGlobals.h"
#include "xMath.h"
#include "zAutoTweak.h"

#include <stdio.h>

namespace xhud {

namespace {

F32 tweak_anim_time_delta = 0.1f;

}

void unit_meter_widget::load(xBase& data, xDynAsset& asset, size_t asset_size)
{
    xDynAssetValidate(20, asset, unit_meter_asset, unit_meter_asset::type_name());
    init_base(data, asset, sizeof(widget_chunk));
    widget_chunk& wc = (widget_chunk&)data;
    new (&wc.w) unit_meter_widget((unit_meter_asset&)asset);
}

unit_meter_widget::unit_meter_widget(const unit_meter_asset& a)
    : meter_widget(a)
    , res(a)
    , anim_time(0.0f)
{
    for (S32 i = 0; i < 2; i++) {
        for (S32 j = 0; j < 6; j++) {
            res.model[i].loc.x = NSCREENX(a.model[i].loc.x);
            res.model[i].loc.y = NSCREENY(a.model[i].loc.y);

            res.model[i].size.x = NSCREENX(a.model[i].size.x);
            res.model[i].size.y = NSCREENY(a.model[i].size.y);

            model[j][i] = load_model(res.model[i].id);
        }
    }

    static bool registered = false;
    if (!registered) {
        registered = true;
        xTWEAK("Temp|HUD Unit Anim Delta", &tweak_anim_time_delta, 0.0f, 10.0f, NULL, NULL, 0);
    }

    res.offset.x = NSCREENX(a.offset.x);
    res.offset.y = NSCREENY(a.offset.y);
}

void unit_meter_widget::destruct()
{
    meter_widget::destruct();
}

void unit_meter_widget::destroy()
{
    destruct();
}

U32 unit_meter_widget::type() const
{
    static U32 myid = xStrHash(unit_meter_asset::type_name());
    return myid;
}

bool unit_meter_widget::is(U32 id) const
{
    return id == unit_meter_widget::type() || meter_widget::is(id);
}

#if DEBUG || RELEASE
static const tweak_callback tweakOffsetX = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, normalizedToScreenX, screenToNormalizedX
};
static const tweak_callback tweakOffsetY = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, normalizedToScreenY, screenToNormalizedY
};
#endif

void unit_meter_widget::setup()
{
    presetup();

#if DEBUG || RELEASE
    char name[128];
    sprintf(name, "HUD|%s|%s|", category(), xSTAssetName((void*)a));

    char* prefix = name;
    zAUTOTWEAK(prefix, "Offset \001X", &res.offset.x, 0.0f, (F32)FB_XRES, &tweakOffsetX, NULL, 2);
    zAUTOTWEAK(prefix, "Offset \001Y", &res.offset.y, 0.0f, (F32)FB_YRES, &tweakOffsetY, NULL, 2);
#endif

    char tempString[128];
    S32 i;
    char* modelName[2] = { "ModelEmpty", "ModelFull" };
    for (i = 0; i < 2; i++) {
        sprintf(tempString, "%s|Location \001X", modelName[i]);
        zAUTOTWEAK(prefix, tempString, &res.model[i].loc.x, 0.0f, (F32)FB_XRES, &tweakOffsetX, NULL, 2);

        sprintf(tempString, "%s|Location \001Y", modelName[i]);
        zAUTOTWEAK(prefix, tempString, &res.model[i].loc.y, 0.0f, (F32)FB_YRES, &tweakOffsetY, NULL, 2);

        sprintf(tempString, "%s|Location \001Z", modelName[i]);
        zAUTOTWEAK(prefix, tempString, &res.model[i].loc.z, -100.0f, 100.0f, NULL, NULL, 2);

        sprintf(tempString, "%s|Size \001Width", modelName[i]);
        zAUTOTWEAK(prefix, tempString, &res.model[i].size.x, 0.0f, (F32)FB_XRES, &tweakOffsetX, NULL, 2);

        sprintf(tempString, "%s|Size \001Height", modelName[i]);
        zAUTOTWEAK(prefix, tempString, &res.model[i].size.y, 0.0f, (F32)FB_YRES, &tweakOffsetY, NULL, 2);
    }
}

void unit_meter_widget::update(F32 dt)
{
    meter_widget::updater(dt);
    if (!visible() || rc.a <= 0.5f/255.0f) return;
    anim_time += dt;
    S32 units = 0.5f + max_value;
    if (units > 6) units = 6;
    xASSERT(124, units >= 0);

    F32 mod_duration = 0.0f;
    for (S32 i = 0; i < units; i++) {
        S32 which = 0; if ((res.fill_forward && value >= i + 1) ||
            (!res.fill_forward && value >= units - i))
            which = 1;
        xModelInstance* m = model[i][which];
        if (!m || !m->Anim) continue;

        F32 duration = m->Anim->Single->State->Data->Duration;
        if (duration <= 0.0f) continue;
        F32 time = i * tweak_anim_time_delta + anim_time;
        mod_duration = duration;
        if (time > duration) time = xfmod(time, duration);
        m->Anim->Single->Time = time;

        xModelEval(m);
    }
}

void unit_meter_widget::render()
{
    render_context unitrc = rc;
    if (xfeq0(rc.a)) return;

    S32 units = 0.5f + max_value;
    if (units > 6) units = 6;
    xASSERT(157, units >= 0);
    for (S32 i = 0; i < units; i++) {
        S32 which = 0; if ((res.fill_forward && value >= i + 1) ||
            (!res.fill_forward && value >= units - i))
            which = 1;

        if (!model[i][which]) continue;

        unitrc.loc.x = rc.loc.x + res.model[which].loc.x + res.offset.x * i;
        unitrc.loc.y = rc.loc.y + res.model[which].loc.y + res.offset.y * i;
        unitrc.loc.z = rc.loc.z + res.model[which].loc.z + res.offset.z * i;

        unitrc.size.x = rc.size.x * res.model[which].size.x;
        unitrc.size.y = rc.size.y * res.model[which].size.y;
        unitrc.size.z = rc.size.z * res.model[which].size.z;

        render_model(*model[i][which], unitrc);
    }
}

}
