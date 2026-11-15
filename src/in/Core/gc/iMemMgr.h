#ifndef IMEMMGR_H
#define IMEMMGR_H

#include "types.h"

void iMemInit();
void iMemExit();
void* iMemMgrMalloc(unsigned long size, U32);
void* iMemMgrRealloc(void* mem, unsigned long newSize, U32);
void iMemMgrFree(void* mem);
void* iMemMgrCalloc(unsigned long numObj, unsigned long sizeObj, U32);
bool iMemMgrCheckCollisionWithMallocHeap(void* mem, U32 size);
void iMemMgrEnableRenderWareMemoryHack(bool hackEnabled);
void* iMemPushTemp(U32 size);
void iMemPopTemp(void* pointer);
S32 iMemMgrGetMinFreeSpace();

#endif
