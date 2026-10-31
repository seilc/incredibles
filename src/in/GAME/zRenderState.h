#ifndef ZRENDERSTATE_H
#define ZRENDERSTATE_H

#include "types.h"

typedef enum _SDRenderState
{
    SDRS_Unknown,
    SDRS_Default,
    SDRS_OpaqueModels,
    SDRS_AlphaModels,
    SDRS_Bubble,
    SDRS_Projectile,
    SDRS_Font,
    SDRS_HUD,
    SDRS_Particles,
    SDRS_Lightning,
    SDRS_SkidMark,
    SDRS_Streak,
    SDRS_Tide,
    SDRS_SkyBack,
    SDRS_Environment,
    SDRS_NPCMatter,
    SDRS_NPCVisual,
    SDRS_NPCBossMeter,
    SDRS_Fill,
    SDRS_OOBFade,
    SDRS_OOBPlayerZ,
    SDRS_OOBPlayerAlpha,
    SDRS_OOBHand,
    SDRS_Glare,
    SDRS_Newsfish,
    SDRS_CruiseHUD,
    SDRS_DiscoFloorGlow,
    SDRS_FireCards,
    SDRS_SplashCones,
    SDRS_Water,
    SDRS_Grass,
    SDRS_WaterSkyDome,
    SDRS_LaserBeam,
    SDRS_Spotlight,
    SDRS_LightVolume,
    SDRS_Ribbon,
    SDRS_ScanLaser,
    SDRS_HDRBeginRender,
    SDRS_BlurAlpha,
    SDRS_ZParPTank,
    SDRS_XPTankPool,
    SDRS_UI,
    SDRS_IncrediBall,
    SDRS_Frozone,
    SDRS_SmashRing,
    SDRS_LaserBolt,
    SDRS_PatStunRings,
    SDRS_Total = -1
} SDRenderState;

void zRenderState(SDRenderState next);

#endif
