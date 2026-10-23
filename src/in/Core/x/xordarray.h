#ifndef XORDARRAY_H
#define XORDARRAY_H

#include <types.h>

typedef struct st_XORDEREDARRAY
{
    void** list;
    S32 cnt;
    S32 max;
    S32 warnlvl;
} XORDEREDARRAY;

typedef S32 (*XOrdCompareCB)(void* vkey, void* vitem);
typedef S32 (*XOrdTestCB)(const void* vkey, void* vitem);

void XOrdInit(XORDEREDARRAY* array, S32 size, S32 tempAlloc);
void XOrdDone(XORDEREDARRAY* array, S32 wasTempAlloc);
void XOrdAppend(XORDEREDARRAY* array, void* elt);
void XOrdInsert(XORDEREDARRAY* array, void* elt, XOrdCompareCB compare);
S32 XOrdLookup(XORDEREDARRAY* array, const void* key, XOrdTestCB test);
void XOrdSort(XORDEREDARRAY* array, XOrdCompareCB test);

#endif