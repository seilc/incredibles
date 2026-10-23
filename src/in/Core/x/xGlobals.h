#ifndef XGLOBALS_H
#define XGLOBALS_H

#include "xMath3.h"
#include "xPad.h"
#include "xUpdateCull.h"
#include "iFog.h"
#include "iTime.h"

#include "zPlayer.h"
#include "zScene.h"

#include <types.h>
#include <rwcore.h>
#include <rpworld.h>

struct xCamGroup;
struct xCamScreen;

enum sceDemoEndReason
{
    SCE_DEMO_ENDREASON_ATTRACT_INTERRUPTED,
    SCE_DEMO_ENDREASON_ATTRACT_COMPLETE,
    SCE_DEMO_ENDREASON_PLAYABLE_INACTIVITY_TIMEOUT,
    SCE_DEMO_ENDREASON_PLAYABLE_GAMEPLAY_TIMEOUT,
    SCE_DEMO_ENDREASON_PLAYABLE_COMPLETE,
    SCE_DEMO_ENDREASON_PLAYABLE_QUIT,
    SCE_DEMO_ENDREASON_NETCONFIG_REQUEST,
    SCE_DEMO_ENDREASON_NETCONFIG_COMPLETE
};

struct PS2DemoGlobals
{
    U16 language;
    U16 aspect;
    U16 play_mode;
    U16 inactive_timeout;
    U16 gameplay_timeout;
    sceDemoEndReason exit_code;
    struct
    {
        U32 FMV_playing : 1;
        U32 more_padding : 31;
    };
    F32 bail_timer;
    F32 inactive_timer;
    F32 gameplay_timer;
    char subdir[16];
    U16 quit;
    U16 vmode;
};

struct xGlobals
{
    xCamGroup* cam;
    xCamScreen* screen;
    xVec4 frustplane[12];
    xPad* pad0;
    xPad* pad1;
    xPad* pad2;
    xPad* pad3;
    //xPad* pad[4]; removed in ship
    S32 profile;
    char profFunc[128][6];
    xUpdateCullMgr* updateMgr;
    S32 sceneFirst;
    char sceneStart[32];
    RpWorld* currWorld;
    iFogParams fog;
    iFogParams fogA;
    iFogParams fogB;
    iTime fog_t0;
    iTime fog_t1;
    S32 option_vibration;
    S32 option_subtitles;
    U32 slowdown;
    F32 update_dt;
    S16 ForceCinematic;
    S32 useHIPHOP;
    U8 NoMusic;
    U8 NoCutscenes;
    U8 NoPadCheck;
    S8 currentActivePad;
    U8 firstStartPressed;
    U8 fromLauncher;
    U8 FlashWIP;
    U8 inLoadingScreen;
    U8 LoadingScene;
    U8 InitializingLoadingScreen;
    U8 ForceMono;
    U32 minVSyncCnt;
    U8 dontShowPadMessageDuringLoadingOrCutScene;
    U8 autoSaveFeature;
    S32 asyncLoadingScreen;
    S32 asyncLoadingFlags;
    char fromLauncherUser[32];
    zPlayer* ___player_ent_dont_use_directly;
    zScene* sceneCur;
    zScene* scenePreload;
    PS2DemoGlobals* PS2demo;
    char watermark[127];
    U8 watermarkAlpha;
    F32 watermarkSize;
};

extern xGlobals* xglobals;

#endif