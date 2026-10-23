#ifndef ZFX_H
#define ZFX_H

#include <types.h>
#include <rwcore.h>
#include <rpworld.h>

extern U32 gFXSurfaceFlags;

void zFXUpdate(F32 dt);
void zFX_SceneEnter(RpWorld* world);
void zFX_SceneExit(RpWorld* world);
void zFX_SceneReset();

#endif