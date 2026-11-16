#ifndef ZFROZONEICE_H
#define ZFROZONEICE_H

#include "xVec3.h"

void zFrozoneIceBridgeStart(bool force_gap);
void zFrozoneIceBridgeMove(const xVec3& lead_loc, const xVec3& foot_loc, bool gap);
void zFrozoneIceBridgeStop();

#endif
