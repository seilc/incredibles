#ifndef XHIPIO_H
#define XHIPIO_H

#include "xbinio.h"

#define MAX_OPENBLK 8

enum en_READ_ASYNC_STATUS
{
    HIP_RDSTAT_NONE = -1,
    HIP_RDSTAT_INPROG = 0,
    HIP_RDSTAT_SUCCESS,
    HIP_RDSTAT_FAILED,
    HIP_RDSTAT_NOBYPASS,
    HIP_RDSTAT_NOASYNC
};

struct st_HIPLOADBLOCK
{
    S32 endpos;
    U32 blk_id;
    S32 blk_remain;
    S32 flags;
};

struct st_HIPLOADDATA
{
    st_FILELOADINFO* fli;
    S32 lockid;
    S32 bypass;
    S32 bypass_recover;
    U32 base_sector;
    S32 use_async;
    en_READ_ASYNC_STATUS asyn_stat;
    S32 pos;
    S32 top;
    S32 readTop;
    st_HIPLOADBLOCK stk[MAX_OPENBLK];
};

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