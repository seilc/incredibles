#ifndef XFMV_H
#define XFMV_H

#include "iFMV.h"

typedef void(*xFMVPauseSoundCallback)(bool doPause);

void xFMVPauseSound(bool doPause);
xFMVPauseSoundCallback xFMVSetPauseSoundCB(xFMVPauseSoundCallback pauseSoundCB); // inferred from TSSM GC, Rat MAP
S32 xFMVGetBinkCompliantVolume();
RwTexture* xFMVGetTexture(MOVIE* pMovie, U16 looping, U16 freezeframed, U32 startframe, U32 endframe, U32);

#endif
