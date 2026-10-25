#ifndef XSCRPULSEGLARE_H
#define XSCRPULSEGLARE_H

#include "xMath3.h"

#include <rwcore.h>

S32 xScrFX_PulseGlaresAdd(RwRaster* raster);
S32 xScrFX_PulseGlaresAdd(U32 glareID);
void xScrFX_PulseGlaresRender(const xMat4x3& mat);
void xScrFX_PulseGlaresUpdate(F32 dt);
void xScrFX_PulseGlaresReset();
void xScrFX_PulseGlaresUpdatePos(S32 idx, xVec3* pos);
void xScrFX_PulseGlaresUpdateDir(S32 idx, xVec3* dir);
void xScrFX_PulseGlaresUpdateSize(S32 idx, F32 size);
void xScrFX_PulseGlaresSetScale(S32 idx, F32 scale);
void xScrFX_PulseGlaresSetLifetime(S32 idx, F32 lifetime);
void xScrFX_PulseGlaresSetFlag(S32 idx, S32 flag, bool reverse);
bool xScrFX_PulseGlaresCheckFlag(S32 idx, S32 flag);

#endif
