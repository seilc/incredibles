#ifndef XVOLUME_H
#define XVOLUME_H

#include "xBase.h"

struct xBound;
struct xVolumeAsset;

struct xVolume : xBase
{
    xVolumeAsset* asset;

    void Init(xVolumeAsset* asset);
    void Reset();
    void Save(xSerial* s);
    void Load(xSerial* s);
    xBound* GetBound();
};

#endif