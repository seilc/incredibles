#ifndef XCM_H
#define XCM_H

#include "xColor.h"

#include <rwcore.h>

struct xBase;
struct xCreditsData;

void xCMupdate(F32 dt);
void xCMrender();
void xCMstart(xCreditsData* data, F32, xBase* parent);
void xCMstop();
void xCMsetDest(F32 x, F32 y, F32 width, F32 height);
bool xCMisRunning();

#endif
