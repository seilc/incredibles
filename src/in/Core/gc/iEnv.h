#ifndef IENV_H
#define IENV_H

#include "xMath3.h"
#include "xJSP.h"

#include <rwcore.h>
#include <rpworld.h>

struct xEnvAsset;

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

void iEnvLoadBegin(iEnv* env, S32 dataType, S32 count);
bool iEnvLoadJSP(iEnv* env, U32 aid, const void* data, U32, S32 dataType, S32 index);
void iEnvJSPVisibilityInc(iEnv* env, xJSPHeader* jsp);
void iEnvJSPVisibilityDec(iEnv* env, xJSPHeader* jsp);
void iEnvJSPOn(iEnv* env, xJSPHeader* jsp);
void iEnvJSPOff(iEnv* env, xJSPHeader* jsp);
void iBuildJSPBound(const xClumpCollBSPTree* colltree, xBox& box);
void iEnvLoadEnd(iEnv* env, S32 dataType);
void iEnvFree(iEnv* env);
void iEnvDefaultLighting(iEnv*);
void iEnvLightingBasics(iEnv*, xEnvAsset*);
void iEnvSetup(iEnv* env);
void iEnvRender(iEnv* env, bool alpha);
void iEnvEndRenderFX(iEnv* env);

inline bool iEnvIsJSPActive(const iEnv* env, S32 which)
{
    return env->jsp_visibilityCount[which] > 0;
}

inline bool iEnvIsJSPVisible(const iEnv* env, S32 which)
{
    return env->jsp_visibilityCount[which] > 0;
}

inline xBox* iEnvGetBBox(const iEnv* env)
{
    return (xBox*)RpWorldGetBBox(env->world);
}

#endif
