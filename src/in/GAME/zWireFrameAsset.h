#ifndef ZWIREFRAMEASSET_H
#define ZWIREFRAMEASSET_H

#include <types.h>

struct zWireframeVertex
{
    F32 x;
    F32 y;
    F32 z;
};

struct zWireframeLine
{
    U16 start;
    U16 end;
};

struct zWireframeAsset
{
    U32 size;
    U32 vertexCount;
    U32 lineCount;
    zWireframeVertex* vertices;
    zWireframeLine* lines;
};

void WireframeAssetSetupPointers(zWireframeAsset* wireframe);

#endif