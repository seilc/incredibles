#ifndef ZLOD_H
#define ZLOD_H

#include "xModelBucket.h"

struct zLODTable
{
    xModelBucket** baseBucket;
    F32 noRenderDist;
    U32 flags;
    xModelBucket** lodBucket[3];
    F32 lodDist[3];
};

struct zLODManager
{
    S32 numextra;
    zLODTable* lod;
    xModelInstance* model;
    F32 adjustNoRenderDist;
};

void zLOD_Setup(F32 fadeDistance);
void zLOD_Update(U32 percent_update);
zLODTable* zLOD_Get(xEnt* ent);

#endif