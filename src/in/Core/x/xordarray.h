#ifndef XORDARRAY_H
#define XORDARRAY_H

#include "types.h"

struct st_XORDEREDARRAY
{
    void** list;
    S32 cnt;
    S32 max;
    S32 warnlvl;
};

typedef S32(*XOrdCompareCallback)(void*, void*);
typedef S32(*XOrdTestCallback)(const void*, void*);

void XOrdInit(st_XORDEREDARRAY* array, S32 size, S32 tempAlloc);
void XOrdDone(st_XORDEREDARRAY* array, S32 wasTempAlloc);
void XOrdAppend(st_XORDEREDARRAY* array, void* elt);
void XOrdInsert(st_XORDEREDARRAY* array, void* elt, XOrdCompareCallback compare);
S32 XOrdLookup(st_XORDEREDARRAY* array, const void* key, XOrdTestCallback test);
void XOrdSort(st_XORDEREDARRAY* array, XOrdCompareCallback test);

#endif