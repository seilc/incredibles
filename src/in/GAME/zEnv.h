#ifndef ZENV_H
#define ZENV_H

#include "xBase.h"
#include "xEnvAsset.h"
#include "xEnv.h"
#include "xMath3.h"

// xEnvAsset::flags
#define XENVASSET_FLAGS_0x1 0x1

typedef struct _zEnv : xBase
{
    xEnvAsset* easset;
} zEnv;

void zEnvInit(void* env, void* easset);
void zEnvInit(zEnv* env, xEnvAsset* easset);
void zEnvSetup(zEnv* env);
void zEnvStartingCamera(zEnv*);
void zEnvRender(xEnv* env);
void zEnvSave(zEnv* ent, xSerial* s);
void zEnvLoad(zEnv* ent, xSerial* s);
void zEnvReset(zEnv* env);
void zEnvEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase* toParamWidget, U32);

#endif