#ifndef ICUTSCENE_H
#define ICUTSCENE_H

#include "types.h"

struct xCutscene;

U32 iCSFileOpen(xCutscene* csn);
void iCSFileAsyncRead(xCutscene* csn, void* dest, U32 size);
void iCSFileClose(xCutscene* csn);

#endif
