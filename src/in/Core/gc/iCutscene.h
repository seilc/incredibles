#ifndef ICUTSCENE_H
#define ICUTSCENE_H

#include "types.h"

struct xCutscene;

void iCSSoundSetup(xCutscene* csn);
void* iCSSoundGetData(U32 assetID, U32* len);
U32 iCSFileOpen(xCutscene* csn);
void iCSFileAsyncRead(xCutscene* csn, void* dest, U32 size);
void iCSFileAsyncSkip(xCutscene* csn, U32 size);
void iCSFileClose(xCutscene* csn);
S32 iCSLoadStep(xCutscene* csn);

#endif
