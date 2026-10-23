#ifndef ZGLOBALS_H
#define ZGLOBALS_H

#include "xGlobals.h"
#include "xSndMgr.h"
#include "zCutsceneMgr.h"

struct zGrapplePoint;
struct zAssetPickupTable;

struct zGlobalSettings
{
    U16 AnalogMin;
    U16 AnalogMax;
    U32 TakeDamage;
    U32 Initial_Specials;
    F32 DamageInvincibility;
    F32 Gravity;
    U8 AttractModeDuringGameplay;
};

struct zCheckPoint
{
    xVec3 pos;
    F32 rot;
    U32 initCamID;
    bool* jsp_active;
    xSndEffect currentEffect;
};

struct zPlayerGlobals
{
    zGlobalSettings g;
    S32 Teeter;
    F32 SlipFadeTimer;
    S32 Slide;
    F32 SlideTimer;
    S32 Stepping;
    S32 JumpState;
    S32 LastJumpState;
    F32 JumpTimer;
    U32 LastProjectile;
    F32 ControlOffTimer;
    F32 ControlOnTime;
    F32 ForceSlipperyTimer;
    F32 ForceSlipperyFriction;
    F32 ShockRadius;
    F32 ShockRadiusOld;
    U32 ControlOff;
    U32 ControlOnEvent;
    zCheckPoint checkpoint;
    xVec3 turnToFace;
    zGrapplePoint* Grapple;
    U8 bCheatMode;
    U8 bHasBeenHit;
};

enum zGlobalDemoType
{
    zDT_NONE,
    zDT_E3,
    zDT_PUBLICITY,
    zDT_OPM,
    zDT_LAST
};

struct zGlobals : xGlobals
{
    U32 playerTag;
    bool playerLoaded;
    bool invertJoystick;
    bool invertCameraX;
    bool invertCameraY;
    F32 timeMultiplier;
    zPlayerGlobals player;
    zAssetPickupTable* pickupTable;
    zCutsceneMgr* cmgr;
    char startDebugMode[32];
    U32 noMovies;
    U32 boundUpdateTime;
    bool draw_player_after_fx;
    bool bAllowMasterCheats;
    zGlobalDemoType demoType;
    zCutsceneMgr* DisabledCutsceneDoneMgr;
};

#endif