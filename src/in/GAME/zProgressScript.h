#ifndef ZPROGRESSSCRIPT_H
#define ZPROGRESSSCRIPT_H

#include "xBase.h"

struct xProgressScriptAsset : xBaseAsset
{
    U32 eventCount;
};

struct xProgressScriptEventAsset
{
    F32 percent;
    S32 flags;
    U32 widget;
    U32 paramEvent;
    F32 param[4];
    U32 paramWidget;
};

struct zProgressScript : xBase
{
    xProgressScriptAsset* sasset;
    xProgressScriptEventAsset* events;
    S32 numFlagWords;
    U32* flags;
    F32 percent;
    S32 current;
};

void zProgressScript_Init(void* b, void* sasset);
void zProgressScript_Init(xBase* b, xProgressScriptAsset* sasset);
void zProgressScript_Reset(zProgressScript* s);
void zProgressScript_JumpTo(zProgressScript* s, F32 percent);
void zProgressScript_WalkForward(zProgressScript* s, F32 percent);
void zProgressScript_WalkBack(zProgressScript* s, F32 percent);
void zProgressScript_MoveTo(zProgressScript* s, F32 percent);
void zProgressScript_Save(zProgressScript* ent, xSerial* s);
void zProgressScript_Load(zProgressScript* ent, xSerial* s);
void zProgressScript_EventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*, U32);

#endif