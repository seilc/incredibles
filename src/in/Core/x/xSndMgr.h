#ifndef XSNDMGR_H
#define XSNDMGR_H

#include "iSnd.h"
#include "xEnt.h"
#include "xMath3.h"
#include "xSnd.h"

#define XSNDMGR_INVALID_SND_HANDLE ((iSndHandle)0xFFFFFFFF)

enum
{
    xSndCat_ALL = -3,
    xSndCat_ALL_BUT_MUSIC = -2,
    xSndCat_GAME_3D = 0,
    xSndCat_GAME_2D,
    xSndCat_DIALOG,
    xSndCat_MUSIC,
    xSndCat_CUTSCENE,
    xSndCat_UI,
    xSndCat_FIRST_CATEGORY = xSndCat_GAME_3D,
    xSndCat_LAST_CATEGORY = xSndCat_UI
};

enum xSndEffect
{
    xSndEffect_NONE,
    xSndEffect_CAVE,
    xSndEffect_MAX_TYPES
};

struct xSndGroupHeader : xBaseAsset
{
    U32 uPlayedMask;
    U8 uInfoCount;
    U8 uSetBits;
    S8 nMaxPlays;
    U8 uPriority;
    U8 uFlags;
    U8 eSoundCategory;
    U8 ePlayRule;
    U8 uLastSelected;
    F32 fInnerRadius;
    F32 fOuterRadius;
    const char* pszGroupName;
};

struct xSndGroupInfo
{
    U32 uSoundNameHash;
    F32 fVolume;
    F32 fMinPitchMult;
    F32 fMaxPitchMult;
};

struct xSndGroup
{
    xSndGroupHeader header;
    xSndGroupInfo aSndGroupInfo[0];
};

extern xSndEffect m_eCurrentEffect;
extern U32 gCurrSoundMemoryUsed;
extern U32 gHighwaterSoundMemoryUsed;

xSndGroup* xSndMgr_AllocGroup(U8 nSoundAssets);
void xSndMgrInit();
void xSndMgrSelectListenerMode(xSndListener Mode);
void xSndMgrSetEffect(xSndEffect Effect);
xSndEffect xSndMgrGetEffect();
void xSndMgrProcessSoundPosition(const xVec3* pActual, xVec3* pProcessed);
void xSndMgr_UpdateListenerPosition();
bool xSndMgr_UpdateVoicePosition(xSndVoiceInfo* pVoice);
xSndVoiceInfo* xSndMgr_GetFreeVirtualVoiceInfo(U32 priority, U32 flags);
void xSndMgr_Stop(xSndVoiceInfo* pVoice);
void xSndMgr_Pause(xSndVoiceInfo* pVoice, bool bPaused);
void xSndMgr_CutscenePause(xSndVoiceInfo* pVoice, bool bPaused);
bool xSndMgr_SetInaudible(xSndVoiceInfo* pVoice);
bool xSndMgr_SetAudible(xSndVoiceInfo* pVoiceStorage);
void xSndMgrSceneInit();
void xSndMgrUpdate();
void xSndMgrSceneExit();
void xSndMgrExit();
bool xSndMgrDoesEffects(S16 eSoundCategory);
void xSndMgrStopSounds(S16 eSoundCategory, bool bSkipPaused);
void xSndMgrPauseSounds(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType);
void xSndMgrCutscenePause(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType);
void xSndMgrSetUserVolume(S16 eSoundCategory, F32 fVolume);
F32 xSndMgrGetUserVolume(S16 eSoundCategory);
F32 xSndMgrGetDampedVolume(S16 eSoundCategory);
void xSndMgrStopSounds(xSndGroupHandle hActiveSound);
U32 xSndMgrGetGroupId(xSndGroupHandle hSound); // UNUSED
U8 xSndMgrGetSoundCount(xSndGroupHandle hSound);
U32 xSndMgrGetSoundId(xSndGroupHandle hSound, U32 uIndexOfSound);
xSndGroupHandle xSndMgrGetSoundGroup(U32 uSoundGroupNameHash);
bool xSndMgrIsPlaying(xSndGroupHandle hSoundGroup);
U8 xSndMgrGetPriority(xSndGroupHandle hSound, bool bStreaming, bool bLooping);
xSndVoiceInfo* xSndGetFreeVoiceInfo(const xSndGroupHeader* pSnd, U32 iFlags);
iSndHandle xSndMgrPlay(xSndGroupHandle hSoundToPlay, U32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, const F32* pfOverrideVolume, const F32* pfOverridePitch);
U32 xSndMgrGetSoundId(iSndHandle hSound);
bool xSndMgrIsPlaying(iSndHandle hSound);
bool xSndMgrIsLooping(iSndHandle hSound);
U32 xSndMgrIsReady(iSndHandle hSound);
void xSndMgrPause(iSndHandle hSound, bool bPause);
void xSndMgrStop(iSndHandle& hSound);
void xSndMgrSetVolume(iSndHandle hSound, F32 fVolume, bool assetRelative);
void xSndMgrSetPitch(iSndHandle hSound, F32 fPitch);
void xSndMgrStartStereo(iSndHandle hSnd00, iSndHandle hSnd01);
void xSndMgrStopChildren(xEnt* pParentEnt);
iSndHandle xSndMgrFadeOn(xSndGroupHandle hSoundToPlay, F32 fadeTime, U32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, const F32* pfOverrideVolume, const F32* pfOverridePitch);
void xSndMgrFadeOff(iSndHandle& hSound, F32 fadeTime);
void xSndMgr_SetPaused(S16 whichCat, bool bPause);
bool xSndMgrIsPaused(S16 eSoundCategory);

inline xSndGroup* xSndMgr_GetGroup(xSndGroupHandle hHandle) { return (xSndGroup*)hHandle; }
inline xSndGroupHandle xSndMgr_GetHandle(xSndGroup* pSnd) { return (xSndGroupHandle)pSnd; }

inline bool xSndMgrIsDialogPlaying()
{
    return (gSnd.m_uSndMgrFlags >> 1) & 0x1;
}

inline bool xSndMgrIsEnvironmentalStream(xSndGroupHandle hSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE) return false;

    xSndGroup* pSndGroup = xSndMgr_GetGroup(hSound);

    return pSndGroup->header.uFlags & 0x2;
}

template <S32 unknown>
class sound_queue
{
public:
    sound_queue() { head = 0; tail = 0; }

    void play(xSndGroupHandle hSoundToPlay, U32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, F32* pfOverrideVolume, F32* pfOverridePitch)
    {
        iSndHandle handle = xSndMgrPlay(hSoundToPlay, uFlags, pPos_WS, pVel, pParentEnt, pfOverrideVolume, pfOverridePitch);
        push(handle);
    }

    void push(iSndHandle handle)
    {
        _playing[tail] = handle;

        S32 begin = head;
        S32 end = tail+1;
        if (end <= begin) end += unknown+1;

        S32 len = end - begin;
        if (len > unknown) {
            xSndMgrStop(_playing[begin]);
            begin++;
            head = begin % (unknown+1);
        }

        tail = end % (unknown+1);
    }

private:
    iSndHandle _playing[unknown+1];
    S32 head;
    S32 tail;
};

#endif
