#ifndef ISCRFX_H
#define ISCRFX_H

#include "xVec3.h"

#include <rwcore.h>

struct iScrFxLensFlare
{
    xVec3 pos;
    F32 dist2;
    xVec3 at;
    void* source;
    F32 intensity;
    F32 attenuatedIntensity;
    U32 flags;
    U32 padding;
};

void iScrFxInit();
void iScrFxBegin();
void iScrFxEnd();
void iScrFxLensFlare_Reset();
void iScrFxLensFlare_Render(RwCamera* camera, iScrFxLensFlare* flare, void (*renderCB)(RwRaster*, RwFrame*, U8, void*));
void iScrFxCameraCreated(RwCamera* pCamera);
void iScrFxCameraEndScene(RwCamera* pCamera);
S32 iScrFxCameraDestroyed(RwCamera*);

#endif
