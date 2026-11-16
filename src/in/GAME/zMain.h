#ifndef ZMAIN_H
#define ZMAIN_H

#include "zGlobals.h"

typedef enum _tagGameVidMode
{
    eGameVidModeNTSC,
    eGameVidModePAL
} eGameVidMode;

extern U32 FB_XRES;
extern U32 FB_YRES;
extern U32 SCENE_ID_MNU_START;
extern S32 GCN_DEMO_PARAM;
extern eGameVidMode gVidMode;
extern F32 SECS_PER_VBLANK;
extern xGlobals* xglobals;

#define ZPLAYER (xglobals->___player_ent_dont_use_directly)

void zMainShowProgressBar(S32 percentage);

inline bool zMainIsPAL() { return gVidMode == eGameVidModePAL; }

inline bool zMainIsNTSC() { return gVidMode == eGameVidModeNTSC; }

#endif