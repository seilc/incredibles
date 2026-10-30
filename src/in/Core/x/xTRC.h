#ifndef XTRC_H
#define XTRC_H

#include "types.h"
#include "iTRC.h"

#define k_MAX_TRC_PADS 4

typedef enum _tagTRCState
{
    TRC_Unknown,
    TRC_PadMissing,
    TRC_PadInserted,
    TRC_PadInvalidNoAnalog,
    TRC_PadInvalidType,
    TRC_DiskNotIdentified,
    TRC_DiskIdentified,
    TRC_DiskTrayOpen,
    TRC_DiskTrayClosed,
    TRC_DiskNoDisk,
    TRC_DiskInvalid,
    TRC_DiskRetry,
    TRC_DiskFatal,
    TRC_Total
} TRCState;

struct _tagTRCPadInfo : _tagiTRCPadInfo
{
    S32 id;
    TRCState state;
};

struct _tagTRCDiskInfo
{
    TRCState state;
    TRCState id;
};

void xTRCInit();
void xTRCInitMessages();
void xTRCClearPadMsg();
void xTRCRender();
void xTRCReset();
void xTRCPad(S32 pad_id, TRCState state);
void xTRCDisk(TRCState state);
void render_mem_card_no_space(S32 needed, S32 available, S32 neededFiles, bool enabled);
void RenderText(const char* text, bool enabled);

#endif
