#ifndef ZSPLASH_H
#define ZSPLASH_H

#include "xMath3.h"

S32 zSplashFindType(const char* name);
void zSplashEmit(S32 type, const xVec3& loc);
void zEmitSwimRings(F32& emitted, xVec3& pos, const xVec3& velocity, F32 speed, F32 dt);

#endif
