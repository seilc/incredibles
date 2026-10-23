#include "xFMV.h"

#include "xSndMgr.h"
#include "zGame.h"
#include "zMenu.h"

#include <bink.h>

typedef void (*xFMVPauseSoundCB)(bool doPause);

static void DefaultFMVPauseSoundCB(bool doPause);

static F32 sFMVVolumeScale = 1.0f;
static xFMVPauseSoundCB sxFMVPauseSoundCB = DefaultFMVPauseSoundCB;

void xFMVPauseSound(bool doPause)
{
    sxFMVPauseSoundCB(doPause);

    if (doPause)
    {
        xSndMgrUpdate();
    }
}

static void DefaultFMVPauseSoundCB(bool doPause)
{
    if (zGameIsPaused())
    {
        xSndMgrPauseSounds(5, doPause, true);

        if (zMenuRunning())
        {
            xSndMgrPauseSounds(3, doPause, true);
        }
    }
    else
    {
        xSndMgrPauseSounds(-3, doPause, true);
    }
}

S32 xFMVGetBinkCompliantVolume()
{
    S32 volume = (S32)(32768.0f * xSndMgrGetDampedVolume(4) * sFMVVolumeScale);
    return volume;
}

RwTexture* xFMVGetTexture(MOVIE* pMovie, U16 looping, U16 freezeframed, U32 startframe,
                          U32 endframe, U32)
{
    if (!pMovie)
    {
        return NULL;
    }

    if (!pMovie->hBink)
    {
        return NULL;
    }

    if (endframe == 0)
    {
        endframe = pMovie->hBink->Frames;
    }

    if (pMovie->hBink->LastFrameNum == -1)
    {
        BinkGoto(pMovie->hBink, startframe + 1, NULL);
    }

    RwRaster* raster = RwTextureGetRaster(pMovie->pBinkTexture);

    if (!BinkWait(pMovie->hBink))
    {
        BinkDoFrame(pMovie->hBink);

        U8* pBits = RwRasterLock(raster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKRAW);

        BinkCopyToBuffer(pMovie->hBink, pBits, RwRasterGetStride(raster), RwRasterGetHeight(raster),
                         0, 0, BINKCOPYALL | BINKSURFACE32);

        RwRasterUnlock(raster);

        if (pMovie->hBink->Paused != freezeframed)
        {
            BinkPause(pMovie->hBink, freezeframed);
        }

        if (looping)
        {
            if (endframe == pMovie->hBink->LastFrameNum && startframe != 0)
            {
                BinkGoto(pMovie->hBink, startframe, NULL);
            }

            BinkNextFrame(pMovie->hBink);
            BinkWait(pMovie->hBink);
        }
        else
        {
            if (endframe != pMovie->hBink->LastFrameNum)
            {
                BinkNextFrame(pMovie->hBink);
                BinkWait(pMovie->hBink);
            }
        }
    }

    return pMovie->pBinkTexture;
}