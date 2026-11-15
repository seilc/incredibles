#ifndef XDEBUG_H
#define XDEBUG_H

#include "iDebug.h"

struct xSB;
struct xVec2;
struct xVec3;

void xSBRender(xSB* sb); // UNUSED
void xprintf(const char* fmt, ...);
void xDebugInit();
void xDebugUpdate();
void xDebugExit();
void xDebugTimestampScreen();
void xDrawTextInit(U32 uBufferSize, U32 uNumLinesMax);
void xDrawTextRender();
bool xDrawTextComputeScreenPt(xVec2& vUnitScreen, const xVec3& vWorldSpace); // UNUSED

extern U32 gFrameCount;
extern U32 gSceneFrameCount;

#define xTWEAKABLE const

#endif
