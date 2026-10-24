#ifndef XJAW_H
#define XJAW_H

#include "types.h"

struct xJawDataTable
{
    U32 soundHashID;
    U32 dataStart;
    U32 dataLength;
};

void* xJaw_FindData(U32 soundID);
F32 xJaw_EvalData(void* data, F32 time);

#endif