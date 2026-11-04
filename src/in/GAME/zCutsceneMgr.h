#ifndef ZCUTSCENEMGR_H
#define ZCUTSCENEMGR_H

#include "xCutsceneMgr.h"

struct zCutsceneHack
{
    char* cinname;
    char* modelname;
    U32 noshadow;
    U32 numPipes;
    RpAtomicCallBackRender renderCB;
    xModelPipe pipe[8];
    void* dataPtr;
    U32 numAtomics;
    void* morph_dataPtr[8];
    void* atomic_dataPtr[8];
    U32 atomic_Idx[8];
};

struct zCutsceneMgr : xCutsceneMgr
{
    bool pausedMusic;
    bool sendDoneForDisabledCutscene;
};

void zCutsceneMgr_Started();
S32 zCutsceneMgrGetHackTableSize();
zCutsceneHack& zCutsceneMgrGetHackTableDataByIdx(S32 idx);

#endif