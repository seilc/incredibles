#ifndef FXMAIN_H
#define FXMAIN_H

#include "types.h"

#include <rwcore.h>
#include <rpworld.h>
#include <rpmatfx.h>

#define NUM_FX_FRAMES 2

typedef struct FxParam FxParam;

struct FxParam
{
    RpMaterial* material;
    RpMTEffect* effect;
    void (*set)();
    void (*unset)();
    void (*close)();
};

extern RwFrame* FxFrameArray[NUM_FX_FRAMES];

RpAtomic* AtomicSetupEffect(RpAtomic* atomic, void* data);
S32 FxOpen();
void FxClose();

#endif
