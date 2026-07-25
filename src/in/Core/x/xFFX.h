#ifndef XFFX_H
#define XFFX_H

#include "xEnt.h"

struct xFFX
{
    U32 flags;
    void (*doEffect)(xEnt*, xScene*, F32, void*);
    void* fdata;
    xFFX* next;
};

#endif