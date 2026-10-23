#ifndef XCUTSCENEMGR_H
#define XCUTSCENEMGR_H

#include "xCutscene.h"
#include "xBase.h"

struct xCutsceneMgrAsset : xBaseAsset
{
    U32 cutsceneAssetID;
    U32 flags;
    F32 interpSpeed;
    U32 uSubtitlesID;
    F32 startTime[15];
    F32 endTime[15];
    U32 emitID[15];
};

struct xCutsceneMgr : xBase
{
    xCutsceneMgrAsset* tasset;
    xCutscene* csn;
    U32 stop;
    F32 oldfov;
};

#endif