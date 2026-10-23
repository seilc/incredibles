#ifndef ZSCRIPT_H
#define ZSCRIPT_H

#include "xBase.h"

struct xScriptAsset : xBaseAsset
{
    F32 scaleFactor;
    U32 eventCount;
    bool loop;
    U8 pad[3];
};

struct xScriptEventAsset
{
    F32 time;
    U32 widget;
    U32 paramEvent;
    F32 param[4];
    U32 paramWidget;
};

struct _zScript : xBase
{
    xScriptAsset* tasset;
    xScriptEventAsset* current;
    U8 state;
    U8 pad[3];
    F32 time;
    F32 scaleFactor;
};

#define ZSCRIPT_STOPPED 0
#define ZSCRIPT_RUNNING 1

struct xScene;

void zScriptInit(void* b, void* tasset);
void zScriptInit(xBase* b, xScriptAsset* tasset);
void zScriptReset(_zScript* t);
void zScriptSave(_zScript* ent, xSerial* s);
void zScriptLoad(_zScript* ent, xSerial* s);
void zScriptEventCB(xBase*, xBase* to, U32 toEvent, const F32* toParam, xBase*, U32);
void zScriptUpdate(xBase* to, xScene*, F32 dt);

#endif