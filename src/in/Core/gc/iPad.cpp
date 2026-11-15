#include "xCommon.h"
#include "iPad.h"

#include "xPad.h"
#include "xTRC.h"
#include "zMain.h"
#include "iTime.h"
#include "iTRC.h"

F32 g_rumblingThreshold = 0.5f;

static PADStatus sPadData[PAD_MAX_CONTROLLERS] = {};
static u32 sPadMotor[PAD_MAX_CONTROLLERS];
static S32 sPadCurrLevel[PAD_MAX_CONTROLLERS];
static S32 sPadDeltaLevel[PAD_MAX_CONTROLLERS];

S32 iPadInit()
{
    S32 i;

    PADInit();

    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        sPadCurrLevel[i] = 0;
        sPadDeltaLevel[i] = 0;
        sPadMotor[i] = PAD_MOTOR_STOP;
    }
    return TRUE;
}

xPad* iPadEnable(xPad* pad, S16 port)
{
    pad->port = port;
    pad->slot = 0;
    pad->state = ePad_Enabled;
    gTrcPad[pad->port].state = TRC_PadInserted;
    pad->flags |= 0x3;
    pad->flags |= 0x4;

    U32 new_on;
    iPadUpdate(pad, &new_on);
    pad->on = new_on;
    pad->pressed = 0;
    pad->released = 0;

    return pad;
}

static inline U32 iPadConvFromGCN(U32 buttons, U32 gcn_flag, U32 x_flag)
{
    if (buttons & gcn_flag)
        return x_flag;
    return 0;
}

static S8 iPadConvStick(F32 in)
{
    if (in > 40.0f)
        in = 40.0f;
    else if (in < -40.0f)
        in = -40.0f;

    F32 fin = 3.2f * in;
    if (fin > 127.0f)
        fin = 127.0f;
    else if (fin < -127.0f)
        fin = -127.0f;

    return (S8)fin;
}

S32 iPadUpdate(xPad* pad, U32* on)
{
    U32 read_on = 0;

    if (pad->port == 0) {
        PADRead(sPadData);
        PADClamp(sPadData);
    }

    switch (sPadData[pad->port].err) {
    case PAD_ERR_NO_CONTROLLER:
        xTRCPad(pad->port, TRC_PadMissing);
        PADReset(PAD_CHAN0_BIT >> pad->port);
        pad->pressed = 0;
        pad->released = 0;
        pad->analog1.x = 0;
        pad->analog1.y = 0;
        pad->analog2.x = 0;
        pad->analog2.y = 0;
        break;
    case PAD_ERR_NOT_READY:
    case PAD_ERR_TRANSFER:
        return FALSE;
    default:
        break;
    case PAD_ERR_NONE:
        goto read;
    }
    return FALSE;

read:
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_LEFT, 0x80);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_RIGHT, 0x20);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_DOWN, 0x40);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_UP, 0x10);

    if (sPadData[pad->port].button & PAD_TRIGGER_Z) {
        read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_TRIGGER_R, 0x2000);
        read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_TRIGGER_L, 0x200);

        if (sPadData[pad->port].triggerLeft >= 0x18)
            read_on |= 0x200;
        if (sPadData[pad->port].triggerRight >= 0x18)
            read_on |= 0x2000;

        read_on |= 0x100000;
    } else {
        read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_TRIGGER_R, 0x1000);
        read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_TRIGGER_L, 0x100);

        if (sPadData[pad->port].triggerLeft >= 0x18)
            read_on |= 0x100;
        if (sPadData[pad->port].triggerRight >= 0x18)
            read_on |= 0x1000;
    }

    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_A, 0x10000);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_B, 0x80000);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_Y, 0x40000);
    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_X, 0x20000);

    read_on |= iPadConvFromGCN(sPadData[pad->port].button, PAD_BUTTON_START, 0x1);

    *on = read_on;

    pad->analog1.x = iPadConvStick(sPadData[pad->port].stickX);
    pad->analog1.y = -iPadConvStick(sPadData[pad->port].stickY);
    pad->analog2.x = iPadConvStick(sPadData[pad->port].substickX);
    pad->analog2.y = -iPadConvStick(sPadData[pad->port].substickY);

    if (gTrcPad[pad->port].state != TRC_PadInserted)
        xTRCPad(pad->port, TRC_PadInserted);

    iPadUpdateRumble(pad);

    static iTime held_start = 0;
    static bool held_through_reset = true;

    if (pad->port == 0) {
        bool held = (*on & 0xA0001) == 0xA0001;
        if (held && !held_through_reset) {
            if (iTimeDiffSec(held_start, iTimeGet()) >= 0.5f)
                ResetButton::ResetSystem();
        } else {
            held_start = iTimeGet();
            if (!held)
                held_through_reset = false;
        }
    }

    return TRUE;
}

void iPadStopRumble(xPad* pad)
{
    sPadCurrLevel[pad->port] = 0;
    sPadDeltaLevel[pad->port] = 0;
    sPadMotor[pad->port] = PAD_MOTOR_STOP;
    PADControlMotor(pad->port, PAD_MOTOR_STOP);
}

void iPadStopRumble()
{
    xPad* pad = &mPad[globals.currentActivePad];
    sPadCurrLevel[pad->port] = 0;
    sPadDeltaLevel[pad->port] = 0;
    sPadMotor[pad->port] = PAD_MOTOR_STOP;
    PADControlMotor(pad->port, PAD_MOTOR_STOP);
}

void iPadStartRumble(xPad* pad, F32 intensity)
{
    if (pad->flags & 0x8) {
        if (intensity < g_rumblingThreshold) {
            sPadCurrLevel[pad->port] = 0;
            sPadDeltaLevel[pad->port] = 0;
            sPadMotor[pad->port] = PAD_MOTOR_STOP;
        } else {
            sPadCurrLevel[pad->port] = 0;
            sPadDeltaLevel[pad->port] = 3600.0 * (0.05 + intensity);
            sPadMotor[pad->port] = PAD_MOTOR_RUMBLE;
        }

        PADControlMotor(pad->port, sPadMotor[pad->port]);
    }
}

void iPadUpdateRumble(xPad* pad)
{
    if (pad->flags & 0x8) {
        sPadCurrLevel[pad->port] += sPadDeltaLevel[pad->port];
        if (sPadCurrLevel[pad->port] > 3600) {
            sPadCurrLevel[pad->port] = 0;
            if (sPadMotor[pad->port] != PAD_MOTOR_RUMBLE) {
                sPadMotor[pad->port] = PAD_MOTOR_RUMBLE;
                PADControlMotor(pad->port, sPadMotor[pad->port]);
            }
        }
        else if (sPadMotor[pad->port] != PAD_MOTOR_STOP) {
            sPadMotor[pad->port] = PAD_MOTOR_STOP;
            PADControlMotor(pad->port, sPadMotor[pad->port]);
        }
    }
}

void iPadKill()
{
}
