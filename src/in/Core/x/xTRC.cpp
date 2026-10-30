#include "xTRC.h"

#include "xFont.h"
#include "xTextAsset.h"
#include "iCamera.h"
#include "zGameState.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zMenu.h"
#include "zUIIncredibles.h"
#include "zVar.h"

#include <string.h>

_tagTRCPadInfo gTrcPad[k_MAX_TRC_PADS];
_tagTRCDiskInfo gTrcDisk;
static const char* k_CONTROLLER_MISSING_MESSAGE;
static const char* k_CONTROLLER_INVALID_MESSAGE;
static const xColor yellow = { 0xFF, 0xE6, 0x00, 0xFF };

void xTRCInit()
{
    memset(gTrcPad, 0, sizeof(gTrcPad));
    for (S32 i = 0; i < k_MAX_TRC_PADS; i++) {
        gTrcPad[i].id = i;
        gTrcPad[i].state = TRC_PadMissing;
    }
    memset(&gTrcDisk, 0, sizeof(gTrcDisk));
}

void xTRCInitMessages()
{
    k_CONTROLLER_MISSING_MESSAGE = xTextFindString("TRC_CONTROLLER_MISSING", NULL);
    k_CONTROLLER_INVALID_MESSAGE = xTextFindString("TRC_CONTROLLER_INVALID", NULL);
}

// RESTORED: from Rat GC
static U32 pad_message_valid()
{
    if (gGameMode == eGameMode_Boot || gGameMode == eGameMode_Intro) {
        return FALSE;
    }
    if (zMenuRunning()) {
        return FALSE;
    }
    if (zGameModeGet() == eGameMode_Save || zGameModeGet() == eGameMode_Load) {
        return TRUE;
    }
    return TRUE;
}
// END RESTORED

static void render_message(const char* s)
{
    static xtextbox tb = xtextbox::create(xfont::create(0, NSCREENX(19.0f), NSCREENY(22.0f), 0.0f, yellow, screen_bounds), screen_bounds);
    tb.set_text(s);
    tb.bounds = screen_bounds;
    tb.bounds.contract(0.1f);
    tb.bounds.h = tb.yextent(true);
    tb.bounds.y = 0.5f - 0.5f * tb.bounds.h;
    xColor c = { 0x00, 0x00, 0x00, 0xC8 };
    render_fill_rect(tb.font.clip, c);
    tb.render(true);
}

static char composed_message_text[256];
static bool is_controller_message;
static const char* message_text;
// RESTORED: from Incredibles PS2
static U32 standalone;
// END RESTORED

void xTRCClearPadMsg()
{
    message_text = NULL;
    composed_message_text[0] = '\0';
}

static bool ShouldDisplayMessage()
{
    if (!is_controller_message) {
        return true;
    }
    if (!globals.cmgr && !globals.player.ControlOff) {
        return true;
    }
    if (zGameModeGet() == eGameMode_Pause && zUIIncrediblesGetPauseType() == 1) {
        return true;
    }
    return false;
}

void xTRCRender()
{
    if (ShouldDisplayMessage()) {
        if (composed_message_text[0] != '\0')
            render_message(composed_message_text);
        else if (message_text)
            render_message(message_text);
    }
}

void xTRCReset()
{
    xTRCClearPadMsg();
    xglobals->dontShowPadMessageDuringLoadingOrCutScene = false;
}

// RESTORED: from Incredibles PS2
static S32 DisplayMessage(TRCState state)
{
    switch (state) {
    case TRC_Unknown:
        message_text = NULL;
        break;
    case TRC_PadMissing:
        if (!pad_message_valid()) {
            return FALSE;
        }
        message_text = k_CONTROLLER_MISSING_MESSAGE;
        U32 len_message_text = strlen(message_text);
        if (len_message_text < 256) {
            const char* msg = message_text;
            char c;
            while ((c = *msg++) != '\0' && c != '<') {}
            if (c == '\0') {
                break;
            }
            U32 sub_len = msg - message_text;
            memcpy(composed_message_text, message_text, sub_len);
            composed_message_text[sub_len - 1] = '1' + globals.pad0->port;
            memcpy(composed_message_text + sub_len, &message_text[sub_len + 2], len_message_text - sub_len - 1);
            composed_message_text[len_message_text - 2] = '\0';
        }
        break;
    case TRC_PadInvalidType:
        message_text = k_CONTROLLER_INVALID_MESSAGE;
        break;
    case TRC_DiskNotIdentified:
        message_text = "{i:k_disk_not_identified}";
        break;
    case TRC_DiskTrayOpen:
        message_text = "{i:k_disk_cover_open}";
        break;
    case TRC_DiskNoDisk:
        message_text = "{i:k_disk_no_disk}";
        break;
    case TRC_DiskInvalid:
        message_text = "{i:k_disk_wrong_disk}";
        break;
    case TRC_DiskRetry:
        message_text = "{i:k_disk_retry}";
        break;
    case TRC_DiskFatal:
        message_text = "{i:k_disk_fatal}";
        break;
    default:
        return FALSE;
    }
    if (standalone || RwCameraGetCurrentCamera()) {
        RwCamera* cam = NULL;
        if (standalone) {
            cam = iCameraCreate(FB_XRES, FB_YRES, 0);
            RwRGBA bg = { 0, 0, 0, 0 };
            RwCameraClear(cam, &bg, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);
            RwCameraBeginUpdate(cam);
        }
        render_message(message_text);
        if (standalone) {
            RwCameraEndUpdate(cam);
            RwCameraShowRaster(cam, NULL, rwRASTERFLIPWAITVSYNC);
            iCameraDestroy(cam);
        }
    }
    return TRUE;
}
// END RESTORED

void xTRCPad(S32 pad_id, TRCState state)
{
}

void xTRCDisk(TRCState state)
{
    if (state != TRC_DiskNotIdentified) {
        gTrcDisk.state = state;
        gTrcDisk.id = TRC_DiskIdentified;
    } else {
        gTrcDisk.id = TRC_DiskNotIdentified;
    }
}

void render_mem_card_no_space(S32 needed, S32 available, S32 neededFiles, bool enabled)
{
    if (available < 0 && neededFiles != -1 && needed != -1) {
        available = 0;
    }
    bad_card_needed = needed;
    bad_card_available = available;
    const char* error_text = "{i:text_mem_card_no_space}";
    if (neededFiles == 0 && needed > available) {
        error_text = "{i:text_mem_card_no_space_overwrite}";
    }
    else if (neededFiles > 0 || needed > available || neededFiles == -1) {
        error_text = "{i:text_mem_card_no_space_no_save}";
    }
    RenderText(error_text, enabled);
}

void RenderText(const char* text, bool enabled)
{
    xColor c1 = { 0xFF, 0xE6, 0x00, 0xFF };
    static xtextbox tb = xtextbox::create(xfont::create(0, NSCREENX(19.0f), NSCREENY(22.0f), 0.0f, c1, screen_bounds), screen_bounds);
    tb.set_text(enabled ? text : "");
    tb.bounds = screen_bounds;
    tb.bounds.contract(0.1f);
    tb.bounds.h = tb.yextent(true);
    tb.bounds.y = 0.5f - 0.5f * tb.bounds.h;
    tb.render(true);
    xColor c2 = { 0x00, 0x00, 0x00, 0x96 };
    if (!enabled) {
        render_fill_rect(tb.font.clip, c2);
    }
}
