#ifndef IENV_H
#define IENV_H

#include "xJSP.h"
#include "xMath3.h"

struct iEnv
{
    RpWorld* world;
    RpWorld* collision;
    RpWorld* fx;
    RpWorld* camera;
    S32 jsp_count;
    U32* jsp_aid;
    xJSPHeader** jsp_list;
    xBox* jsp_bound;
    S32* jsp_visibilityCount;
    RpLight* light[2];
    RwFrame* light_frame[2];
    S32 memlvl;
};

struct xEnvAsset;

void iEnvLoadBegin(iEnv* env, S32 dataType, S32 count);
bool iEnvLoadJSP(iEnv* env, U32 aid, const void* data, U32, S32 dataType, S32 index);
void iEnvJSPVisibilityInc(iEnv* env, xJSPHeader* jsp);
void iEnvJSPVisibilityDec(iEnv* env, xJSPHeader* jsp);
void iEnvJSPOn(iEnv* env, xJSPHeader* jsp);
void iEnvJSPOff(iEnv* env, xJSPHeader* jsp);
void iEnvLoadEnd(iEnv* env, S32 dataType);
void iEnvFree(iEnv* env);
void iEnvDefaultLighting(iEnv*);
void iEnvLightingBasics(iEnv*, xEnvAsset*);
void iEnvSetup(iEnv* env);
void iEnvRender(iEnv* env, bool alpha);

#endif