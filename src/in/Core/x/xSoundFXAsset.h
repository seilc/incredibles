#ifndef XSOUNDFXASSET_H
#define XSOUNDFXASSET_H

#include "xBaseAsset.h"
#include "xVec3.h"
#include "xSnd.h"

struct xSoundFXAsset : xBaseAsset
{
    union
    {
        U32 soundAssetID;
        xSndGroupHandle soundAsset;
    };
    U32 attachID;
    xVec3 pos;
    U32 uFlags;
};

#endif
