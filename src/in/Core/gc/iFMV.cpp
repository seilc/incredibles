#include "xCommon.h"
#include "iFMV.h"

#include "xCam.h"
#include "xColor.h"
#include "xFile.h"
#include "xFMV.h"
#include "xGlobals.h"
#include "xPad.h"
#include "xstransvc.h"
#include "xSubTitles.h"
#include "xTRC.h"
#include "iARAMTmp.h"
#include "iCamera.h"
#include "iDebug.h"
#include "iFile.h"
#include "iPad.h"
#include "iSystem.h"
#include "iTime.h"
#include "iTRC.h"
#include "zGame.h"
#include "zGameState.h"

#include <bink.h>
#include <rwcore.h>
#include <dolphin.h>
#include <string.h>

static U32 iFMVPlay(const char* filename, U32 buttons, F32, U32 uSubtitlesAID, bool skippable, bool lockController, bool displayDemoBlink, S32 uAudioTrack);

inline S32 NextPow2(S32 in)
{
    S32 ret = 1;
    while (ret < in)
        ret <<= 1;
    return ret;
}

BINK* hBinkCurrMovie;
static bool sARAMtmp;
static U32 amt;

static void* BigMovieAlloc(U32 uNumBytes)
{
    void* addr;

    if (sARAMtmp) {
        if (uNumBytes <= amt) {
            addr = iARAMTmpMalloc('ANIM', uNumBytes);
            if ((U32)addr == 0xBEEEEEEF)
                addr = RwMalloc(uNumBytes, 0);
            else
                amt -= uNumBytes;
        } else
            addr = RwMalloc(uNumBytes, 0);
    } else {
        addr = RwMalloc(uNumBytes, 0);
    }

    return addr;
}

static void BigMovieFree(void* pMemToFree)
{
    if (iARAMTmpInARAMTemp('ANIM', pMemToFree)) {
        iARAMTmpFree('ANIM', pMemToFree);
    } else {
        RwFree(pMemToFree);
    }
}

static void* LilMovieAlloc(U32 uNumBytes)
{
    void* addr = RwMalloc(uNumBytes, 0);
    return addr;
}

static void LilMovieFree(void* pMemToFree)
{
    RwFree(pMemToFree);
}

bool iFMVSystemInit()
{
    return true;
}

static BINK* iFMVOpen(const char* fullFile, U32 uBinkFlags)
{
    RADARAMCALLBACKS OurGCCallbacks;
    OurGCCallbacks.aram_malloc = iSystemARAMAlloc;
    OurGCCallbacks.aram_free = iSystemARAMFree;
    BinkSoundUseAX(&OurGCCallbacks);

    iTRCDisk::SetMovieResetCallBack(iFMVResetCallback);
    iTRCDisk::EnableMovieResetCallBack(true);

    BinkSetMemory(BigMovieAlloc, BigMovieFree);

    BINK* hBinkMovie = NULL;
    if (!iFileExists(fullFile, 0)) {
        return hBinkMovie;
    }

    S32 numRetries = 0;
    do {
        if (iTRCDisk::IsDiskIDed()) {
            if ((gGameMode == eGameMode_Game || gGameMode == eGameMode_Pause) && !sARAMtmp) {
                sARAMtmp = true;
                amt = iARAMTmpBegin('ANIM', 0x280000, true);
            }

            hBinkMovie = BinkOpen(fullFile, uBinkFlags);
            if (!hBinkMovie) {
                iprintf("BinkOpen failed:  %s\n", BinkGetError());
            }
        }

        if (iTRC_CHECK()) {
            numRetries++;
            if (hBinkMovie) {
                BinkClose(hBinkMovie);
                hBinkMovie = NULL;
            }
            continue;
        }

        if (!hBinkMovie) {
            numRetries++;
            xFile tfile;
            iFileOpen(fullFile, 0, &tfile);
            DVDSeekAsyncPrio(&tfile.ps.file, 0, NULL, 2);
            iTRC_CHECK();
            DVDCancel(&tfile.ps.file.cb);
            iFileClose(&tfile);
        }
    } while (!hBinkMovie && numRetries < 3);

    xRegion reg = xSTGetLocalizationEnum();
    // BUG: always true, || instead of &&
    if (reg > eRegionUS && reg < eRegionUK && (reg != eRegionJP || reg != eRegionKR) && hBinkMovie->Frames == 0x511) {
        if (strstr(fullFile, "FMVOM01.")) {
            hBinkMovie->Frames = 0x50E;
        }
    }

    if (strstr(fullFile, "PROMO2.") && reg == eRegionJP) {
        BinkGoto(hBinkMovie, 60, 0);
    }

    return hBinkMovie;
}

static void iFMVClose(BINK* hMovie)
{
    BinkClose(hMovie);

    iTRCDisk::SetMovieResetCallBack(NULL);
    iTRCDisk::EnableMovieResetCallBack(false);

    if (sARAMtmp) {
        sARAMtmp = false;
        iARAMTmpEnd('ANIM', true);
    }
}

static MOVIE* movie_array;
static S32 movie_asset_count;

void iFMVScenePostInit()
{
    movie_asset_count = xSTAssetCountByType('BINK');
    movie_array = (MOVIE*)xMEMALLOC(movie_asset_count * sizeof(MOVIE), 0);
    memset(movie_array, 0, movie_asset_count * sizeof(MOVIE));

    PKRAssetTOCInfo temp_asset_info;
    for (S32 i = 0; i < movie_asset_count; i++) {
        xSTGetAssetInfoByType('BINK', i, &temp_asset_info);
        const char* hipname = xST_xAssetID_HIPFullPath(temp_asset_info.aid);
        char fullname[255];
        iFileFullPath(hipname, fullname);

        S32 bFileOpenOk = DVDOpen(fullname, &movie_array[i].file);
        if (!bFileOpenOk) {
            continue;
        }
        movie_array[i].aid = temp_asset_info.aid;
    }
}

void iFMVSceneExit()
{
    for (S32 i = 0; i < movie_asset_count; i++) {
        DVDClose(&movie_array[i].file);
    }
    movie_asset_count = 0;
    movie_array = NULL;
}

MOVIE* iFMVOpen(PKRAssetTOCInfo& rAssetInfo)
{
    RADARAMCALLBACKS OurGCCallbacks;
    OurGCCallbacks.aram_malloc = iSystemARAMAlloc;
    OurGCCallbacks.aram_free = iSystemARAMFree;
    BinkSoundUseAX(&OurGCCallbacks);

    iTRCDisk::SetMovieResetCallBack(iFMVResetCallback);
    iTRCDisk::EnableMovieResetCallBack(true);

    BinkSetMemory(LilMovieAlloc, LilMovieFree);

    MOVIE* pMovie = NULL;
    for (S32 i = 0; i < movie_asset_count; i++) {
        if (rAssetInfo.aid == movie_array[i].aid) {
            pMovie = &movie_array[i];
            break;
        }
    }

    if (!pMovie) {
        iFMVClose(pMovie);
    }

    pMovie->uMovieOffsetBytes = rAssetInfo.sector * iFileGetSectorSize(XFILE_DRIVE_CDVD) + rAssetInfo.plus_offset;
    pMovie->file.cb.userData = (void*)pMovie->uMovieOffsetBytes;
    pMovie->pBinkTexture = NULL;
    pMovie->hBink = BinkOpen((const char*)&pMovie->file, BINKFILEHANDLE | BINKNOFILLIOBUF);
    if (!pMovie->hBink) {
        iFMVClose(pMovie);
    } else {
        S32 width = NextPow2(pMovie->hBink->Width);
        S32 height = NextPow2(pMovie->hBink->Height);
        RwRaster* pBinkRaster = RwRasterCreate(width, height, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
        if (!pBinkRaster) {
            iFMVClose(pMovie);
        } else {
            RwRasterPushContext(pBinkRaster);
            RwRasterClear(*(RwInt32*)&g_BLACK);
            RwRasterPopContext();

            pMovie->pBinkTexture = RwTextureCreate(pBinkRaster);
            if (!pMovie->pBinkTexture) {
                iFMVClose(pMovie);
            }
        }
    }

    return pMovie;
}

void iFMVClose(MOVIE*& pMovie)
{
    iTRCDisk::SetMovieResetCallBack(NULL);
    iTRCDisk::EnableMovieResetCallBack(false);

    if (pMovie->pBinkTexture) {
        RwTextureDestroy(pMovie->pBinkTexture);
        pMovie->pBinkTexture = NULL;
    }

    BinkClose(pMovie->hBink);
    pMovie->hBink = NULL;
    pMovie = NULL;
}

U32 iFMVPlay(const char* filename, U32 buttons, F32 time, U32 uSubtitlesAID, bool skippable, bool lockController, bool displayDemoBlink)
{
    char fullname[64];
    char BIK[5] = ".BIK";

    if (IsPS2Demo()) {
        strcpy(fullname, xglobals->PS2demo->subdir);
        strcat(fullname, "\\");
        strcat(fullname, filename);
        strcat(fullname, BIK);
    } else {
        strcpy(fullname, filename);
        strcat(fullname, BIK);
    }

    if (!iFileExists(fullname, 0)) {
        return 1;
    }

    xFMVPauseSound(true);

    eGameOstrich old = zGameGetOstrich();
    zGameSetOstrich(eGameOstrich_PlayingMovie);

    xRegion localRegion = xSTGetLocalizationEnum();
    U32 ret = iFMVPlay(fullname, buttons, time, uSubtitlesAID, skippable, lockController, displayDemoBlink, localRegion);

    zGameSetOstrich(old);

    xFMVPauseSound(false);
    return ret;
}

void iFMVSuspend()
{
    if (hBinkCurrMovie) {
        BinkPause(hBinkCurrMovie, 1);
    }

    for (S32 i = 0; i < movie_asset_count; i++) {
        if (movie_array[i].hBink && !movie_array[i].movie_is_suspended) {
            movie_array[i].disk_open_frame = movie_array[i].hBink->LastFrameNum;
            BinkPause(movie_array[i].hBink, 1);
            movie_array[i].movie_is_suspended = 1;
        }
    }
}

void iFMVResume()
{
    for (S32 i = 0; i < movie_asset_count; i++) {
        if (movie_array[i].movie_is_suspended) {
            movie_array[i].file.cb.userData = (void*)movie_array[i].uMovieOffsetBytes;
            BinkClose(movie_array[i].hBink);
            movie_array[i].hBink = NULL;

            BINK* hBink = BinkOpen((const char*)&movie_array[i].file, BINKFILEHANDLE | BINKNOFILLIOBUF);
            if (hBink) {
                BinkGoto(hBink, movie_array[i].disk_open_frame, 0);
                movie_array[i].hBink = hBink;
                movie_array[i].movie_is_suspended = 0;
            }
        }
    }
}

static U32 iFMVPlay(const char* filename, U32 buttons, F32, U32 uSubtitlesAID, bool skippable, bool lockController, bool displayDemoBlink, S32 uAudioTrack)
{
    iPadStopRumble(xglobals->pad0);

    if (skippable) {
        buttons |= 0x20002;
    }

    char fullFile[256];
    U32 uBinkFlags = 0;
    if (!filename) {
        return 1;
    }
    iFileFullPath(filename, fullFile);

    uBinkFlags = 0;
    BinkSetSoundTrack(1, (U32*)&uAudioTrack);
    uBinkFlags |= BINKSNDTRACK;

    BINK* hBinkMovie = iFMVOpen(fullFile, uBinkFlags);
    if (!hBinkMovie) {
        iTime delay = iTimeGet();
        iTime now = iTimeGet();
        while (delay + 200 > now)
            now = iTimeGet();

        if (sARAMtmp) {
            sARAMtmp = false;
            iARAMTmpEnd('ANIM', true);
        }

        return 1;
    }

    bool bTrackFound = false;
    for (S32 track = 0; track < hBinkMovie->NumTracks; track++) {
        if (uAudioTrack == hBinkMovie->trackIDs[track]) {
            bTrackFound = true;
            break;
        }
    }

    if (!bTrackFound) {
        iFMVClose(hBinkMovie);

        hBinkMovie = iFMVOpen(fullFile, uBinkFlags & ~BINKSNDTRACK);
        uAudioTrack = BinkGetTrackID(hBinkMovie, 0);
    }

    BinkSetVolume(hBinkMovie, uAudioTrack, xFMVGetBinkCompliantVolume());

    RwRaster* pCamRaster = RwCameraGetRaster(xglobals->screen->icam);
    RwInt32 iBackBufWidth = RwRasterGetWidth(pCamRaster);
    RwInt32 iBackBufHeight = RwRasterGetHeight(pCamRaster);
    S32 iSafeWidth = iBackBufWidth * 1.0f;
    S32 iSafeHeight = iBackBufHeight * 1.0f;
    iSafeWidth = (iSafeWidth + 3) & ~3;
    iSafeHeight = (iSafeHeight + 3) & ~3;

    RwRect MovieRect = { 0, 0, hBinkMovie->Width, hBinkMovie->Height };
    F32 aspect_ratio_mult = 1.0f;

    if (MovieRect.w > iSafeWidth) {
        MovieRect.h = (F32)MovieRect.h * iSafeWidth / MovieRect.w;
        MovieRect.w = iSafeWidth;
    }

    if (MovieRect.h > iSafeHeight) {
        MovieRect.w = (F32)MovieRect.w * iSafeHeight / MovieRect.h;
        MovieRect.h = iSafeHeight;
    }

    MovieRect.h = MovieRect.h * aspect_ratio_mult;
    if (MovieRect.h > iSafeHeight) {
        MovieRect.w = (F32)MovieRect.w * iSafeHeight / MovieRect.h;
        MovieRect.h = iSafeHeight;
    }

    S32 iHorizPad = iBackBufWidth - MovieRect.w;
    S32 iVertPad = iBackBufHeight - MovieRect.h;
    if (iHorizPad < 0)
        iHorizPad = 0;
    if (iVertPad < 0)
        iVertPad = 0;
    MovieRect.x = iHorizPad / 2;
    MovieRect.y = iVertPad / 2;

    S32 controllerStart = lockController ? xglobals->currentActivePad : 0;
    S32 controllerEnd = xglobals->currentActivePad + 1;

    iTime fmvAbortDelay = iTimeGet();
    bool aborted = false;

    iTime dwTimeStart = iTimeGet();
    iTime dwTimeNow = iTimeGet();

    void* xfbDisp, *xfbCopy;
    RwGameCubeGetXFBs(&xfbDisp, &xfbCopy);

    S32 iBinkMovieHalfHeight = (S32)hBinkMovie->Height / 2;
    if (iBinkMovieHalfHeight & 1) {
        iBinkMovieHalfHeight++;
    }

    RwRaster* pBinkRaster = RwRasterCreate(hBinkMovie->Width, iBinkMovieHalfHeight, 32, rwRASTERTYPENORMAL | rwRASTERFORMAT888 | rwRASTERDONTALLOCATE);
    RwGameCubeRasterExtension* pRasterExt = RwGameCubeRasterGetExtension(pBinkRaster);

    hBinkCurrMovie = hBinkMovie;
    bool waiting = false;

    while (hBinkMovie->FrameNum != hBinkMovie->Frames && !aborted) {
        if (iTRC_CHECK()) {
            RwGameCubeGetXFBs(&xfbDisp, &xfbCopy);
            VISetBlack(TRUE);

            VISetNextFrameBuffer(xfbDisp);

            VIFlush();
            VISetBlack(FALSE);

            break;
        }

        if (!waiting) {
            dwTimeNow = iTimeGet();
            if (fmvAbortDelay + 2000.0f <= dwTimeNow && skippable) {
                for (S32 i = controllerStart; i < controllerEnd; i++) {
                    xPad* pad = &mPad[i];
                    xPadUpdate(i, 0.0f);

                    if ((pad->pressed & buttons) && (gTrcPad[0].state == TRC_PadInserted || !lockController)) {
                        aborted = true;
                        break;
                    }
                }
            }

            if (gSoak) {
                F32 time = hBinkMovie->FrameNum / ((F32)hBinkMovie->FrameRate / hBinkMovie->FrameRateDiv);
                if (time > 2.0f) {
                    aborted = true;
                }
            }

            S32 oneIfSkip = BinkDoFrame(hBinkMovie);

            RwRasterPushContext(pCamRaster);

            RwGameCubeGetXFBs(&xfbDisp, &xfbCopy);
            pRasterExt->pixels = (RwUInt8*)xfbCopy;

            U8* pBits = RwRasterLock(pBinkRaster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKRAW);

            oneIfSkip = BinkCopyToBufferRect(hBinkMovie, pBits, RwRasterGetStride(pBinkRaster), iBinkMovieHalfHeight, 0, 0, 0, 0, RwRasterGetWidth(pBinkRaster), iBinkMovieHalfHeight, BINKCOPYALL | BINKSURFACE32);

            RwRasterUnlock(pBinkRaster);

            if (oneIfSkip == 0) {
                RwRasterClear(0xFF000000);
                RwRasterRenderFast(pBinkRaster, MovieRect.x, MovieRect.y);

                pBits = RwRasterLock(pBinkRaster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKRAW);

                BinkCopyToBufferRect(hBinkMovie, pBits, RwRasterGetStride(pBinkRaster), iBinkMovieHalfHeight, 0, 0, 0, hBinkMovie->Height - iBinkMovieHalfHeight, RwRasterGetWidth(pBinkRaster), iBinkMovieHalfHeight, BINKCOPYALL | BINKNOSKIP | BINKSURFACE32);

                RwRasterUnlock(pBinkRaster);
                RwRasterRenderFast(pBinkRaster, MovieRect.x, MovieRect.y + iBinkMovieHalfHeight);
            }

            RwRasterPopContext();

            pRasterExt->pixels = NULL;
            if (uSubtitlesAID || displayDemoBlink) {
                xglobals->screen->begin_render(false);
                F32 fTime = (F32)(hBinkMovie->FrameNum * hBinkMovie->FrameRateDiv) / hBinkMovie->FrameRate;
                xSubTitlesRender(uSubtitlesAID, fTime);
                if (displayDemoBlink)
                    zGameDemoText(fTime);
                xglobals->screen->end_render();
            }

            iCameraShowRaster(xglobals->screen->icam);

            if (hBinkMovie->FrameNum != hBinkMovie->Frames) {
                BinkNextFrame(hBinkMovie);
            }
        }

        waiting = BinkWait(hBinkMovie);
    }

    RwRasterDestroy(pBinkRaster);

    iFMVClose(hBinkMovie);
    hBinkCurrMovie = NULL;

    return 0;
}

void iFMVResetCallback()
{
    iTRC_CHECK();

    OSSetResetCallback(iFMVResetCallback);
}

void iFMVKill()
{
    for (S32 i = 0; i < movie_asset_count; i++) {
        if (movie_array[i].hBink) {
            BinkClose(movie_array[i].hBink);
            movie_array[i].hBink = NULL;
        }
    }

    if (hBinkCurrMovie) {
        BinkClose(hBinkCurrMovie);
        hBinkCurrMovie = NULL;
    }

    iTRCDisk::SetMovieResetCallBack(NULL);
    iTRCDisk::EnableMovieResetCallBack(false);

    if (sARAMtmp) {
        sARAMtmp = false;
        iARAMTmpEnd('ANIM', false);
    }
}
