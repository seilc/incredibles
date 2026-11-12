#ifndef XHIPIO_H
#define XHIPIO_H

#include "types.h"

enum en_READ_ASYNC_STATUS
{
    HIP_RDSTAT_NONE = -1,
    HIP_RDSTAT_INPROG = 0,
    HIP_RDSTAT_SUCCESS,
    HIP_RDSTAT_FAILED,
    HIP_RDSTAT_NOBYPASS,
    HIP_RDSTAT_NOASYNC
};

struct st_HIPLOADDATA;

struct st_HIPLOADFUNCS
{
    st_HIPLOADDATA* (*create)(const char* filename, char* dblbuf, S32 bufsize, S32 fileflags);
    void (*destroy)(st_HIPLOADDATA* lddata);
    U32 (*basesector)(st_HIPLOADDATA* lddata);
    U32 (*enter)(st_HIPLOADDATA* lddata);
    void (*exit)(st_HIPLOADDATA* lddata);
    S32 (*readBytes)(st_HIPLOADDATA* lddata, char* data, S32 cnt);
    S32 (*readShorts)(st_HIPLOADDATA* lddata, S16* data, S32 cnt);
    S32 (*readLongs)(st_HIPLOADDATA* lddata, S32* data, S32 cnt);
    S32 (*readFloats)(st_HIPLOADDATA* lddata, F32* data, S32 cnt);
    S32 (*readString)(st_HIPLOADDATA* lddata, char* buf);
    S32 (*setBypass)(st_HIPLOADDATA* lddata, S32 enable, S32 use_async);
    void (*setSpot)(st_HIPLOADDATA* lddata, S32 spot);
    en_READ_ASYNC_STATUS (*pollRead)(st_HIPLOADDATA* lddata);
};

st_HIPLOADFUNCS* get_HIPLFuncs();

#endif
