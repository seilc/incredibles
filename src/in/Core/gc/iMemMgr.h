#ifndef IMEMMGR_H
#define IMEMMGR_H

#include "types.h"

void iMemInit();
void iMemExit();
bool iMemMgrCheckCollisionWithMallocHeap(void* mem, U32 size);
void iMemPopTemp(void* pointer);
S32 iMemMgrGetMinFreeSpace();

#endif
