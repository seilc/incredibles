#ifndef XRENDERSTATE_H
#define XRENDERSTATE_H

#include "types.h"
#include <rwcore.h>
#include <rpworld.h>

void xRenderSceneEnter();
void xRenderSceneExit();
void xRenderStateSetZBias(F32 zbias);
void xRenderStateResetZBias();
void xRenderStateSetAlphaDiscard(S32 value);
void xRenderStateResetAlphaDiscard();
void xRenderFixUntexturedBegin(RpAtomic*);
void xRenderFixUntexturedEnd(RpAtomic*);
void xRenderFixIMBegin();
void xRenderFixIMEnd();

#endif