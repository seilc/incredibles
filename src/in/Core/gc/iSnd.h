#ifndef ISND_H
#define ISND_H

#include "xMath.h"

#include <fmod.h>

struct xSndVoiceInfo;

#define ISND_MAX_VOICES (48)
#define ISND_STATIC_BUFFER_SIZE (640 * 1024)

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
void iSndInitSceneLoaded();
void iSndSceneExit();
void iSndExit();
iSndFileInfo* iSndLookup(U32 id);
bool initBytesToConsume(FSOUND_STREAM* stream);
bool releaseBytesToConsume(FSOUND_STREAM* stream);
S32 getBytesToConsume(FSOUND_STREAM* stream);
S32 getFrontBuffer(FSOUND_STREAM* stream);
void* getDataPtr(FSOUND_STREAM* stream);
bool setBytesToConsume(FSOUND_STREAM* stream, S32 bytesToConsume);
bool setFrontBuffer(FSOUND_STREAM* stream, S32 frontBuffer);
bool setDataPtr(FSOUND_STREAM* stream, void* pData);
S8 iSndExternalCB(FSOUND_STREAM* stream, void* buff, S32 len, void* userdata);
bool iSndPlay(xSndVoiceInfo* vp);
void iSndStartStereo(iSndHandle, iSndHandle);
void iSndSetMode();
void iSndSetEnvironmentalEffect(S32 effectType);
xSndVoiceInfo* iSndGetFreeVoiceInfo(U32 priority, U32 flags);
void iSndUpdate();
bool iSndIsReady(iSndHandle);
void iSndStop(xSndVoiceInfo* vp);
void iSndPause(xSndVoiceInfo* vp, U32 paused);
void iSndSetVol(xSndVoiceInfo* vp, F32 xVolume);
void iSndSetPitch(xSndVoiceInfo* pVoice, F32 xpitch);
void iSndKill();
void iSndSetExternalCallback(void*);
void iSndSuspendCD(U32);
void iSndSuspend();
void iSndResume();

inline U8 iSndVolFromX(F32 fVolume)
{
    U8 vol;
    fVolume = CLAMP(fVolume, -1.0f, 1.0f);
    vol = 255.0f * fVolume;
    return vol;
}

#endif
