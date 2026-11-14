#ifndef IARAMTMP_H
#define IARAMTMP_H

#include "types.h"

U32 iARAMTmpBegin(U32 type, U32 max, bool doTRCCheck);
void* iARAMTmpMalloc(U32 type, U32 size);
void iARAMTmpFree(U32, void*);
S32 iARAMTmpInARAMTemp(U32 type, void* ptr);
void iARAMTmpEnd(U32 type, bool doTRCCheck);

#endif
