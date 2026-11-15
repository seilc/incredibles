#ifndef IWATER_H
#define IWATER_H

#include "water/fxmain.h"

#include <rpworld.h>

extern S32 gGCwaterDisable;
extern RpLight* iWaterDirectionalLight;
extern FxParam* iWaterEMBM;

void iWaterBumpGen();
void iWaterCreateMTEffect();
void iWaterSceneEnter(RpWorld*);
void iWaterSceneExit();
void iWaterEnable(RpAtomic* atomic);
void iWaterUpdate(F32);
void iWaterReset();

#endif
