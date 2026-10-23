#ifndef ZSCENE_H
#define ZSCENE_H

#include "zBase.h"
#include "zPortal.h"
#include "zEnv.h"
#include "xScene.h"
#include "xDynAsset.h"

struct zSceneParameters : xDynAsset
{
    U32 idle03ExtraCount;
    xAnimFile** idle03Extras;
    U32 idle04ExtraCount;
    xAnimFile** idle04Extras;
    U8 bombCount;
    U8 extraIdleDelay;
    U8 hdrGlow;
    U8 hdrDarken;
    U32 uDefaultMusicHash;
    U32 flags;
    F32 waterTileWidth;
    F32 lodFadeDistance;
    U32 pad[4];
};

struct zScene : xScene
{
    _zPortal* pendingPortal;
    S32 num_base;
    xBase** base;
    U32 num_update_base;
    xBase** update_base;
    S32 baseCount[eBaseTypeCount];
    xBase* baseList[eBaseTypeCount];
    _zEnv* zen;
    zSceneParameters* parameters;
    U8 enableDrawing;
};

typedef xBase* (*zSceneForAllBaseCallback)(xBase*, zScene*, void*);

#define ZSCENE_LOAD_HIP 0x1
#define ZSCENE_LOAD_HOP 0x2

void zSceneSwitch(_zPortal* p, S32 forceSameScene);
xBase* zSceneFindObject(U32 gameID);
void zSceneForAllBase(zSceneForAllBaseCallback func, S32 baseType, void* data);
bool zSceneLoadPackFile(U32 id, S32 options, bool noLocalized = false,
                        const char* filename = NULL);

#endif