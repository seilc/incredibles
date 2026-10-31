#ifndef IDRAW_H
#define IDRAW_H

#include "types.h"

void iDrawSetFBMSK(U32 abgr, U32 setMask, bool);
void iXFBreset();
void* iXFBalloc(U32 size);

extern U32 lastSetFBMSK;

inline U32 iDrawGetFBMSK()
{
    return lastSetFBMSK;
}

#endif
