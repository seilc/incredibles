#ifndef ZLIGHTEFFECT_H
#define ZLIGHTEFFECT_H

#include "zLight.h"

void zLightEffectInitStrobe(zLight*);
void zLightEffectInitDim(zLight*);
void zLightEffectInitHalfDim(zLight*);
void zLightEffectInitRandomCol(zLight*);
void zLightEffectInitFlicker(zLight* zlight);
void zLightEffectFlicker(zLight* zlight, F32 seconds);
void zLightEffectFlickerSlow(zLight* zlight, F32 seconds);
void zLightEffectFlickerErratic(zLight* zlight, F32 seconds);
void zLightEffectStrobeSlow(zLight*, F32);
void zLightEffectStrobe(zLight*, F32);
void zLightEffectStrobeFast(zLight*, F32);
void zLightEffectDimSlow(zLight*, F32);
void zLightEffectDim(zLight*, F32);
void zLightEffectDimFast(zLight*, F32);
void zLightEffectHalfDimSlow(zLight*, F32);
void zLightEffectHalfDim(zLight*, F32);
void zLightEffectHalfDimFast(zLight*, F32);
void zLightEffectRandomColSlow(zLight*, F32);
void zLightEffectRandomCol(zLight*, F32);
void zLightEffectRandomColFast(zLight*, F32);
void zLightEffectInitCauldron(zLight* zlight);
void zLightEffectCauldron(zLight* zlight, F32 seconds);

#endif
