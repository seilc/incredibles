#include "xordarray.h"

#include "xMemMgr.h"


void XOrdInit(XORDEREDARRAY* array, S32 size, S32 tempAlloc)
{
    size = (size<1) ? 1 : size;

    if (tempAlloc)
    {
        array->list = (void**)xMEMPUSHTEMP_L(size * sizeof(void*), 'XORD', 75);
    }
    else
    {
        array->list = (void**)xMEMALLOC(size * sizeof(void*));
    }

    array->cnt = 0;
    array->max = size;
    array->warnlvl = 0.95f * size;

    if (array->warnlvl == array->max)
    {
        array->warnlvl = (array->max-1 < 0) ? 0 : array->max-1;
    }
}

void XOrdDone(XORDEREDARRAY* array, S32 wasTempAlloc)
{
    if (array->max && wasTempAlloc)
    {
        xMEMPOPTEMP(array->list);
    }

    array->list = NULL;
    array->cnt = 0;
    array->max = 0;
    array->warnlvl = 0;
}

void XOrdAppend(XORDEREDARRAY* array, void* elt)
{
    if (array->cnt < array->max)
    {
        array->list[array->cnt++] = elt;
    }
}

void XOrdInsert(XORDEREDARRAY* array, void* elt, XOrdCompareCB compare)
{
    if (array->cnt < array->max)
    {
        array->cnt++;

        for (S32 i = array->cnt - 1; i > 0; i--)
        {
            if (compare(array->list[i - 1], elt) <= 0)
            {
                array->list[i] = elt;
                return;
            }
            else
            {
                array->list[i] = array->list[i - 1];
            }
        }

        array->list[0] = elt;
    }
}

S32 XOrdLookup(XORDEREDARRAY* array, const void* key, XOrdTestCB test)
{
    S32 da_idx = -1;
    S32 k0 = 0;
    S32 k1 = array->cnt;

    while (k1 > k0)
    {
        S32 k = (k0 + k1) / 2;
        S32 v = test(key, array->list[k]);

        if (v == 0)
        {
            da_idx = k;
            break;
        }

        if (v > 0)
        {
            k0 = k + 1;
        }
        else
        {
            k1 = k;
        }
    }

    return da_idx;
}

void XOrdSort(XORDEREDARRAY* array, XOrdCompareCB test)
{
    void** list = array->list;
    S32 num = array->cnt;
    S32 i, j;
    S32 h = 1;

    while (h <= num)
    {
        h = h * 3 + 1;
    }

    while (h != 1)
    {
        h /= 3;

        for (i = h; i < num; i++)
        {
            void* v = list[i];

            for (j = i; j >= h && test(v, list[j - h]) < 0; j -= h)
            {
                list[j] = list[j - h];
            }

            list[j] = v;
        }
    }
}