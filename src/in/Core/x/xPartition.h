#ifndef XPARTITION_H
#define XPARTITION_H

#include "xMath3.h"

typedef struct _tagPartLink
{
    void* data;
    _tagPartLink* next;
} PartLink;

typedef struct _tagPartSpace
{
    S32 total;
    PartLink head;
} PartSpace;

typedef struct _tagPartition
{
    xVec3 min;
    xVec3 max;
    xVec3 space_dim;
    S32 total_x;
    S32 total_y;
    S32 total_z;
    PartSpace* space;
    PartSpace global;
} Partition;

#endif