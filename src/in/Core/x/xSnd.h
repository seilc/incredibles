#ifndef XSND_H
#define XSND_H

#include "iSnd.h"
#include "xDebugTweak.h"
#include "xMath3.h"

struct xEnt;
struct xSndGroup;

typedef iSndGroupHandle xSndGroupHandle;

#define XSND_MAX_VOICES (ISND_MAX_VOICES)
#define XSND_MAX_VIRTUAL_VOICES (72)
#define XSND_TOTAL_VOICES (XSND_MAX_VOICES +XSND_MAX_VIRTUAL_VOICES )

// xSndVoiceInfo::flags
enum
{
    xSnd_ACTIVE = 0x1,
    xSnd_CALC3D = 0x2,
    xSnd_INAUDIBLE = 0x8,
    xSnd_SAMPLE = 0x10,
    xSnd_EFFECTS = 0x20,
    xSnd_STREAMED = 0x40,
    xSnd_PAUSED = 0x200,
    xSnd_CUTSCENE = 0x400,
    xSnd_FOLLOW = 0x800,
    xSnd_NOEFFECTS = 0x2000,
    xSnd_LOOP = 0x200000,
    xSnd_DOPPLER = 0x400000,
    xSnd_UNIQUE = 0x800000,
    xSnd_RELATIVEVOLUME = 0x1000000,
    xSnd_RELATIVEPITCH = 0x2000000
};

struct xSndVoiceInfo
{
    union
    {
        xSndGroup* soundGroup;
        xSndGroupHandle soundGroupHandle;
    };
    U32 assetID;
    iSndHandle sndUniqueID;
    xEnt* pParent;
    const xVec3* pPosition;
    U32 flags;
    F32 vol;
    F32 assetVol;
    F32 xpitch;
    F32 dopplerFreqCoef;
    S16 category;
    U8 bformerlyAPartOfPriority;
    U8 priority;
    xVec3 actualPos;
    xVec3 playPos;
    const xVec3* pVelocity;
    xVec3 actualVelocity;
    F32 distToListener;
    F32 innerRadius;
    F32 outerRadius;
    iSndInfo ps;
    iSndFileInfo* pSndFileInfo;
};

enum xSndMode
{
    xSndMode_Mono,
    xSndMode_Stereo,
    xSndMode_Dolby,
    xSndMode_Count
};

enum xSndListener
{
    xSndListener_CAMERA,
    xSndListener_PLAYER,
    xSndListener_MAX_TYPES
};

struct xSndGlobals
{
    static xTWEAKABLE F32 speedOfSound;

    xSndVoiceInfo voice[XSND_TOTAL_VOICES];
    xMat4x3 listenerMat[2];
    xVec3 listenerVel[2];
    xVec3 right;
    xVec3 up;
    xVec3 at;
    xVec3 pos;
    xVec3 velocity;
    U32 m_uSndMgrFlags;
    xSndMode sndMode;
    U32 SndCount;
    xSndListener listenerMode;
};

extern xSndGlobals gSnd;

#endif
