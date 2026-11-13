#include "xCommon.h"
#include "xPad.h"

#include "zScene.h"
#include "zMenu.h"
#include "zGame.h"

#include <string.h>

xPad mPad[k_XPAD_MAX];

S32 xPadInit()
{
    memset(mPad, 0, sizeof(mPad));

    if (!iPadInit()) {
        return 0;
    }

    return 1;
}

xPad* xPadEnable(S32 idx)
{
    xPad* p = &mPad[idx];

    if (p->state != ePad_Disabled) {
        return p;
    }

    if (idx != 0) {
        return p;
    }

    p = iPadEnable(p, idx);
    xPadRumbleEnable(idx, TRUE);
    return p;
}

void xPadRumbleEnable(S32 idx, S32 enable)
{
    xPad* p = &mPad[idx];

    if (p->state != ePad_Enabled) {
        enable = FALSE;
    }

    if (enable) {
        if (p->flags & 0x4)
            p->flags |= 0x8;
    } else {
        if (p->flags & 0x8) {
            p->flags ^= 0x8;
        }
    }
}

S32 xPadUpdate(S32 idx, F32 time_passed)
{
    U32 new_on = 0;

    if (idx != 0) {
        return 0;
    }

    if (zScene_ScreenAdjustMode()) {
        xPadAnalogIsDigital(idx, FALSE);
    }
    else if (zMenuRunning() || zGameIsPaused()) {
        xPadAnalogIsDigital(idx, TRUE);
    } else {
        xPadAnalogIsDigital(idx, FALSE);
    }

    xPad* p = &mPad[idx];

    S32 ret = iPadUpdate(p, &new_on);
    if (!ret) {
        p->pressed = 0;
        p->released = 0;
        return 1;
    }

    if (p->flags & 0x10) {
        if (p->flags & 0x1) {
            U32 fake_dpad = 0;
            if (p->analog1.x >= 50) {
                fake_dpad |= 0x20;
            }
            else if (p->analog1.x <= -50) {
                fake_dpad |= 0x80;
            }
            if (p->analog1.y >= 50) {
                fake_dpad |= 0x40;
            }
            else if (p->analog1.y <= -50) {
                fake_dpad |= 0x10;
            }
            if (fake_dpad == 0) {
                p->al2d_timer = 0.0f;
            } else {
                p->al2d_timer -= time_passed;
                if (p->al2d_timer <= 0.0f) {
                    new_on |= fake_dpad;
                    p->al2d_timer = 0.35f;
                }
            }
        }
    }

    p->pressed = new_on & ~p->on;
    p->released = p->on & ~new_on;
    p->on = new_on;

    for (S32 i = 0; i < k_XPAD_BUTTON_MAX; i++) {
        if (p->pressed & (1<<i)) {
            p->down_tmr[i] = 0.0f;
        } else if (p->released & (1<<i)) {
            p->up_tmr[i] = 0.0f;
        }
        if (p->on & (1<<i)) {
            p->down_tmr[i] += time_passed;
        } else {
            p->up_tmr[i] += time_passed;
        }
    }

    if (p->flags & 0x10) {
        if (!(p->on & 0x10) && !(p->on & 0x40) && !(p->on & 0x80) && !(p->on & 0x20)) {
            p->d_timer = 0.0f;
        } else {
            p->d_timer -= time_passed;
            if (p->d_timer <= 0.0f) {
                p->d_timer = 0.35f;
                if (p->on & 0x10) {
                    p->pressed |= 0x10;
                }
                else if (p->on & 0x40) {
                    p->pressed |= 0x40;
                }
                if (p->on & 0x80) {
                    p->pressed |= 0x80;
                }
                else if (p->on & 0x20) {
                    p->pressed |= 0x20;
                }
            }
        }
    }

    return 1;
}

void xPadNormalizeAnalog(xPad& pad, S32 inner_zone, S32 outer_zone)
{
    const PadAnalog* src = &pad.analog1;
    for (S32 i = 0; i < k_XPAD_ANALOG_MAX; i++) {
        pad.analog[i].offset.x = normalize_analog(src[i].x, -outer_zone, outer_zone, 0, -inner_zone, inner_zone);
        pad.analog[i].offset.y = normalize_analog(src[i].y, -outer_zone, outer_zone, 0, -inner_zone, inner_zone);
        if (pad.analog[i].offset.x == 0.0f && pad.analog[i].offset.y == 0.0f) {
            pad.analog[i].mag = 0.0f;
            pad.analog[i].dir.assign(1.0f, 0.0f);
            pad.analog[i].ang = 0.0f;
        } else {
            if (pad.analog[i].offset.x > 1.0f) pad.analog[i].offset.x = 1.0f;
            else if (pad.analog[i].offset.x < -1.0f) pad.analog[i].offset.x = -1.0f;
            if (pad.analog[i].offset.y > 1.0f) pad.analog[i].offset.y = 1.0f;
            else if (pad.analog[i].offset.y < -1.0f) pad.analog[i].offset.y = -1.0f;
            pad.analog[i].mag = pad.analog[i].offset.length();
            pad.analog[i].dir = pad.analog[i].offset;
            pad.analog[i].dir /= pad.analog[i].mag;
            pad.analog[i].ang = xatan2(pad.analog[i].dir.y, pad.analog[i].dir.x);
        }
    }
}

void xPadKill()
{
    iPadKill();
}

void xPadAnalogIsDigital(S32 idx, S32 enable)
{
    if (idx != 0) {
        return;
    }

    xPad* pad = &mPad[idx];

    if (enable)
        pad->flags |= 0x10;
    else
        pad->flags &= ~0x10;

    if (pad->al2d_timer >= 0.35f)
        pad->al2d_timer = 0.35f;

    if (pad->ar2d_timer >= 0.35f)
        pad->ar2d_timer = 0.35f;

    if (pad->d_timer >= 0.35f)
        pad->d_timer = 0.35f;
}
