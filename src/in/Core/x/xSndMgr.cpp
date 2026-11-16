#include "xCommon.h"
#include "xSndMgr.h"

#include "containers.h"
#include "xDebugTweak.h"
#include "xEnt.h"
#include "xGlobals.h"
#include "xMemMgr.h"
#include "xRand.h"
#include "xstransvc.h"
#include "xutil.h"
#include "zBase.h"
#include "zBulletTime.h"
#include "zCutsceneMgr.h"
#include "zGlobals.h"
#include "zMain.h"
#include "zNPCCommon.h"

#include <fmodgc.h>
#include <string.h>

#define XSND_DEFAULT_PRIORITY ((U8)128)

xTWEAKABLE F32 xSndGlobals::speedOfSound = 200.0f;

xSndGlobals gSnd;

xSndEffect m_eCurrentEffect;

static F32 s_fCategoryVolume[6];
static F32 s_fCategoryVolumeFade[6];

inline void xSndMgr_SetInitFlag(bool bIsInit) { bIsInit ? (gSnd.m_uSndMgrFlags |= k_XSNDMGR_INITED) : (gSnd.m_uSndMgrFlags &= ~k_XSNDMGR_INITED); }

namespace
{
    struct sound_fader
    {
        iSndHandle handle;
        F32 volume;
        F32 end;
        F32 rate;
    };

    linear_pool<sound_fader> faders;

    void setup_faders()
    {
        faders.create(XSND_TOTAL_VOICES);
    }

    void reset_faders()
    {
        faders.clear();
    }

    void update_faders(F32 dt)
    {
        sound_fader* it = faders.begin(), *end = faders.end();
        while (it != end) {
            it->volume += it->rate * dt;
            if (it->rate < 0.0f && it->volume < it->end) {
                xSndMgrStop(it->handle);
                faders.erase(it);
                end--;
            }
            else if (it->rate >= 0.0f && it->volume >= it->end) {
                xSndMgrSetVolume(it->handle, it->end, true);
                faders.erase(it);
                end--;
            } else {
                xSndMgrSetVolume(it->handle, it->volume, true);
                it++;
            }
        }
    }

    xSndVoiceInfo* find_voice(iSndHandle handle)
    {
        xSndVoiceInfo* it = gSnd.voice, *end = gSnd.voice + XSND_TOTAL_VOICES;
        for (; it != end; it++) {
            if ((it->flags & xSnd_ACTIVE) && it->sndUniqueID == handle) {
                return it;
            }
        }
        return NULL;
    }

    sound_fader* find_fader(iSndHandle handle)
    {
        sound_fader* it = faders.begin(), *end = faders.end();
        for (; it != end; it++) {
            if (it->handle == handle)
                return it;
        }
        return NULL;
    }
}

xSndGroup* xSndMgr_AllocGroup(U8 nSoundAssets)
{
    U32 datasize = sizeof(xSndGroup);
    datasize += nSoundAssets * sizeof(xSndGroupInfo);
    void* pAllocated = xMEMALLOC(datasize, 0);
    xSndGroup* pRetGroup = (xSndGroup*)pAllocated;
    pRetGroup->header.uPlayedMask = 0;
    pRetGroup->header.uLastSelected = 0;
    pRetGroup->header.uSetBits = 0;
    pRetGroup->header.nMaxPlays = 0;
    pRetGroup->header.eSoundCategory = 0;
    pRetGroup->header.uPriority = XSND_DEFAULT_PRIORITY;
    pRetGroup->header.uFlags = 0;
    pRetGroup->header.fInnerRadius = 25.0f;
    pRetGroup->header.fOuterRadius = 100.0f;
    pRetGroup->header.pszGroupName = "";
    pRetGroup->header.uInfoCount = nSoundAssets;

    for (U32 i = 0; i < nSoundAssets; i++) {
        pRetGroup->aSndGroupInfo[i].fMaxPitchMult = 0.0f;
        pRetGroup->aSndGroupInfo[i].fMinPitchMult = 0.0f;
        pRetGroup->aSndGroupInfo[i].fVolume = 1.0f;
        pRetGroup->aSndGroupInfo[i].uSoundNameHash = 0;
    }
    return pRetGroup;
}

void xSndMgrInit()
{
    memset(&gSnd, 0, sizeof(gSnd));

    iSndInit();

    for (xSndVoiceInfo* voice = gSnd.voice; voice < gSnd.voice + XSND_TOTAL_VOICES; voice++) {
        voice->flags = 0;
        voice->soundGroupHandle = ISND_INVALID_SND_GROUP_HANDLE;
        voice->sndUniqueID = XSNDMGR_INVALID_SND_HANDLE;
    }

    xSndMgrSetUserVolume(xSndCat_MUSIC, 0.63636363f);
    xSndMgrSetUserVolume(xSndCat_GAME_3D, 0.72727272f);
    xSndMgrSetUserVolume(xSndCat_GAME_2D, 0.72727272f);
    xSndMgrSetUserVolume(xSndCat_DIALOG, 0.72727272f);
    xSndMgrSetUserVolume(xSndCat_CUTSCENE, 0.63636363f);
    xSndMgrSetUserVolume(xSndCat_UI, 0.72727272f);

    xSndMgrSelectListenerMode(xSndListener_PLAYER);

    m_eCurrentEffect = xSndEffect_NONE;
    iSndSetEnvironmentalEffect(xSndEffect_NONE);

    iSndSuspendCD(1);
}

void xSndMgrSelectListenerMode(xSndListener Mode)
{
    gSnd.listenerMode = Mode;
}

void xSndMgrSetEffect(xSndEffect Effect)
{
    if (m_eCurrentEffect != Effect) {
        m_eCurrentEffect = Effect;
        iSndSetEnvironmentalEffect(Effect);
    }
}

xSndEffect xSndMgrGetEffect()
{
    return m_eCurrentEffect;
}

void xSndMgrProcessSoundPosition(const xVec3* pActual, xVec3* pProcessed)
{
    switch (gSnd.listenerMode) {
    case xSndListener_PLAYER:
    {
        xVec3 playerDelta, cameraDelta;
        playerDelta = *pActual - gSnd.listenerMat[xSndListener_PLAYER].pos;
        cameraDelta = *pActual - gSnd.listenerMat[xSndListener_CAMERA].pos;
        F32 playerDist = xVec3Length(&playerDelta);
        F32 cameraDist = xVec3Length(&cameraDelta);
        if (cameraDist < playerDist) {
            F32 inwardShift = playerDist - cameraDist;
            inwardShift *= 0.5f;
            F32 factor = (playerDist - inwardShift) / playerDist;
            playerDelta *= factor;
            *pProcessed = playerDelta + gSnd.listenerMat[xSndListener_PLAYER].pos;
        }
        else if (xsqr(playerDelta.x) + xsqr(playerDelta.z) < 0.1f) {
            xVec3 fudgeAwayFromPlayer;
            xVec3SMul(fudgeAwayFromPlayer, cameraDelta, 0.25f / cameraDist);
            *pProcessed = *pActual + fudgeAwayFromPlayer;
        } else {
            *pProcessed = *pActual;
        }
        break;
    }
    case xSndListener_CAMERA:
        *pProcessed = *pActual;
        break;
    }
}

void xSndMgr_UpdateListenerPosition()
{
    switch (gSnd.listenerMode) {
    case xSndListener_PLAYER:
    {
        xMat4x3* pMat = &gSnd.listenerMat[xSndListener_CAMERA];
        gSnd.right = pMat->right;
        gSnd.up = pMat->up;
        gSnd.at = pMat->at;
        gSnd.pos = gSnd.listenerMat[xSndListener_PLAYER].pos;
        gSnd.velocity = gSnd.listenerVel[xSndListener_PLAYER];
        break;
    }
    case xSndListener_CAMERA:
    {
        xMat4x3* pMat = &gSnd.listenerMat[xSndListener_CAMERA];
        gSnd.right = pMat->right;
        gSnd.up = pMat->up;
        gSnd.at = pMat->at;
        gSnd.pos = gSnd.listenerMat[xSndListener_CAMERA].pos;
        gSnd.velocity = gSnd.listenerVel[xSndListener_PLAYER];
        break;
    }
    }
}

bool xSndMgr_UpdateVoicePosition(xSndVoiceInfo* pVoice)
{
    if (!(pVoice->flags & xSnd_ACTIVE)) {
        return true;
    }
    if (pVoice->flags & xSnd_PAUSED) {
        return true;
    }

    if (pVoice->flags & xSnd_CALC3D) {
        if (pVoice->flags & xSnd_FOLLOW) {
            if (pVoice->pPosition) {
                if (pVoice->pVelocity) {
                    pVoice->actualVelocity = *pVoice->pVelocity;
                }
                else if ((pVoice->flags & xSnd_DOPPLER) && !xfeq0(xglobals->update_dt)) {
                    xVec3 vel = (*pVoice->pPosition - pVoice->actualPos) / xglobals->update_dt;
                    xVec3Copy(&pVoice->actualVelocity, &vel);
                }
                pVoice->actualPos = *pVoice->pPosition;
            }
            else if (pVoice->pParent) {
                xEnt* ent = pVoice->pParent;
                if ((ent->baseFlags & k_XBASE_IS_ENABLED) && !xSndMgrIsPaused(pVoice->category)) {
                    xVec3Copy(&pVoice->actualPos, xEntGetPos(ent));
                    if ((pVoice->flags & xSnd_DOPPLER) && !xfeq0(xglobals->update_dt)) {
                        xVec3 vel = (ent->frame->mat.pos - ent->frame->oldmat.pos) / xglobals->update_dt;
                        if (vel.length2() > xsqr(xSndGlobals::speedOfSound)) {
                            if (ent->baseType == eBaseTypeNPC) {
                                zNPC::common* npc = (zNPC::common*)ent;
                            }
                        }
                        xVec3Copy(&pVoice->actualVelocity, &vel);
                    }
                }
                else if (!(ent->baseFlags & k_XBASE_IS_ENABLED)) {
                    pVoice->flags &= ~xSnd_FOLLOW;
                }
            }
        }

        xSndMgrProcessSoundPosition(&pVoice->actualPos, &pVoice->playPos);

        if (pVoice->flags & xSnd_DOPPLER) {
            xVec3 soundDir = gSnd.pos - pVoice->actualPos;
            F32 length2 = soundDir.length2();
            if (!xfeq0(length2)) {
                soundDir *= xinvsqrt(length2);
                xVec3* ptr = &pVoice->actualVelocity;
                F32 velListener = gSnd.velocity.dot(soundDir);
                F32 velSource = pVoice->actualVelocity.dot(soundDir);
                F32 newDoppler = (xSndGlobals::speedOfSound - velListener) / (xSndGlobals::speedOfSound - velSource);
                if (newDoppler > 0.0f)
                    pVoice->dopplerFreqCoef = newDoppler;
                else
                    pVoice->dopplerFreqCoef = 1000000.0f;
            }
        }

        xVec3 soundDirectionXYZ;
        xVec3Sub(&soundDirectionXYZ, &pVoice->playPos, &gSnd.pos);
        pVoice->distToListener = soundDirectionXYZ.length();
        if (!(pVoice->flags & xSnd_INAUDIBLE)) {
            if (pVoice->distToListener > pVoice->outerRadius) {
                if (!pVoice->pSndFileInfo->isLooping() && pVoice->distToListener > 1.2f * pVoice->outerRadius)
                    return false;
                return xSndMgr_SetInaudible(pVoice);
            }
        } else {
            if (pVoice->distToListener <= pVoice->outerRadius)
                return xSndMgr_SetAudible(pVoice);
        }
    }
    return true;
}

xSndVoiceInfo* xSndMgr_GetFreeVirtualVoiceInfo(U32 priority, U32 flags)
{
    for (S32 i = XSND_MAX_VOICES; i < XSND_TOTAL_VOICES; i++) {
        if (gSnd.voice[i].flags == 0)
            return &gSnd.voice[i];
    }
    return iSndGetFreeVoiceInfo(priority, flags);
}

void xSndMgr_Stop(xSndVoiceInfo* pVoice)
{
    S32 voiceId = pVoice - gSnd.voice;
    if (voiceId < XSND_MAX_VOICES) {
        iSndStop(pVoice);
    } else if (pVoice->flags & xSnd_INAUDIBLE) {
        pVoice->flags = 0;
        pVoice->assetID = 0;
    } else {
        pVoice->flags &= ~xSnd_ACTIVE;
    }
}

void xSndMgr_Pause(xSndVoiceInfo* pVoice, bool bPaused)
{
    S32 voiceId = pVoice - gSnd.voice;
    if (voiceId < XSND_MAX_VOICES) {
        iSndPause(pVoice, bPaused);
    }
    if (bPaused)
        pVoice->flags |= xSnd_PAUSED;
    else
        pVoice->flags &= ~xSnd_PAUSED;
}

void xSndMgr_CutscenePause(xSndVoiceInfo* pVoice, bool bPaused)
{
    if (pVoice->category != xSndCat_CUTSCENE)
        if (pVoice->pSndFileInfo && pVoice->pSndFileInfo->isLooping()) {
            xSndMgr_Pause(pVoice, bPaused);
        }
        else if (bPaused) {
            xSndMgr_Stop(pVoice);
        }
}

bool xSndMgr_SetInaudible(xSndVoiceInfo* pVoice)
{
    if (pVoice->pSndFileInfo->isStreamed()) {
        return true;
    }
    if (!pVoice->pSndFileInfo->isLooping()) {
        return true;
    }
    xSndVoiceInfo* pVoiceStorage = xSndMgr_GetFreeVirtualVoiceInfo(pVoice->priority, pVoice->flags);
    if (!pVoiceStorage || pVoiceStorage - gSnd.voice < XSND_MAX_VOICES)
        return true;

    *pVoiceStorage = *pVoice;
    xSndMgr_Stop(pVoice);
    pVoiceStorage->flags |= xSnd_INAUDIBLE;
    return false;
}

bool xSndMgr_SetAudible(xSndVoiceInfo* pVoiceStorage)
{
    xSndVoiceInfo* vp = iSndGetFreeVoiceInfo(pVoiceStorage->priority, pVoiceStorage->flags);
    if (!vp)
        return false;
    *vp = *pVoiceStorage;
    vp->flags &= ~xSnd_INAUDIBLE;

    bool bOk = iSndPlay(vp);
    if (bOk) {
        pVoiceStorage->flags = 0;
        if (xSndMgrIsPaused(vp->category))
            xSndMgrPause(vp->sndUniqueID, true);
    }
    return false;
}

void xSndMgrSceneInit()
{
    xSndMgr_SetInitFlag(true);

    for (S32 i = 0; i < ARRAY_LENGTH(s_fCategoryVolumeFade); i++) {
        s_fCategoryVolumeFade[i] = 0.0f;
    }

    gSnd.listenerMode = xSndListener_PLAYER;

    for (S32 i = 0; i < xSndListener_MAX_TYPES; i++) {
        gSnd.listenerMat[i].at.assign(1.0f, 0.0f, 0.0f);
        gSnd.listenerMat[i].right.assign(0.0f, 1.0f, 0.0f);
        gSnd.listenerMat[i].up.assign(0.0f, 0.0f, 1.0f);
        gSnd.listenerMat[i].pos.assign(999999.0f, 999999.0f, 999999.0f);
    }
    gSnd.at.assign(1.0f, 0.0f, 0.0f);
    gSnd.right.assign(0.0f, 1.0f, 0.0f);
    gSnd.up.assign(0.0f, 0.0f, 1.0f);
    gSnd.pos.assign(999999.0f, 999999.0f, 999999.0f);
    iSndUpdate();

    setup_faders();
}

void xSndMgrUpdate()
{
    xSndMgr_UpdateListenerPosition();

    for (U32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        xSndMgr_UpdateVoicePosition(&gSnd.voice[i]);
    }

    xSndMgrIsDialogPlaying();
    iSndUpdate();
    bool bDialogPlaying = xSndMgrIsDialogPlaying();

    if (ZPLAYER && ZPLAYER->IsDead()) {
        s_fCategoryVolumeFade[xSndCat_GAME_3D] += 1.25f * xglobals->update_dt;
        s_fCategoryVolumeFade[xSndCat_GAME_2D] += 1.25f * xglobals->update_dt;
        s_fCategoryVolumeFade[xSndCat_GAME_3D] = CLAMP(s_fCategoryVolumeFade[xSndCat_GAME_3D], 0.0f, 1.0f);
        s_fCategoryVolumeFade[xSndCat_GAME_2D] = CLAMP(s_fCategoryVolumeFade[xSndCat_GAME_2D], 0.0f, 1.0f);
    } else if (zBulletTime_IsActive()) {
        s_fCategoryVolumeFade[xSndCat_GAME_3D] += 0.5f * (1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_GAME_2D] += 0.5f * (1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_MUSIC] += 0.5f * (1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_DIALOG] += 0.5f * (1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_GAME_3D] = CLAMP(s_fCategoryVolumeFade[xSndCat_GAME_3D], 0.0f, 0.5f);
        s_fCategoryVolumeFade[xSndCat_GAME_2D] = CLAMP(s_fCategoryVolumeFade[xSndCat_GAME_2D], 0.0f, 0.5f);
        s_fCategoryVolumeFade[xSndCat_MUSIC] = CLAMP(s_fCategoryVolumeFade[xSndCat_MUSIC], 0.0f, 0.5f);
        s_fCategoryVolumeFade[xSndCat_DIALOG] = CLAMP(s_fCategoryVolumeFade[xSndCat_DIALOG], 0.0f, 0.5f);
    } else if (bDialogPlaying) {
        s_fCategoryVolumeFade[xSndCat_GAME_3D] += 0.33f * (1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_GAME_2D] += 0.33f * (1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_MUSIC] += 0.33f * (1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_GAME_3D] = CLAMP(s_fCategoryVolumeFade[xSndCat_GAME_3D], 0.0f, 0.33f);
        s_fCategoryVolumeFade[xSndCat_GAME_2D] = CLAMP(s_fCategoryVolumeFade[xSndCat_GAME_2D], 0.0f, 0.33f);
        s_fCategoryVolumeFade[xSndCat_MUSIC] = CLAMP(s_fCategoryVolumeFade[xSndCat_MUSIC], 0.0f, 0.33f);
    } else if (globals.cmgr) {
        s_fCategoryVolumeFade[xSndCat_MUSIC] += 0.275f * (1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_MUSIC] = CLAMP(s_fCategoryVolumeFade[xSndCat_MUSIC], 0.0f, 0.33f);
    } else {
        s_fCategoryVolumeFade[xSndCat_GAME_3D] += 0.33f * (-1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_GAME_2D] += 0.33f * (-1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_MUSIC] += 0.33f * (-1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_DIALOG] += 0.5f * (-1.25f * xglobals->update_dt);
        s_fCategoryVolumeFade[xSndCat_GAME_3D] = CLAMP(s_fCategoryVolumeFade[xSndCat_GAME_3D], 0.0f, 0.33f);
        s_fCategoryVolumeFade[xSndCat_GAME_2D] = CLAMP(s_fCategoryVolumeFade[xSndCat_GAME_2D], 0.0f, 0.33f);
        s_fCategoryVolumeFade[xSndCat_MUSIC] = CLAMP(s_fCategoryVolumeFade[xSndCat_MUSIC], 0.0f, 0.33f);
        s_fCategoryVolumeFade[xSndCat_DIALOG] = CLAMP(s_fCategoryVolumeFade[xSndCat_DIALOG], 0.0f, 0.5f);
    }

    update_faders(xglobals->update_dt);
}

void xSndMgrSceneExit()
{
    xSndMgrStopSounds(xSndCat_ALL, false);

    xSndMgrUpdate();

    iSndSceneExit();

    xSndMgr_SetPaused(xSndCat_ALL, false);
    xSndMgr_SetInitFlag(false);
}

void xSndMgrExit()
{
    iSndExit();
}

bool xSndMgrDoesEffects(S16)
{
    return false;
}

void xSndMgrStopSounds(S16 eSoundCategory, bool bSkipPaused)
{
    if (eSoundCategory == xSndCat_ALL) {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if ((gSnd.voice[i].flags & xSnd_ACTIVE) && (!bSkipPaused || !(gSnd.voice[i].flags & xSnd_PAUSED))) {
                xSndMgr_Stop(&gSnd.voice[i]);
            }
        }
        FSOUND_AUXFX_ShutdownEffect(0);
        reset_faders();
    }
    else if (eSoundCategory == xSndCat_ALL_BUT_MUSIC) {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if ((gSnd.voice[i].flags & xSnd_ACTIVE) && (!bSkipPaused || !(gSnd.voice[i].flags & xSnd_PAUSED)))
                if (gSnd.voice[i].category != xSndCat_MUSIC)
                    xSndMgr_Stop(&gSnd.voice[i]);
        }
    } else {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if ((gSnd.voice[i].flags & xSnd_ACTIVE) && (!bSkipPaused || !(gSnd.voice[i].flags & xSnd_PAUSED)))
                if (eSoundCategory == gSnd.voice[i].category)
                    xSndMgr_Stop(&gSnd.voice[i]);
        }
    }
}

void xSndMgrPauseSounds(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType)
{
    if (eSoundCategory == xSndCat_ALL) {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if (gSnd.voice[i].flags & xSnd_ACTIVE) {
                xSndMgr_Pause(&gSnd.voice[i], bPaused);
            }
        }
    }
    else if (eSoundCategory == xSndCat_ALL_BUT_MUSIC) {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if ((gSnd.voice[i].flags & xSnd_ACTIVE) && gSnd.voice[i].category != xSndCat_MUSIC) {
                xSndMgr_Pause(&gSnd.voice[i], bPaused);
            }
        }
    } else {
        for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
            if ((gSnd.voice[i].flags & xSnd_ACTIVE) && eSoundCategory == gSnd.voice[i].category) {
                xSndMgr_Pause(&gSnd.voice[i], bPaused);
            }
        }
    }

    xSndMgr_SetPaused(eSoundCategory, bPaused && bPauseFutureSoundsOfThisType);
}

void xSndMgrCutscenePause(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType)
{
    if (eSoundCategory == xSndCat_ALL) {
        for (S32 i = 0; i < XSND_MAX_VOICES; i++) {
            if (gSnd.voice[i].flags & xSnd_ACTIVE) {
                xSndMgr_CutscenePause(&gSnd.voice[i], bPaused);
            }
        }
    }
    else if (eSoundCategory == xSndCat_ALL_BUT_MUSIC) {
        for (S32 i = 0; i < XSND_MAX_VOICES; i++) {
            if ((gSnd.voice[i].flags & xSnd_ACTIVE) && gSnd.voice[i].category != xSndCat_MUSIC) {
                xSndMgr_CutscenePause(&gSnd.voice[i], bPaused);
            }
        }
    } else {
        for (S32 i = 0; i < XSND_MAX_VOICES; i++) {
            if ((gSnd.voice[i].flags & xSnd_ACTIVE) && eSoundCategory == gSnd.voice[i].category) {
                xSndMgr_CutscenePause(&gSnd.voice[i], bPaused);
            }
        }
    }

    xSndMgr_SetPaused(eSoundCategory, bPaused && bPauseFutureSoundsOfThisType);
}

void xSndMgrSetUserVolume(S16 eSoundCategory, F32 fVolume)
{
    switch (eSoundCategory) {
    case xSndCat_ALL:
        s_fCategoryVolume[xSndCat_MUSIC] = fVolume;
    case xSndCat_ALL_BUT_MUSIC:
        s_fCategoryVolume[xSndCat_GAME_3D] = fVolume;
        s_fCategoryVolume[xSndCat_GAME_2D] = fVolume;
        s_fCategoryVolume[xSndCat_DIALOG] = fVolume;
        s_fCategoryVolume[xSndCat_UI] = fVolume;
        s_fCategoryVolume[xSndCat_CUTSCENE] = fVolume;
        break;
    default:
        s_fCategoryVolume[eSoundCategory] = fVolume;
        break;
    }
}

F32 xSndMgrGetUserVolume(S16 eSoundCategory)
{
    return MAX(s_fCategoryVolume[eSoundCategory], 0.0f);
}

F32 xSndMgrGetDampedVolume(S16 eSoundCategory)
{
    F32 fRealVolume = s_fCategoryVolume[eSoundCategory] - s_fCategoryVolumeFade[eSoundCategory];
    return CLAMP(fRealVolume, 0.0f, 1.0f);
}

void xSndMgrStopSounds(xSndGroupHandle hActiveSound)
{
    if (hActiveSound == ISND_INVALID_SND_GROUP_HANDLE) {
        return;
    }
    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if ((gSnd.voice[i].flags & xSnd_ACTIVE) && hActiveSound == gSnd.voice[i].soundGroupHandle) {
            xSndMgr_Stop(&gSnd.voice[i]);
        }
    }
}

// UNUSED: needed to match (speculative code)
U32 xSndMgrGetGroupId(xSndGroupHandle hSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) {
        return 0;
    }
    xSndGroup* pSndGroup = xSndMgr_GetGroup(hSound);
    return pSndGroup->header.id;
}

U8 xSndMgrGetSoundCount(xSndGroupHandle hSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) {
        return 0;
    }
    xSndGroup* pSndGroup = xSndMgr_GetGroup(hSound);
    return pSndGroup->header.uInfoCount;
}

U32 xSndMgrGetSoundId(xSndGroupHandle hSound, U32 uIndexOfSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) {
        return 0;
    }
    xSndGroup* pSndGroup = xSndMgr_GetGroup(hSound);
    if (uIndexOfSound < pSndGroup->header.uInfoCount) {
        return pSndGroup->aSndGroupInfo[uIndexOfSound].uSoundNameHash;
    }
    return 0;
}

xSndGroupHandle xSndMgrGetSoundGroup(U32 uSoundGroupNameHash)
{
    if (!uSoundGroupNameHash) {
        return ISND_INVALID_SND_GROUP_HANDLE;
    }
    xSndGroupHandle retHandle = (xSndGroupHandle)xSTFindAsset(uSoundGroupNameHash, NULL);
    if (!retHandle) {
        return ISND_INVALID_SND_GROUP_HANDLE;
    }
    xSndGroup* pSndGroup = (xSndGroup*)retHandle;
    pSndGroup->header.pszGroupName = xSTAssetName(uSoundGroupNameHash);
    return retHandle;
}

bool xSndMgrIsPlaying(xSndGroupHandle hSoundGroup)
{
    if (hSoundGroup == ISND_INVALID_SND_GROUP_HANDLE) {
        return false;
    }
    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if ((gSnd.voice[i].flags & xSnd_ACTIVE) && hSoundGroup == gSnd.voice[i].soundGroupHandle)
            return true;
    }
    return false;
}

U8 xSndMgrGetPriority(xSndGroupHandle hSound, bool bStreaming, bool bLooping)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) {
        return 0;
    }
    xSndGroupHeader* pSnd = &xSndMgr_GetGroup(hSound)->header;
    U32 uPriority = pSnd->uPriority;
    if (bStreaming) {
        uPriority = 224;
    }
    uPriority += pSnd->eSoundCategory;
    if (bLooping) {
        uPriority += 10;
    }
    if (uPriority > 255) {
        uPriority = 255;
    }
    return uPriority;
}

xSndVoiceInfo* xSndGetFreeVoiceInfo(const xSndGroupHeader* pSnd, U32 iFlags)
{
    F32 dstFurthest = 0.0f;
    S32 idxFurthest = 0;
    U32 ageOldest = 0xFFFFFFFF;
    S32 idxOldest = 0;

    if (pSnd->nMaxPlays > 0) {
        U8 uInstanceCount = 0;
        for (S32 i = 0; i < XSND_MAX_VOICES; i++) {
            if (gSnd.voice[i].flags & xSnd_ACTIVE) {
                if (pSnd->id == xSndMgr_GetGroup(gSnd.voice[i].soundGroupHandle)->header.id) {
                    uInstanceCount++;
                    if (pSnd->eSoundCategory == xSndCat_GAME_3D) {
                        if (gSnd.voice[i].distToListener > dstFurthest) {
                            dstFurthest = gSnd.voice[i].distToListener;
                            idxFurthest = i;
                        }
                        if (uInstanceCount == pSnd->nMaxPlays) {
                            xSndMgrStop(gSnd.voice[idxFurthest].sndUniqueID);
                            return &gSnd.voice[idxFurthest];
                        }
                    } else {
                        if (gSnd.voice[i].sndUniqueID < ageOldest) {
                            ageOldest = gSnd.voice[i].sndUniqueID;
                            idxOldest = i;
                        }
                        if (uInstanceCount == pSnd->nMaxPlays) {
                            xSndMgrStop(gSnd.voice[idxOldest].sndUniqueID);
                            return &gSnd.voice[idxOldest];
                        }
                    }
                }
            }
        }
    }

    xSndVoiceInfo* vp = iSndGetFreeVoiceInfo(pSnd->uPriority, iFlags);
    if (!vp) {
        return NULL;
    }
    return vp;
}

iSndHandle xSndMgrPlay(xSndGroupHandle hSoundToPlay, U32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, const F32* pfOverrideVolume, const F32* pfOverridePitch)
{
    if (hSoundToPlay == ISND_INVALID_SND_GROUP_HANDLE) {
        return XSNDMGR_INVALID_SND_HANDLE;
    }

    if (uFlags & xSnd_UNIQUE) {
        if (xSndMgrIsPlaying(hSoundToPlay)) {
            return XSNDMGR_INVALID_SND_HANDLE;
        }
    }

    U32 iFlags = uFlags & ~0x3FF;
    xSndGroupHeader* pSnd = &xSndMgr_GetGroup(hSoundToPlay)->header;
    U32 uInfoIndex = xrand_RandomChoice(pSnd->uPlayedMask, pSnd->uSetBits, pSnd->uInfoCount, (xRandomChoiceRule)pSnd->ePlayRule);
    xSndGroupInfo* pSndInfo = &((xSndGroup*)pSnd)->aSndGroupInfo[uInfoIndex];
    pSnd->uLastSelected = uInfoIndex;

    iSndFileInfo* ip = iSndLookup(pSndInfo->uSoundNameHash);

    if (uFlags & xSnd_CUTSCENE) {
        iFlags |= xSnd_STREAMED;
    } else {
        if (!ip) {
            return XSNDMGR_INVALID_SND_HANDLE;
        }

        if (!ip->isStreamed())
            iFlags |= xSnd_SAMPLE;
        else
            iFlags |= xSnd_STREAMED;

        if (xSndMgrDoesEffects(pSnd->eSoundCategory) && !(uFlags & xSnd_NOEFFECTS)) {
            iFlags |= xSnd_EFFECTS;
        }
    }

    if (iFlags & xSnd_STREAMED) {
        if ((pSnd->uFlags & 0x1) || (uFlags & xSnd_LOOP)) {
            iFlags |= xSnd_LOOP;
        }
    }

    if (globals.cmgr && ip && !ip->isLooping() && globals.cmgr->csn && globals.cmgr->csn->Ready && pSnd->eSoundCategory != xSndCat_CUTSCENE) {
        return XSNDMGR_INVALID_SND_HANDLE;
    }

    if (pSnd->eSoundCategory == xSndCat_CUTSCENE) {
        xSndMgrStopSounds(xSndCat_DIALOG, false);
    }
    else if (pSnd->eSoundCategory == xSndCat_DIALOG) {
        xSndMgrStopSounds(xSndCat_DIALOG, true);
    }

    xSndVoiceInfo* vp = xSndGetFreeVoiceInfo(pSnd, iFlags);
    if (!vp) {
        return XSNDMGR_INVALID_SND_HANDLE;
    }

    vp->priority = ip ? xSndMgrGetPriority(hSoundToPlay, ip->isStreamed(), ip->isLooping()) : XSND_DEFAULT_PRIORITY;
    vp->assetVol = pSndInfo->fVolume;
    vp->assetID = pSndInfo->uSoundNameHash;
    vp->pSndFileInfo = ip;
    vp->dopplerFreqCoef = 1.0f;
    vp->flags = iFlags | xSnd_ACTIVE;
    vp->category = pSnd->eSoundCategory;
    vp->sndUniqueID = (iSndHandle)++gSnd.SndCount;
    vp->soundGroupHandle = xSndMgr_GetHandle((xSndGroup*)pSnd);

    if (!pfOverridePitch) {
        vp->xpitch = xrand_RandomFloatRange(pSndInfo->fMinPitchMult, pSndInfo->fMaxPitchMult);
    } else {
        vp->xpitch = *pfOverridePitch;
        if (iFlags & xSnd_RELATIVEPITCH) {
            vp->xpitch += xrand_RandomFloatRange(pSndInfo->fMinPitchMult, pSndInfo->fMaxPitchMult);
        }
    }

    if (!pfOverrideVolume) {
        vp->vol = pSndInfo->fVolume;
    } else {
        vp->vol = *pfOverrideVolume;
        if (iFlags & xSnd_RELATIVEVOLUME) {
            vp->vol *= pSndInfo->fVolume;
        }
    }

    if (vp->category == xSndCat_GAME_3D) {
        if (pParentEnt) {
            if (pParentEnt == ZPLAYER) {
                vp->flags &= ~xSnd_CALC3D;
            } else {
                vp->flags |= xSnd_CALC3D;
            }
            vp->pParent = pParentEnt;
            vp->pPosition = NULL;
            vp->actualPos = *xEntGetPos(pParentEnt);
            vp->pVelocity = NULL;
            if (vp->flags & xSnd_DOPPLER) {
                vp->actualVelocity = xglobals->update_dt > 0.01f ? (pParentEnt->frame->mat.pos - pParentEnt->frame->oldmat.pos) / xglobals->update_dt : xVec3::m_Null;
            } else {
                vp->actualVelocity.assign(0.0f);
            }

            vp->innerRadius = pSnd->fInnerRadius;
            vp->outerRadius = pSnd->fOuterRadius <= 0.0f ? 100000.0f : pSnd->fOuterRadius;
        }
        else if (pPos_WS) {
            vp->flags |= xSnd_CALC3D;
            vp->pParent = NULL;
            vp->pPosition = pPos_WS;
            vp->actualPos = *pPos_WS;

            vp->innerRadius = pSnd->fInnerRadius;
            vp->outerRadius = pSnd->fOuterRadius <= 0.0f ? 100000.0f : pSnd->fOuterRadius;

            if (pVel) {
                vp->pVelocity = pVel;
                vp->actualVelocity = *pVel;
            } else {
                vp->pVelocity = NULL;
                vp->actualVelocity.assign(0.0f);
            }
        } else {
            vp->pParent = NULL;
            vp->pPosition = NULL;
            vp->actualPos = gSnd.pos;
            vp->actualVelocity.assign(0.0f);
            vp->pVelocity = NULL;
            vp->innerRadius = pSnd->fInnerRadius;
            vp->outerRadius = pSnd->fOuterRadius;
        }
    } else {
        vp->pParent = NULL;
        vp->pPosition = NULL;
        vp->actualPos = gSnd.pos;
        vp->actualVelocity.assign(0.0f);
        vp->pVelocity = NULL;
        vp->innerRadius = pSnd->fInnerRadius;
        vp->outerRadius = pSnd->fOuterRadius;
    }

    bool bOKtoPlay = true;
    if (vp->flags & xSnd_CALC3D) {
        bOKtoPlay = xSndMgr_UpdateVoicePosition(vp);
    }

    bool bPlaying = vp->pSndFileInfo && vp->pSndFileInfo->isLooping();

    if (bOKtoPlay) {
        bPlaying = iSndPlay(vp);
        if (xSndMgrIsPaused(vp->category))
            xSndMgrPause(vp->sndUniqueID, true);
    }

    if (bPlaying) {
    } else {
        vp->flags = 0;
        return XSNDMGR_INVALID_SND_HANDLE;
    }

    return vp->sndUniqueID;
}

U32 xSndMgrGetSoundId(iSndHandle hSound)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        return 0;
    }
    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if ((gSnd.voice[i].flags & xSnd_ACTIVE) && hSound == gSnd.voice[i].sndUniqueID) {
            return gSnd.voice[i].assetID;
        }
    }
    return 0;
}

bool xSndMgrIsPlaying(iSndHandle hSound)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        return false;
    }
    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if ((gSnd.voice[i].flags & xSnd_ACTIVE) && hSound == gSnd.voice[i].sndUniqueID) {
            return true;
        }
    }
    return false;
}

bool xSndMgrIsLooping(iSndHandle hSound)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        return false;
    }
    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if (hSound == gSnd.voice[i].sndUniqueID) {
            return gSnd.voice[i].pSndFileInfo->isLooping();
        }
    }
    return false;
}

U32 xSndMgrIsReady(iSndHandle hSound)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        return FALSE;
    }
    return iSndIsReady(hSound);
}

void xSndMgrPause(iSndHandle hSound, bool bPause)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        return;
    }
    S32 i;
    for (i = 0; i < XSND_MAX_VOICES; i++) {
        if ((gSnd.voice[i].flags & xSnd_ACTIVE) && hSound == gSnd.voice[i].sndUniqueID) {
            xSndMgr_Pause(&gSnd.voice[i], bPause);
        }
    }
    if (i == XSND_MAX_VOICES) {
        for (; i < XSND_TOTAL_VOICES; i++) {
        }
    }
}

void xSndMgrStop(iSndHandle& hSound)
{
    iSndHandle hInHandle = hSound;
    if (hInHandle == XSNDMGR_INVALID_SND_HANDLE) {
        return;
    }
    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if ((gSnd.voice[i].flags & xSnd_ACTIVE) && hInHandle == gSnd.voice[i].sndUniqueID) {
            xSndMgr_Stop(&gSnd.voice[i]);
            break;
        }
    }
    hSound = XSNDMGR_INVALID_SND_HANDLE;
}

void xSndMgrSetVolume(iSndHandle hSound, F32 fVolume, bool assetRelative)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        return;
    }

    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if ((gSnd.voice[i].flags & xSnd_ACTIVE) && hSound == gSnd.voice[i].sndUniqueID) {
            F32 vol = fVolume;
            if (assetRelative) {
                vol *= gSnd.voice[i].assetVol;
            }

            if (i < XSND_MAX_VOICES) {
                iSndSetVol(&gSnd.voice[i], vol);
            }
            gSnd.voice[i].vol = vol;
        }
    }
}

void xSndMgrSetPitch(iSndHandle hSound, F32 fPitch)
{
    if (hSound == XSNDMGR_INVALID_SND_HANDLE) {
        return;
    }

    for (S32 i = 0; i < XSND_TOTAL_VOICES; i++) {
        if ((gSnd.voice[i].flags & xSnd_ACTIVE) && hSound == gSnd.voice[i].sndUniqueID) {
            if (i < XSND_MAX_VOICES) {
                iSndSetPitch(&gSnd.voice[i], fPitch);
            }
            gSnd.voice[i].xpitch = fPitch;
        }
    }
}

void xSndMgrStartStereo(iSndHandle hSnd00, iSndHandle hSnd01)
{
    iSndStartStereo(hSnd00, hSnd01);
}

void xSndMgrStopChildren(xEnt* pParentEnt)
{
    U32 i;
    xSndVoiceInfo* vp = gSnd.voice;
    for (i = 0; i < XSND_TOTAL_VOICES; i++, vp++) {
        if ((vp->flags & xSnd_ACTIVE) && vp->pParent == pParentEnt) {
            xSndMgr_Stop(vp);
        }
    }
}

iSndHandle xSndMgrFadeOn(xSndGroupHandle hSoundToPlay, F32 fadeTime, U32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, const F32* pfOverrideVolume, const F32* pfOverridePitch)
{
    if (fadeTime <= 0.0f) {
        return xSndMgrPlay(hSoundToPlay, uFlags, pPos_WS, pVel, pParentEnt, pfOverrideVolume, pfOverridePitch);
    }

    F32 zero = 0.0f;
    iSndHandle sndUniqueID = xSndMgrPlay(hSoundToPlay, uFlags, pPos_WS, pVel, pParentEnt, &zero, pfOverridePitch);
    if (sndUniqueID == XSNDMGR_INVALID_SND_HANDLE) {
        return XSNDMGR_INVALID_SND_HANDLE;
    }

    faders.push_back();
    sound_fader* fader = &faders.back();
    fader->handle = sndUniqueID;
    fader->volume = 0.0f;
    U8 sound_index = xSndMgr_GetGroup(hSoundToPlay)->header.uLastSelected;
    fader->end = pfOverrideVolume ? *pfOverrideVolume : xSndMgr_GetGroup(hSoundToPlay)->aSndGroupInfo[sound_index].fVolume;
    fader->rate = fader->end / fadeTime;
    return fader->handle;
}

void xSndMgrFadeOff(iSndHandle& hSound, F32 fadeTime)
{
    iSndHandle hInHandle = hSound;
    if (hInHandle == XSNDMGR_INVALID_SND_HANDLE) {
        return;
    }

    sound_fader* fader = find_fader(hSound);
    xSndVoiceInfo* vp;
    F32 start;

    if (fadeTime <= 0.0f) {
        goto stop;
    }

    vp = find_voice(hInHandle);
    if (!vp) {
        goto stop;
    }

    if (!fader) {
        faders.push_back();
        fader = &faders.back();
        start = 1.0f;
    } else {
        start = fader->end;
    }

    if (start <= 0.001f) {
        goto stop;
    }

    fader->handle = hInHandle;
    fader->volume = vp->vol;
    fader->end = 0.0f;
    fader->rate = -start / fadeTime;
    hSound = XSNDMGR_INVALID_SND_HANDLE;
    return;

stop:
    if (fader) {
        faders.erase(fader);
    }
    if (hSound != XSNDMGR_INVALID_SND_HANDLE) {
        xSndMgrStop(hSound);
    }
}

U32 gCurrSoundMemoryUsed;
U32 gHighwaterSoundMemoryUsed;

inline void xSndMgr_SetPaused(S16 whichCat, bool bPause)
{
    switch (whichCat) {
    case xSndCat_ALL:
        bPause ? (gSnd.m_uSndMgrFlags |= k_XSNDMGR_PAUSED_ALL) : (gSnd.m_uSndMgrFlags &= ~k_XSNDMGR_PAUSED_ALL);
        break;
    case xSndCat_ALL_BUT_MUSIC:
        bPause ? (gSnd.m_uSndMgrFlags |= k_XSNDMGR_PAUSED_ALL_BUT_MUSIC) : (gSnd.m_uSndMgrFlags &= ~k_XSNDMGR_PAUSED_ALL_BUT_MUSIC);
        break;
    case xSndCat_GAME_3D:
        bPause ? (gSnd.m_uSndMgrFlags |= k_XSNDMGR_PAUSED_GAME_3D) : (gSnd.m_uSndMgrFlags &= ~k_XSNDMGR_PAUSED_GAME_3D);
        break;
    case xSndCat_GAME_2D:
        bPause ? (gSnd.m_uSndMgrFlags |= k_XSNDMGR_PAUSED_GAME_2D) : (gSnd.m_uSndMgrFlags &= ~k_XSNDMGR_PAUSED_GAME_2D);
        break;
    case xSndCat_DIALOG:
        bPause ? (gSnd.m_uSndMgrFlags |= k_XSNDMGR_PAUSED_DIALOG) : (gSnd.m_uSndMgrFlags &= ~k_XSNDMGR_PAUSED_DIALOG);
        break;
    case xSndCat_MUSIC:
        bPause ? (gSnd.m_uSndMgrFlags |= k_XSNDMGR_PAUSED_MUSIC) : (gSnd.m_uSndMgrFlags &= ~k_XSNDMGR_PAUSED_MUSIC);
        break;
    case xSndCat_CUTSCENE:
        bPause ? (gSnd.m_uSndMgrFlags |= k_XSNDMGR_PAUSED_CUTSCENE) : (gSnd.m_uSndMgrFlags &= ~k_XSNDMGR_PAUSED_CUTSCENE);
        break;
    case xSndCat_UI:
        bPause ? (gSnd.m_uSndMgrFlags |= k_XSNDMGR_PAUSED_UI) : (gSnd.m_uSndMgrFlags &= ~k_XSNDMGR_PAUSED_UI);
        break;
    }
}

bool xSndMgrIsPaused(S16 eSoundCategory)
{
    switch (eSoundCategory) {
    case xSndCat_ALL:
        return gSnd.m_uSndMgrFlags & k_XSNDMGR_PAUSED_ALL;
    case xSndCat_ALL_BUT_MUSIC:
        return gSnd.m_uSndMgrFlags & k_XSNDMGR_PAUSED_ALL_BUT_MUSIC;
    case xSndCat_GAME_3D:
        return gSnd.m_uSndMgrFlags & k_XSNDMGR_PAUSED_GAME_3D;
    case xSndCat_GAME_2D:
        return gSnd.m_uSndMgrFlags & k_XSNDMGR_PAUSED_GAME_2D;
    case xSndCat_DIALOG:
        return gSnd.m_uSndMgrFlags & k_XSNDMGR_PAUSED_DIALOG;
    case xSndCat_MUSIC:
        return gSnd.m_uSndMgrFlags & k_XSNDMGR_PAUSED_MUSIC;
    case xSndCat_CUTSCENE:
        return gSnd.m_uSndMgrFlags & k_XSNDMGR_PAUSED_CUTSCENE;
    case xSndCat_UI:
        return gSnd.m_uSndMgrFlags & k_XSNDMGR_PAUSED_UI;
    }
    return false;
}
