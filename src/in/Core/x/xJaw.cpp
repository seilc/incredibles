#include "xJaw.h"

#include "xstransvc.h"

#include <cmath>

void* xJaw_FindData(U32 soundID)
{
    S32 i;
    S32 numJawTables = xSTAssetCountByType('JAW ');

    for (i = 0; i < numJawTables; i++)
    {
        U32 j;
        void* data = xSTFindAssetByType('JAW ', i, NULL);
        U32 jawcount = *(U32*)data;
        xJawDataTable* tbl = (xJawDataTable*)((U32*)data + 1);
        void* rawdata = (void*)(tbl + jawcount);

        for (j = 0; j < jawcount; j++)
        {
            if (soundID == tbl[j].soundHashID)
            {
                return (U8*)rawdata + tbl[j].dataStart;
            }
        }
    }

    xSTAssetName(soundID);

    return NULL;
}

#define swap(data)                                                                                 \
    {                                                                                              \
        U8 c;                                                                                   \
        c = ((U8*)(data))[3];                                                                   \
        ((U8*)(data))[3] = ((U8*)(data))[0];                                                 \
        ((U8*)(data))[0] = c;                                                                   \
        c = ((U8*)(data))[2];                                                                   \
        ((U8*)(data))[2] = ((U8*)(data))[1];                                                 \
        ((U8*)(data))[1] = c;                                                                   \
    }

F32 xJaw_EvalData(void* data, F32 time)
{
    F32 lerp;
    S32 idx;
    S32 numdata;

    numdata = *(S32*)data;

    if (numdata > (U32)0xFFFF)
    {
        swap(data);
    }

    time *= 60.0f;

    idx = std::floorf(time);
    lerp = time - std::floorf(time);

    if (idx < 0)
    {
        return 0.0f;
    }
    else if (idx >= numdata - 1)
    {
        return 0.0f;
    }
    else
    {
        U8* jawdata = (U8*)data + sizeof(S32);

        return ((1.0f - lerp) * jawdata[idx] + lerp * jawdata[idx + 1]) / 255.0f;
    }
}