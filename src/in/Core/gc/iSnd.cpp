#include "xCommon.h"
#include "iSnd.h"

#include "xSnd.h"
#include "xSndMgr.h"
#include "xstransvc.h"
#include "xFile.h"
#include "iCutscene.h"
#include "iSystem.h"
#include "iTRC.h"
#include "zMain.h"

#include <fmodgc.h>
#include <rwcore.h>
#include <string.h>

#define MAX_SND_TABLES (12)
#define MAX_TEMP_STREAMS (8)
#define MAX_EXTERNAL_STREAMS (2)

struct ExternalStreamInfo
{
    FSOUND_STREAM* stream;
    S32 bytesToConsume;
    S32 frontBuffer;
    void* pData;
};

struct sDSPADPCM : FSOUND_GCADPCM_HDR
{
    U32 uAssetID;
};

struct gcSoundInfoAsset
{
    U32 uSoundInfoID;
    union
    {
        U32 uSumFSBSize;
        S32 nOffsetFinalFSBEnd;
    };
    FMUSIC_MODULE* pFMusicMod;
    S32* pFSBFileArray;
    gcWavInfo* pWavInfoArray;
    sDSPADPCM* pCutsceneAudioHeaders;
    U16 nWavFiles;
    U16 nSounds;
    U16 nStreams;
    U8 nFSBFiles;
    U8 nCutsceneAudioHeaders;
};

static void* sSndStaticBuffer;
ExternalStreamInfo externStreamInfo[MAX_EXTERNAL_STREAMS];
gcSoundInfoAsset sinfoArray[MAX_SND_TABLES];
U32 sinfoArrayLoaded;
static xSndVoiceInfo* pTempStreams[MAX_TEMP_STREAMS];
static bool sOldPauseState[xSndCat_LAST_CATEGORY + 1];
static F32 sOldVolume[xSndCat_LAST_CATEGORY + 1];

static bool loadSoundInfoAsset(U32 uInfoArrayIndex, PKRAssetTOCInfo& rAssetInfo);
static bool unloadSoundInfoAsset(U32 uInfoArrayIndex);
static sDSPADPCM* iSndFindCutsceneHeader(U32 id);
static void iSndCloseStream(S32 idx);
static bool iSndPlayDone(xSndVoiceInfo* vp);

void iSndInit()
{
    for (xSndVoiceInfo* voice = gSnd.voice; voice < gSnd.voice + XSND_TOTAL_VOICES; voice++) {
        voice->ps.nChannel = -1;
    }
    for (S32 i = 0; i < MAX_TEMP_STREAMS; i++) {
        pTempStreams[i] = NULL;
    }

    iSystemARAMInit();

    if (FSOUND_GetVersion() < FMOD_VERSION) {
        return;
    }

    if (!FSOUND_SetAramBlock(iSystemARAMBasePtrForFMOD(), iSystemARAMSizeForFMOD())) {
        return;
    }

    FSOUND_SetMixer(FSOUND_MIXER_QUALITY_AUTODETECT);

    sSndStaticBuffer = RwMalloc(ISND_STATIC_BUFFER_SIZE, rwMEMHINTDUR_NADURATION);
    FSOUND_SetMemorySystem(sSndStaticBuffer, ISND_STATIC_BUFFER_SIZE, NULL, NULL, NULL);

    FSOUND_SetMaxHardwareChannels(ISND_MAX_VOICES);

    if (!FSOUND_Init(32000, 0, FSOUND_INIT_GC_INITLIBS)) {
        return;
    }

    FSOUND_3D_SetRolloffFactor(0.0f);

    if (OSGetSoundMode() == OS_SOUND_MODE_MONO)
        xSndSetMode(xSndMode_Mono);
    else
        xSndSetMode(xSndMode_Stereo);
}

void iSndInitSceneLoaded()
{
    S32 nSInfos = xSTAssetCountByType('SNDI');

    for (S32 i = 0; i < nSInfos; i++) {
        PKRAssetTOCInfo ainfo;
        xSTGetAssetInfoByType('SNDI', i, &ainfo);
        for (S32 i = 0; i < sinfoArrayLoaded; i++) {
            if (ainfo.aid == sinfoArray[i].uSoundInfoID) goto next;
        }
        bool bOk = loadSoundInfoAsset(sinfoArrayLoaded, ainfo);
        if (bOk) {
            sinfoArrayLoaded++;
        }
    next:;
    }
}

static bool loadSoundInfoAsset(U32 uInfoArrayIndex, PKRAssetTOCInfo& rAssetInfo)
{
    bool retval = true;
    gcSoundInfoAsset& rSoundAsset = sinfoArray[uInfoArrayIndex];
    U32 uFileSize = 0;
    const char* hipname = xST_xAssetID_HIPFullPath(rAssetInfo.aid);

    xFile hopFile;
    U32 open = iFileOpen(hipname, IFILE_READ, &hopFile);

    S32 uSoundInfoOffsetBytes = rAssetInfo.plus_offset + rAssetInfo.sector * iFileGetSectorSize(XFILE_DRIVE_CDVD);

    iFileSeek(&hopFile, uSoundInfoOffsetBytes, 0);
    iFileRead(&hopFile, &rSoundAsset, sizeof(gcSoundInfoAsset));

    U32 dataBufferSize = 0;
    dataBufferSize += rSoundAsset.nFSBFiles * sizeof(S32);
    dataBufferSize += rSoundAsset.nWavFiles * sizeof(gcWavInfo);
    dataBufferSize += rSoundAsset.nCutsceneAudioHeaders * sizeof(sDSPADPCM);
    dataBufferSize = (dataBufferSize + 31) & ~31;
    U8* dataBuffer = (U8*)xMEMALLOC(dataBufferSize, 32);

    iFileSeek(&hopFile, rSoundAsset.uSumFSBSize, 1);
    if (dataBufferSize) {
        iFileRead(&hopFile, dataBuffer, dataBufferSize);
    }
    iFileClose(&hopFile);

    if (rSoundAsset.nFSBFiles) {
        rSoundAsset.pFSBFileArray = (S32*)dataBuffer;
    }
    if (rSoundAsset.nWavFiles) {
        rSoundAsset.pWavInfoArray = (gcWavInfo*)(dataBuffer + rSoundAsset.nFSBFiles * sizeof(S32));
    }
    if (rSoundAsset.nCutsceneAudioHeaders) {
        rSoundAsset.pCutsceneAudioHeaders = (sDSPADPCM*)(rSoundAsset.pWavInfoArray + rSoundAsset.nWavFiles);
    }

    for (S32 i = 0; i < rSoundAsset.nFSBFiles; i++) {
        rSoundAsset.pFSBFileArray[i] += uSoundInfoOffsetBytes + sizeof(gcSoundInfoAsset);
    }

    for (S32 i = 0; i < rSoundAsset.nWavFiles; i++) {
        rSoundAsset.pWavInfoArray[i].uSoundInfoIndex = uInfoArrayIndex;
    }

    if (rSoundAsset.nSounds) {
        S32 uStaticBufferLength = 0;
        if (rSoundAsset.nFSBFiles > 1)
            uStaticBufferLength = rSoundAsset.pFSBFileArray[1] - rSoundAsset.pFSBFileArray[0];
        else
            uStaticBufferLength = rSoundAsset.uSumFSBSize;

        rSoundAsset.pFMusicMod = FMUSIC_LoadSongEx(hipname, rSoundAsset.pFSBFileArray[0], uStaticBufferLength,
                                                   FSOUND_NONBLOCKING | FSOUND_GCADPCM, NULL, 0);

        volatile S32 nOpenState = FMUSIC_GetOpenState(rSoundAsset.pFMusicMod);

        while (nOpenState == -2) {
            nOpenState = FMUSIC_GetOpenState(rSoundAsset.pFMusicMod);
            if (iTRC_CHECK()) {
                zMainShowProgressBar(-1);
                zMainShowProgressBar(-1);
            }
            iVSync();
        }

        switch (nOpenState) {
        case 0:
            retval = true;
            break;
        case -1:
            retval = false;
            break;
        case -2:
            retval = true;
            break;
        case -3:
            retval = false;
        }
    }

    rSoundAsset.nOffsetFinalFSBEnd += uSoundInfoOffsetBytes + sizeof(gcSoundInfoAsset);

    if (rSoundAsset.nStreams) {
        FSOUND_Stream_SetBufferSize(500);
    }

    return retval;
}

static bool unloadSoundInfoAsset(U32 uInfoArrayIndex)
{
    gcSoundInfoAsset& rSoundAsset = sinfoArray[uInfoArrayIndex];
    if (rSoundAsset.nSounds) {
        S8 bOK = FMUSIC_FreeSong(rSoundAsset.pFMusicMod);
        rSoundAsset.pFMusicMod = NULL;
        return bOK;
    }
    return true;
}

void iSndSceneExit()
{
    for (U32 i = 0; i < sinfoArrayLoaded; i++) {
        bool bOk = unloadSoundInfoAsset(i);
    }
    sinfoArrayLoaded = 0;

    FSOUND_AUXFX_ShutdownEffect(0);
}

void iSndExit()
{
    FSOUND_Close();
}

iSndFileInfo* iSndLookup(U32 id)
{
    gcSoundInfoAsset* pSoundBankInfo = NULL;
    for (S32 i = 0; i < sinfoArrayLoaded; i++) {
        pSoundBankInfo = &sinfoArray[i];
        S32 lowerIndex = 0;
        S32 upperIndex = pSoundBankInfo->nWavFiles;

        while (upperIndex > lowerIndex) {
            S32 thisIndex = (upperIndex + lowerIndex) / 2;

            if (id == pSoundBankInfo->pWavInfoArray[thisIndex].uAssetID) {
                return (iSndFileInfo*)&pSoundBankInfo->pWavInfoArray[thisIndex];
            }

            if (id > pSoundBankInfo->pWavInfoArray[thisIndex].uAssetID)
                lowerIndex = thisIndex + 1;
            else
                upperIndex = thisIndex;
        }
    }
    return NULL;
}

static sDSPADPCM* iSndFindCutsceneHeader(U32 id)
{
    gcSoundInfoAsset* pSoundBankInfo = NULL;
    for (S32 i = 0; i < sinfoArrayLoaded; i++) {
        pSoundBankInfo = &sinfoArray[i];
        sDSPADPCM* pHeader = NULL;
        for (S32 j = 0; j < pSoundBankInfo->nCutsceneAudioHeaders; j++) {
            pHeader = &pSoundBankInfo->pCutsceneAudioHeaders[j];
            if (pHeader->uAssetID == id) {
                return pHeader;
            }
        }
    }
    return NULL;
}

bool initBytesToConsume(FSOUND_STREAM* stream)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == NULL) {
            externStreamInfo[i].stream = stream;
            externStreamInfo[i].bytesToConsume = 0;
            externStreamInfo[i].frontBuffer = 1;
            externStreamInfo[i].pData = NULL;
            return true;
        }
    }
    return false;
}

bool releaseBytesToConsume(FSOUND_STREAM* stream)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream) {
            externStreamInfo[i].stream = NULL;
            externStreamInfo[i].bytesToConsume = 0;
            externStreamInfo[i].frontBuffer = 1;
            externStreamInfo[i].pData = NULL;
            return true;
        }
    }
    return false;
}

S32 getBytesToConsume(FSOUND_STREAM* stream)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream)
            return externStreamInfo[i].bytesToConsume;
    }
    return -1;
}

S32 getFrontBuffer(FSOUND_STREAM* stream)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream)
            return externStreamInfo[i].frontBuffer;
    }
    return -1;
}

void* getDataPtr(FSOUND_STREAM* stream)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream)
            return externStreamInfo[i].pData;
    }
    return NULL;
}

bool setBytesToConsume(FSOUND_STREAM* stream, S32 bytesToConsume)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream) {
            externStreamInfo[i].bytesToConsume = bytesToConsume;
            return true;
        }
    }
    return false;
}

bool setFrontBuffer(FSOUND_STREAM* stream, S32 frontBuffer)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream) {
            externStreamInfo[i].frontBuffer = frontBuffer;
            return true;
        }
    }
    return false;
}

bool setDataPtr(FSOUND_STREAM* stream, void* pData)
{
    for (S32 i = 0; i < MAX_EXTERNAL_STREAMS; i++) {
        if (externStreamInfo[i].stream == stream) {
            externStreamInfo[i].pData = pData;
            return true;
        }
    }
    return false;
}

S8 iSndExternalCB(FSOUND_STREAM* stream, void* buff, S32 len, void* userdata)
{
    S32 bytesToConsume = getBytesToConsume(stream);
    S32 frontbuffer = getFrontBuffer(stream);
    void* pData = getDataPtr(stream);
    if (bytesToConsume == -1)
        return FALSE;
    if (bytesToConsume == 0) {
        U32 datasize = 0;
        U32 assetID = (U32)userdata;
        pData = iCSSoundGetData(assetID, &datasize);
        bytesToConsume += datasize;
    }

    if (bytesToConsume < len) {
        if (bytesToConsume == 0)
            return FALSE;
        memcpy(buff, pData, bytesToConsume);
        U8* pEndOfCpy = (U8*)buff + bytesToConsume;
        memset(pEndOfCpy, 0, len - bytesToConsume);
        bytesToConsume = 0;
    } else {
        memcpy(buff, pData, len);
        bytesToConsume -= len;
        pData = (U8*)pData + len;
    }

    setBytesToConsume(stream, bytesToConsume);

    if (frontbuffer) {
        FSOUND_GC_Stream_SetGCADPCMLoopContext(stream);
    }
    frontbuffer ^= 1;
    setFrontBuffer(stream, frontbuffer);
    setDataPtr(stream, pData);

    return TRUE;
}

static void iSndCloseStream(S32 idx)
{
    if (gSnd.voice[idx].flags & xSnd_CUTSCENE) {
        releaseBytesToConsume(gSnd.voice[idx].ps.pStream);
    }

    S32 nOpenState = FSOUND_Stream_GetOpenState(gSnd.voice[idx].ps.pStream);
    if (nOpenState == -2) {
        return;
    }

    iTRCDisk::SetInFModLockingCode(true);
    FSOUND_Stream_Close(gSnd.voice[idx].ps.pStream);
    iTRCDisk::SetInFModLockingCode(false);

    gSnd.voice[idx].ps.pStream = NULL;
    gSnd.voice[idx].ps.nChannel = -1;
    gSnd.voice[idx].flags = 0;
}

bool iSndPlay(xSndVoiceInfo* vp)
{
    gcSoundInfoAsset* pSoundBankInfo = NULL;
    iSndFileInfo* pSndInfo = vp->pSndFileInfo;
    FSOUND_SAMPLE* pSample;

    if (!(vp->flags & xSnd_CUTSCENE)) {
        if (!pSndInfo) {
            goto failed;
        }

        pSoundBankInfo = &sinfoArray[pSndInfo->uSoundInfoIndex];
    }

    if ((vp->flags & xSnd_CUTSCENE) || pSndInfo->isStreamed()) {
        U32 uOpenMode = FSOUND_GCADPCM;
        if (vp->flags & xSnd_CALC3D)
            uOpenMode |= FSOUND_HW3D;
        else
            uOpenMode |= FSOUND_HW2D;

        if ((pSndInfo && pSndInfo->isLooping()) || (vp->flags & xSnd_LOOP))
            uOpenMode |= FSOUND_LOOP_NORMAL;
        else
            uOpenMode |= FSOUND_LOOP_OFF;

        if (vp->flags & xSnd_CUTSCENE) {
            sDSPADPCM* pHeader = iSndFindCutsceneHeader(vp->assetID);
            if (!pHeader) {
                goto failed;
            }

            vp->ps.pStream = FSOUND_Stream_Create(iSndExternalCB, 0x4000, uOpenMode | FSOUND_MONO, pHeader->sample_rate,
                                                  (void*)vp->assetID);
            if (!vp->ps.pStream) {
                goto failed;
            }

            initBytesToConsume(vp->ps.pStream);

            FSOUND_GCADPCMINFO* info = (FSOUND_GCADPCMINFO*)pHeader->coef;
            pSample = FSOUND_Stream_GetSample(vp->ps.pStream);
            if (!pSample) {
                iSndCloseStream(vp - gSnd.voice);
                goto failed;
            }
            FSOUND_GC_Sample_SetGCADPCMInfo(pSample, info);
            S32 nChannel = FSOUND_Stream_PlayEx(FSOUND_FREE, vp->ps.pStream, NULL, TRUE);
            if (nChannel == -1) {
                iSndCloseStream(vp - gSnd.voice);
                goto failed;
            }
            vp->ps.nChannel = nChannel;
        } else {
            xSndVoiceInfo* pVoice = xSndMgr_GetFreeVirtualVoiceInfo(vp->priority, vp->flags);
            if (pVoice) {
                uOpenMode |= FSOUND_NONBLOCKING;

                S32 nOffsetToStartStream = pSoundBankInfo->pFSBFileArray[pSndInfo->uFSBIndex];
                S32 nStreamSize = 0;
                if (pSndInfo->uFSBIndex == pSoundBankInfo->nFSBFiles - 1)
                    nStreamSize = pSoundBankInfo->nOffsetFinalFSBEnd - pSoundBankInfo->pFSBFileArray[pSndInfo->uFSBIndex];
                else
                    nStreamSize = pSoundBankInfo->pFSBFileArray[pSndInfo->uFSBIndex + 1] - pSoundBankInfo->pFSBFileArray[pSndInfo->uFSBIndex];

                const char* hipname = xST_xAssetID_HIPFullPath(pSoundBankInfo->uSoundInfoID);

                vp->ps.pStream = FSOUND_Stream_Open(hipname, uOpenMode, nOffsetToStartStream, nStreamSize);

                *pVoice = *vp;

                vp->flags = 0;

                U32 nIndex;
                for (nIndex = 0; nIndex < MAX_TEMP_STREAMS; nIndex++) {
                    if (!pTempStreams[nIndex]) {
                        pTempStreams[nIndex] = pVoice;
                        break;
                    }
                }
                if (nIndex == MAX_TEMP_STREAMS) {
                    iSndCloseStream(vp - gSnd.voice);
                    goto failed;
                }
            } else {
                goto failed;
            }

            iSndSetVol(pVoice, pVoice->vol);
            return true;
        }
    } else {
        pSample = FMUSIC_GetSample(pSoundBankInfo->pFMusicMod, pSndInfo->uAudioSampleIndex);
        if (!pSample) {
            goto failed;
        }

        U32 samplemode = 0;
        if (vp->flags & xSnd_CALC3D)
            samplemode = FSOUND_HW3D;
        else
            samplemode = FSOUND_HW2D;

        if (pSndInfo->isLooping())
            FSOUND_Sample_SetMode(pSample, samplemode | FSOUND_LOOP_NORMAL);
        else
            FSOUND_Sample_SetMode(pSample, samplemode | FSOUND_LOOP_OFF);

        S32 nChannel = FSOUND_PlaySoundEx(FSOUND_FREE, pSample, NULL, TRUE);
        if (nChannel == -1) {
            goto failed;
        }

        vp->ps.nChannel = nChannel;
    }

    S32 nRawChanZero = FSOUND_GetSubChannel(vp->ps.nChannel, 0);

    if (vp != &gSnd.voice[nRawChanZero]) {
        if (gSnd.voice[nRawChanZero].flags & xSnd_ACTIVE) {
            if (!gSnd.voice[nRawChanZero].pSndFileInfo) {
                iSndCloseStream(nRawChanZero);
            }
            else if (gSnd.voice[nRawChanZero].pSndFileInfo->isStreamed()) {
                iSndCloseStream(nRawChanZero);
            }
            else if (gSnd.voice[nRawChanZero].pSndFileInfo->isLooping()) {
                xSndMgr_SetInaudible(&gSnd.voice[nRawChanZero]);
            }
        }

        gSnd.voice[nRawChanZero] = *vp;

        vp->flags = 0;
        vp->ps.nChannel = -1;
    }
    iSndSetVol(&gSnd.voice[nRawChanZero], gSnd.voice[nRawChanZero].vol);
    return iSndPlayDone(&gSnd.voice[nRawChanZero]);

failed:
    return false;
}

static bool iSndPlayDone(xSndVoiceInfo* vp)
{
    S32 nRawChanZero = FSOUND_GetSubChannel(vp->ps.nChannel, 0);

    FSOUND_SetPriority(vp->ps.nChannel, vp->priority);

    FSOUND_AUXFX_SetLevels(vp->ps.nChannel, 0, 0);

    iSndSetPitch(&gSnd.voice[nRawChanZero], gSnd.voice[nRawChanZero].xpitch);

    if (gSnd.voice[nRawChanZero].flags & xSnd_CALC3D) {
        FSOUND_GetNumSubChannels(vp->ps.nChannel);

        F32 fMinDist = gSnd.voice[nRawChanZero].innerRadius;
        F32 fMaxDist = gSnd.voice[nRawChanZero].outerRadius;
        FSOUND_3D_SetMinMaxDistance(gSnd.voice[nRawChanZero].ps.nChannel, fMinDist, fMaxDist);

        xVec3 vPosInvX = gSnd.voice[nRawChanZero].actualPos;
        vPosInvX.x = -vPosInvX.x;
        FSOUND_3D_SetAttributes(gSnd.voice[nRawChanZero].ps.nChannel, (F32*)&vPosInvX, NULL);
    }

    if (m_eCurrentEffect == xSndEffect_CAVE) {
        if (gSnd.voice[nRawChanZero].flags & xSnd_EFFECTS) {
            FSOUND_AUXFX_SetLevels(gSnd.voice[nRawChanZero].ps.nChannel, 0xFF, 0);

            S8 ret;
            switch (gSnd.sndMode) {
            case xSndMode_Dolby:
                ret = FSOUND_AUXFX_SetReverbHiDpl2(1.5f, 0.008f, 0.0f, 1.0f, 0.5f);
                break;
            default:
                ret = FSOUND_AUXFX_SetReverbHi(0, 1.5f, 0.008f, 0.0f, 1.0f, 0.0f, 0.5f);
            }
        }
    }

    if (!xSndMgrIsPaused(gSnd.voice[nRawChanZero].category) && !(gSnd.voice[nRawChanZero].flags & xSnd_PAUSED)) {
        FSOUND_SetPaused(gSnd.voice[nRawChanZero].ps.nChannel, FALSE);
    }

    return true;
}

void iSndStartStereo(iSndHandle, iSndHandle)
{
}

void iSndSetMode()
{
    switch (gSnd.sndMode) {
    case xSndMode_Mono:
        FSOUND_SetSpeakerMode(FSOUND_SPEAKERMODE_MONO);
        break;
    case xSndMode_Stereo:
        FSOUND_SetSpeakerMode(FSOUND_SPEAKERMODE_STEREO);
        break;
    case xSndMode_Dolby:
        FSOUND_SetSpeakerMode(FSOUND_SPEAKERMODE_PROLOGIC2);
        break;
    }
}

void iSndSetEnvironmentalEffect(S32 effectType)
{
    switch (effectType) {
    case xSndEffect_NONE:
        FSOUND_AUXFX_SetLevels(FSOUND_ALL, 0, 0);
        break;
    case xSndEffect_CAVE:
        break;
    }
}

xSndVoiceInfo* iSndGetFreeVoiceInfo(U32 priority, U32 flags)
{
    U32 lo_priority = 0xFFFFFFFF;
    U32 lo_voice = 0;
    U32 i = 0;
    for (; i < ISND_MAX_VOICES; i++) {
        if (gSnd.voice[i].flags) {
            if (gSnd.voice[i].priority < lo_priority) {
                lo_priority = gSnd.voice[i].priority;
                lo_voice = i;
            }
        } else {
            lo_voice = i; lo_priority = 0;
            break;
        }
    }

    if (priority >= lo_priority) {
        if (gSnd.voice[lo_voice].ps.nChannel != -1) {
            U32 loopMode = FSOUND_GetLoopMode(gSnd.voice[lo_voice].ps.nChannel);
            bool islooped = loopMode && loopMode != FSOUND_LOOP_OFF;
            bool isstreaming = (gSnd.voice[lo_voice].flags & xSnd_CUTSCENE) || gSnd.voice[lo_voice].pSndFileInfo->isStreamed();
            if (islooped || isstreaming) {
                iprintf("Emergency stop on voice %d (%s)\n", lo_voice, xSTAssetName(gSnd.voice[lo_voice].assetID));
                iSndStop(&gSnd.voice[lo_voice]);
            }
        }

        iSndStop(&gSnd.voice[lo_voice]);
        return &gSnd.voice[lo_voice];
    }

    return NULL;
}

void iSndUpdate()
{
    for (S32 i = 0; i < MAX_TEMP_STREAMS; i++) {
        if (pTempStreams[i]) {
            S32 nOpenState = FSOUND_Stream_GetOpenState(pTempStreams[i]->ps.pStream);

            if (nOpenState == -2) continue;

            if (nOpenState < 0) {
                if (nOpenState != -1)
                    iSndCloseStream(pTempStreams[i] - gSnd.voice);
                pTempStreams[i] = NULL;
                continue;
            }

            bool bCloseStream = false;
            if (pTempStreams[i]->flags & xSnd_ACTIVE) {
                S32 nChannel = FSOUND_Stream_PlayEx(FSOUND_FREE, pTempStreams[i]->ps.pStream, NULL, TRUE);

                if (nChannel != -1) {
                    S32 nRawChanZero = FSOUND_GetSubChannel(nChannel, 0);

                    gSnd.voice[nRawChanZero] = *pTempStreams[i];
                    gSnd.voice[nRawChanZero].ps.nChannel = nChannel;

                    iSndPlayDone(&gSnd.voice[nRawChanZero]);
                } else
                    bCloseStream = true;
            } else {
                bCloseStream = true;
            }

            if (bCloseStream) {
                FSOUND_Stream_Close(pTempStreams[i]->ps.pStream);
                pTempStreams[i]->ps.pStream = NULL;
                pTempStreams[i]->ps.nChannel = -1;
            }

            pTempStreams[i]->flags = 0;
            pTempStreams[i] = NULL;
        }
    }

    bool bDialogActive = false;
    for (S32 i = 0; i < ISND_MAX_VOICES; i++) {
        if (!(gSnd.voice[i].flags & xSnd_ACTIVE) || (gSnd.voice[i].flags & xSnd_PAUSED)) continue;

        S8 playing = FSOUND_IsPlaying(gSnd.voice[i].ps.nChannel);
        S8 paused = FSOUND_GetPaused(gSnd.voice[i].ps.nChannel);
        bool is3d = gSnd.voice[i].flags & xSnd_CALC3D;

        U32 loopMode = FSOUND_GetLoopMode(gSnd.voice[i].ps.nChannel);
        bool islooped = loopMode && loopMode != FSOUND_LOOP_OFF;

        if (!playing && !paused) {
            if ((gSnd.voice[i].flags & xSnd_CUTSCENE) || gSnd.voice[i].pSndFileInfo->isStreamed()) {
                iSndCloseStream(i);
            }
            else if (islooped) {
                if (gSnd.voice[i].pSndFileInfo->isStreamed()) {
                }

                xSndMgr_SetInaudible(&gSnd.voice[i]);
                gSnd.voice[i].ps.nChannel = -1;
                gSnd.voice[i].flags = 0;
            } else {
                gSnd.voice[i].ps.nChannel = -1;
                gSnd.voice[i].flags = 0;
            }
        } else {
            if (is3d) {
                xVec3 vPosInvX = gSnd.voice[i].actualPos;
                vPosInvX.x = -vPosInvX.x;
                FSOUND_3D_SetAttributes(gSnd.voice[i].ps.nChannel, (F32*)&vPosInvX, NULL);

                if (gSnd.voice[i].flags & xSnd_DOPPLER) {
                    S32 nSampleFreq;
                    FSOUND_SAMPLE* pSample = FSOUND_GetCurrentSample(gSnd.voice[i].ps.nChannel);
                    FSOUND_Sample_GetDefaults(pSample, &nSampleFreq, NULL, NULL, NULL);
                    F32 iShiftedFreq = xSndFreqFromXPitch(gSnd.voice[i].xpitch, nSampleFreq);
                    S32 iDoppleredFreq = iShiftedFreq * gSnd.voice[i].dopplerFreqCoef;
                    FSOUND_SetFrequency(gSnd.voice[i].ps.nChannel, iDoppleredFreq);
                }
            }

            iSndSetVol(&gSnd.voice[i], gSnd.voice[i].vol);
            bDialogActive |= (gSnd.voice[i].category == xSndCat_DIALOG);
        }
    }
    xSndMgrSetDialogPlaying(bDialogActive);

    xVec3 vListenerPosInvX = gSnd.pos;
    vListenerPosInvX.x = -vListenerPosInvX.x;
    FSOUND_3D_Listener_SetAttributes((F32*)&vListenerPosInvX, NULL, -gSnd.at.x, gSnd.at.y, gSnd.at.z, -gSnd.up.x,
                                     gSnd.up.y, gSnd.up.z);
    FSOUND_Update();

    FSOUND_GetMemoryStats(&gCurrSoundMemoryUsed, &gHighwaterSoundMemoryUsed);
}

bool iSndIsReady(iSndHandle)
{
    return true;
}

void iSndStop(xSndVoiceInfo* vp)
{
    S32 nRawChanZeroCheck = vp - gSnd.voice;

    if (vp->ps.nChannel == -1) {
        vp->flags = 0;
    }
    else if ((vp->flags & xSnd_CUTSCENE) || vp->pSndFileInfo->isStreamed()) {
        iSndCloseStream(vp - gSnd.voice);
    } else {
        if (!FSOUND_IsPlaying(vp->ps.nChannel)) {
            S8 paused = FSOUND_GetPaused(vp->ps.nChannel);
            U32 loopMode = FSOUND_GetLoopMode(vp->ps.nChannel);
            bool islooped = loopMode && loopMode != FSOUND_LOOP_OFF;
        } else {
            FSOUND_GetNumSubChannels(vp->ps.nChannel);
            S32 nRawChanZero = FSOUND_GetSubChannel(vp->ps.nChannel, 0);

            if (nRawChanZero == -1) {
            } else {
                FSOUND_StopSound(vp->ps.nChannel);
            }
        }
        vp->ps.nChannel = -1;
        vp->flags = 0;
    }
}

void iSndPause(xSndVoiceInfo* vp, U32 paused)
{
    FSOUND_SetPaused(vp->ps.nChannel, !!paused);
}

void iSndSetVol(xSndVoiceInfo* vp, F32 xVolume)
{
    F32 fVolume = xVolume * xSndMgrGetDampedVolume(vp->category);

    U8 iVolume = iSndVolFromX(fVolume);

    if (vp->flags & xSnd_PANRIGHT)
        FSOUND_SetPan(vp->ps.nChannel, 255);
    else if (vp->flags & xSnd_PANLEFT)
        FSOUND_SetPan(vp->ps.nChannel, 0);

    FSOUND_SetVolume(vp->ps.nChannel, iVolume);
}

void iSndSetPitch(xSndVoiceInfo* pVoice, F32 xpitch)
{
    S32 nSampleFreq;
    FSOUND_SAMPLE* pSample = FSOUND_GetCurrentSample(pVoice->ps.nChannel);
    FSOUND_Sample_GetDefaults(pSample, &nSampleFreq, NULL, NULL, NULL);
    F32 fShiftedFreq = xSndFreqFromXPitch(xpitch, nSampleFreq);
    S32 iDoppleredFreq = fShiftedFreq * pVoice->dopplerFreqCoef;
    FSOUND_SetFrequency(pVoice->ps.nChannel, iDoppleredFreq);
}

void iSndKill()
{
    bool oneOpen;

    xSndMgrStopSounds(xSndCat_ALL, false);

    do {
        iSndUpdate();

        oneOpen = false;
        for (U32 i = 0; i < MAX_TEMP_STREAMS; i++) {
            if (pTempStreams[i]) {
                oneOpen = true;
                break;
            }
        }
    } while (oneOpen);

    FSOUND_Close();
}

void iSndSetExternalCallback(void*)
{
}

void iSndSuspendCD(U32)
{
}

void iSndSuspend()
{
    for (S16 i = 0; i < xSndCat_LAST_CATEGORY + 1; i++) {
        sOldPauseState[i] = xSndMgrIsPaused(i);
        sOldVolume[i] = xSndMgrGetUserVolume(i);
    }
    xSndMgrPauseSounds(xSndCat_ALL, true, true);
    xSndMgrUpdate();
}

void iSndResume()
{
    for (S16 i = 0; i < xSndCat_LAST_CATEGORY + 1; i++) {
        if (!sOldPauseState[i]) {
            xSndMgrPauseSounds(i, false, true);
        }
        xSndMgrSetUserVolume(i, sOldVolume[i]);
    }
    xSndMgrUpdate();
}
