#ifndef XUTIL_H
#define XUTIL_H

#include "xRand.h"

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

const char* xUtil_idtag2string(U32 srctag, S32 bufidx = 0);
void quicksort_istr(void* base, unsigned long num, unsigned long width, unsigned long key_offset);

template <class T, class V, class C>
T binary_find(T first, T last, const V& value, C cmp)
{
    const T notfound = last;
    while (first < last)
    {
        const T mid = first + (last - first) / 2;
        S32 c = cmp(value, *mid);
        if (c < 0)
            last = mid;
        else if (c > 0)
            first = mid + 1;
        else
            return mid;
    }
    return notfound;
}

template <class T> static inline T xUtil_choose(const T* r3, S32 r4, const F32* r5 = NULL)
{
    if (!r3)
    {
        return 0;
    }

    if (r4 < 1)
    {
        return 0;
    }

    S32 r31 = 0;
    F32 f1 = xurand();

    if (!r5)
    {
        r31 = f1 * r4;
    }
    else
    {
        F32 f2 = 0.0f;

        for (S32 i = 0; i < r4; i++)
        {
            F32 f3 = f2;

            f2 += r5[i];

            if (f1 >= f3 && f1 <= f2)
            {
                r31 = i;
                break;
            }
        }
    }

    if (r31 >= r4)
    {
        r31 = r4 - 1;
    }

    if (r31 < 0)
    {
        r31 = 0;
    }

    return r3[r31];
}

#endif