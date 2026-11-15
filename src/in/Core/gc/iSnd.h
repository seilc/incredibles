#ifndef ISND_H
#define ISND_H

#include "types.h"

#include <fmod.h>

struct xSndVoiceInfo;

#define ISND_MAX_VOICES (48)

enum iSndHandle
{
};

enum iSndGroupHandle
{
};

#define ISNDHANDLE_INVALID ((iSndHandle)-1)
#define ISND_INVALID_SND_GROUP_HANDLE ((iSndGroupHandle)0xFFFFFFFF)

struct iSndInfo
{
    FSOUND_STREAM* pStream;
    S32 nChannel;
};

struct gcWavInfo
{
    U32 uAssetID;
    U8 uFlags;
    U8 uAudioSampleIndex;
    U8 uFSBIndex;
    U8 uSoundInfoIndex;
};

struct iSndFileInfo : gcWavInfo
{
    bool isStreamed() { return (uFlags >> 1) & 0x1; }
    bool isLooping() { return uFlags & 0x1; }
};

void iSndInit();
void iSndSceneExit();
void iSndExit();
void iSndUpdate();
void iSndKill();
void iSndSuspendCD(U32);
void iSndSuspend();
void iSndResume();
void iSndSetEnvironmentalEffect(S32 effectType);
bool iSndPlay(xSndVoiceInfo* vp);
xSndVoiceInfo* iSndGetFreeVoiceInfo(U32 priority, U32 flags);
void iSndStop(xSndVoiceInfo* vp);
void iSndPause(xSndVoiceInfo* vp, U32 paused);
void iSndSetVol(xSndVoiceInfo* vp, F32 xVolume);
void iSndSetPitch(xSndVoiceInfo* pVoice, F32 xpitch);
void iSndStartStereo(iSndHandle, iSndHandle);
iSndFileInfo* iSndLookup(U32 id);
bool iSndIsReady(iSndHandle);
void iSndSetExternalCallback(void*);

#endif
