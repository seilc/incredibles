#ifndef ZSCRIPT_H
#define ZSCRIPT_H

#include "xBase.h"
#include "xScriptAsset.h"

typedef struct _zScript : xBase
{
    xScriptAsset* tasset;
    xScriptEventAsset* current;
    U8 state;
    U8 pad[3];
    F32 time;
    F32 scaleFactor;
} zScript;

#define ZSCRIPT_STOPPED 0
#define ZSCRIPT_RUNNING 1

struct xScene;

void zScriptInit(void* b, void* tasset);
void zScriptInit(xBase* b, xScriptAsset* tasset);
void zScriptReset(zScript* t);
void zScriptSave(zScript* ent, xSerial* s);
void zScriptLoad(zScript* ent, xSerial* s);
void zScriptEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32);
void zScriptUpdate(xBase* to, xScene*, F32 dt);

#endif