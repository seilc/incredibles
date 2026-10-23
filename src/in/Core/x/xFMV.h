#ifndef XFMV_H
#define XFMV_H

#include "iFMV.h"

void xFMVPauseSound(bool doPause);
S32 xFMVGetBinkCompliantVolume();
RwTexture* xFMVGetTexture(MOVIE* pMovie, U16 looping, U16 freezeframed, U32 startframe,
                          U32 endframe, U32);

#endif