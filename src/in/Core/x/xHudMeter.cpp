#include "xHudMeter.h"

#include "xHudAsset.h"
#include "xDebugTweak.h"
#include "xSndMgr.h"

#include "decomp.h"

namespace xhud {

namespace {

F32 tweak_accel = 50.0f;
F32 tweak_min_ping_time = 0.05f;
F32 tweak_pitch_scale = 0.01f;

}

DECOMP_FORCEFLOAT(1.0f)

meter_widget::meter_widget(const meter_asset& a)
    : widget(a)
    , res(&a)
    , value(a.start_value)
    , min_value(a.min_value)
    , max_value(a.max_value)
    , inc_value(0.0f)
    , end_value(a.start_value)
    , ping_delay(10.0f)
    , pings()
{
    static bool registered = false;
    if (!registered) {
        registered = true;
        xTWEAK("Temp|HUD Meter Accel", &tweak_accel, 0.0f, 1e9f, NULL, NULL, 0);
        xTWEAK("Temp|HUD Meter Min Ping Time", &tweak_min_ping_time, 0.0f, 1.0f, NULL, NULL, 0);
        xTWEAK("Temp|HUD Meter Pitch Scale", &tweak_pitch_scale, -100000.0f, 100000.0f, NULL, NULL, 0);
    }
}

void meter_widget::destruct()
{
    widget::destruct();
}

U32 meter_widget::type() const
{
    static U32 myid = xStrHash(meter_asset::type_name());
    return myid;
}

bool meter_widget::is(U32 id) const
{
    return id == meter_widget::type() || widget::is(id);
}

void meter_widget::updater(F32 dt)
{
    widget::updater(dt);

    ping_delay += dt;

    if (inc_value == 0.0f) return;

    F32 old_value = value;

    value += inc_value * dt;
    inc_value += inc_accel * dt;

    if ((inc_value < 0.0f) ? 1 : 0) {
        if (value <= end_value) {
            value = end_value;
            inc_value = 0.0f;
        }
        if ((S32)value != (S32)old_value && res->sound.decrement && ping_delay > tweak_min_ping_time) {
            F32 pitch = tweak_pitch_scale * -inc_value;
            ping_delay = 0.0f;
            pings.play(xSndMgrGetSoundGroup(res->sound.decrement), 0, NULL, NULL, NULL, &pitch, NULL);
        }
    } else {
        if (value >= end_value) {
            value = end_value;
            inc_value = 0;
        }
        if ((S32)value != (S32)old_value && res->sound.increment && ping_delay > tweak_min_ping_time) {
            F32 pitch = tweak_pitch_scale * inc_value;
            ping_delay = 0.0f;
            pings.play(xSndMgrGetSoundGroup(res->sound.increment), 0, NULL, NULL, NULL, &pitch, NULL);
        }
    }
}

#if DEBUG || RELEASE
DECOMP_FORCEACTIVE("xHud.h", "*always*", "%s")
#endif

}
