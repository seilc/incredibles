#include "xFMV.h"

#include "xSndMgr.h"
#include "zGame.h"
#include "zMenu.h"

#include <bink.h>

static void DefaultFMVPauseSoundCB(bool doPause);

static F32 sFMVVolumeScale = 1.0f;
static xFMVPauseSoundCallback sxFMVPauseSoundCB = DefaultFMVPauseSoundCB;

void xFMVPauseSound(bool doPause)
{
    sxFMVPauseSoundCB(doPause);

    if (doPause) {
        xSndMgrUpdate();
    }
}

static void DefaultFMVPauseSoundCB(bool doPause)
{
    if (zGameIsPaused()) {
        xSndMgrPauseSounds(5, doPause, true);

        if (zMenuRunning()) {
            xSndMgrPauseSounds(3, doPause, true);
        }
    } else {
        xSndMgrPauseSounds(-3, doPause, true);
    }
}

S32 xFMVGetBinkCompliantVolume()
{
    S32 volume = (S32)(32768.0f * xSndMgrGetDampedVolume(4) * sFMVVolumeScale);
    return volume;
}

RwTexture* xFMVGetTexture(MOVIE* pMovie, U16 looping, U16 freezeframed, U32 startframe, U32 endframe, U32)
{
    if (!pMovie) {
        return NULL;
    }

    if (!pMovie->hBink) {
        return NULL;
    }

    if (endframe == 0) {
        endframe = pMovie->hBink->Frames;
    }

    if (pMovie->hBink->LastFrameNum == 0xFFFFFFFF) {
        BinkGoto(pMovie->hBink, startframe + 1, 0);
    }

    RwRaster* pBinkRaster = RwTextureGetRaster(pMovie->pBinkTexture);

    if (!BinkWait(pMovie->hBink)) {
        BinkDoFrame(pMovie->hBink);

        RwUInt8* pBits = RwRasterLock(pBinkRaster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKRAW);

        RwInt32 format = RwRasterGetFormat(pBinkRaster);
        RwInt32 pixformat = format & rwRASTERFORMATPIXELFORMATMASK;

        RwInt32 nDestPitch = RwRasterGetStride(pBinkRaster);

        BinkCopyToBuffer(pMovie->hBink, pBits, nDestPitch, RwRasterGetHeight(pBinkRaster), 0, 0, BINKCOPYALL | BINKSURFACE32);

        RwRasterUnlock(pBinkRaster);

        if (pMovie->hBink->Paused != freezeframed) {
            BinkPause(pMovie->hBink, freezeframed);
        }

        if (looping) {
            if (endframe == pMovie->hBink->LastFrameNum) {
                if (startframe != 0) {
                    BinkGoto(pMovie->hBink, startframe, 0);
                }
            }
            BinkNextFrame(pMovie->hBink);
            BinkWait(pMovie->hBink);
        }
        else if (endframe != pMovie->hBink->LastFrameNum) {
            BinkNextFrame(pMovie->hBink);
            BinkWait(pMovie->hBink);
        }
    }

    return pMovie->pBinkTexture;
}
