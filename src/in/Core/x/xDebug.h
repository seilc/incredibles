#ifndef XDEBUG_H
#define XDEBUG_H

#include "iDebug.h"

extern U32 gFrameCount;
extern U32 gSceneFrameCount;

void xprintf(const char*, ...);

void xDebugInit();
void xDebugUpdate();
void xDebugExit();
void xDebugTimestampScreen();
void xDrawTextInit(U32, U32);
void xDrawTextRender();

#endif