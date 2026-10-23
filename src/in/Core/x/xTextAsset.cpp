#include "xTextAsset.h"

#include "xstransvc.h"
#include "xString.h"

const char* xTextFindString(U32 key, U32* len)
{
    if (key == 0)
    {
        return NULL;
    }

    U32 size;
    xTextAsset* asset = (xTextAsset*)xSTFindAsset(key, &size);

    if (!asset)
    {
        return NULL;
    }

    if (len)
    {
        *len = asset->len;
    }

    return (const char*)(asset + 1);
}

const char* xTextFindString(const char* key, U32* len)
{
    return xTextFindString(xStrHash(key), len);
}
