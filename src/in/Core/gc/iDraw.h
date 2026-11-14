#ifndef IDRAW_H
#define IDRAW_H

#include "types.h"

void iDrawBegin();
void iDrawEnd();
void iDrawSetFBMSK(U32 abgr, U32 setMask, bool);
void iXFBreset();
void* iXFBalloc(U32 size);

extern U32 lastSetFBMSK;

inline U32 iDrawGetFBMSK()
{
    return lastSetFBMSK;
}

void iDrawSetDisplayOffset(F32 x, F32 y);
void iXFBinit();

#endif
