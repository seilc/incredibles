#ifndef XFOG_H
#define XFOG_H

#include "xBase.h"
#include "xFogAsset.h"

typedef struct _xFog : xBase
{
    xFogAsset* tasset;
} xFog;

struct xScene;

void xFogClearFog();
void xFogInit(void* b, void* tasset);
void xFogInit(xBase* b, xFogAsset* tasset);
void xFogReset(xFog* t);
void xFogSave(xFog* ent, xSerial* s);
void xFogLoad(xFog* ent, xSerial* s);
void xFogEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);
void xFogUpdate(xBase*, xScene*, F32);

#endif