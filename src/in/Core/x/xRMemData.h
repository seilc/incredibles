#ifndef XRMEMDATA_H
#define XRMEMDATA_H

#include "xBase.h"

#include <new>

#define RYZMEMGROW_ENABLED 0x1

struct RyzMemGrow
{
    S32 flg_grow;
    S32 amt;
    char* ptr;
    xBase* user;
    S32 amt_last;
    char* ptr_last;
    xBase* user_last;

    S32 IsEnabled()
    {
        return flg_grow & RYZMEMGROW_ENABLED;
    }
};

struct RyzMemData
{
    void* operator new(size_t amt, S32, RyzMemGrow* growCtxt);
    void operator delete(void*);
};

#endif