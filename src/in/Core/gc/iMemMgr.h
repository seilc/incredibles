#ifndef IMEMMGR_H
#define IMEMMGR_H

#include "types.h"

extern S32 the_heap;

void iMemInit();
void iMemExit();
bool iMemMgrCheckCollisionWithMallocHeap(void* mem, U32 size);
void iMemMgrEnableRenderWareMemoryHack(bool hackEnabled);
void* iMemPushTemp(U32 size);
void iMemPopTemp(void* pointer);
S32 iMemMgrGetMinFreeSpace();

#endif
