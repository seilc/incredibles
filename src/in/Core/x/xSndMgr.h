#ifndef XSNDMGR_H
#define XSNDMGR_H

#include "iSnd.h"
#include "xEnt.h"

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
    U8 uInfoPad0;
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

void xSndMgrSetEffect(xSndEffect Effect);
xSndEffect xSndMgrGetEffect();
void xSndMgrUpdate();
void xSndMgrStopSounds(S16 eSoundCategory, bool bSkipPaused);
void xSndMgrPauseSounds(S16 eSoundCategory, bool bPaused, bool bPauseFutureSoundsOfThisType);
F32 xSndMgrGetDampedVolume(S16 eSoundCategory);
iSndGroupHandle xSndMgrGetSoundGroup(U32 uSoundGroupNameHash);
iSndHandle xSndMgrPlay(iSndGroupHandle hSoundToPlay, U32 uFlags, const xVec3* pPos_WS = NULL,
                       const xVec3* pVel = NULL, xEnt* pParentEnt = NULL,
                       const F32* pfOverrideVolume = NULL,
                       const F32* pfOverridePitch = NULL);
void xSndMgrStop(iSndHandle& hSound);
xSndGroup* xSndMgr_GetGroup(iSndGroupHandle hSound);

template <S32 unknown>
class sound_queue
{
public:
    sound_queue()
    {
        head = 0;
        tail = 0;
    }

    void play(iSndGroupHandle hSoundToPlay, U32 uFlags, const xVec3* pPos_WS, const xVec3* pVel, xEnt* pParentEnt, F32* pfOverrideVolume, F32* pfOverridePitch)
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

inline bool xSndMgrIsEnvironmentalStream(iSndGroupHandle hSound)
{
    if (hSound == ISND_INVALID_SND_GROUP_HANDLE)
    {
        return false;
    }

    xSndGroup* pSndGroup = xSndMgr_GetGroup(hSound);

    return pSndGroup->header.uFlags & 0x2;
}

#endif