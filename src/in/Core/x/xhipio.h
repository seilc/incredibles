#ifndef XHIPIO_H
#define XHIPIO_H

#include "xbinio.h"

#define MAX_OPENBLK 8

typedef enum en_READ_ASYNC_STATUS
{
    HIP_RDSTAT_NONE = -1,
    HIP_RDSTAT_INPROG = 0,
    HIP_RDSTAT_SUCCESS,
    HIP_RDSTAT_FAILED,
    HIP_RDSTAT_NOBYPASS,
    HIP_RDSTAT_NOASYNC
} READ_ASYNC_STATUS;

typedef struct st_HIPLOADBLOCK
{
    S32 endpos;
    U32 blk_id;
    S32 blk_remain;
    S32 flags;
} HIPLOADBLOCK;

typedef struct st_HIPLOADDATA
{
    st_FILELOADINFO* fli;
    S32 lockid;
    S32 bypass;
    S32 bypass_recover;
    U32 base_sector;
    S32 use_async;
    READ_ASYNC_STATUS asyn_stat;
    S32 pos;
    S32 top;
    S32 readTop;
    HIPLOADBLOCK stk[MAX_OPENBLK];
} HIPLOADDATA;

typedef struct st_HIPLOADFUNCS
{
    HIPLOADDATA* (*create)(const char* filename, char* dblbuf, S32 bufsize, S32 fileflags);
    void (*destroy)(HIPLOADDATA* lddata);
    U32 (*basesector)(HIPLOADDATA* lddata);
    U32 (*enter)(HIPLOADDATA* lddata);
    void (*exit)(HIPLOADDATA* lddata);
    S32 (*readBytes)(HIPLOADDATA* lddata, char* data, S32 cnt);
    S32 (*readShorts)(HIPLOADDATA* lddata, S16* data, S32 cnt);
    S32 (*readLongs)(HIPLOADDATA* lddata, S32* data, S32 cnt);
    S32 (*readFloats)(HIPLOADDATA* lddata, F32* data, S32 cnt);
    S32 (*readString)(HIPLOADDATA* lddata, char* buf);
    S32 (*setBypass)(HIPLOADDATA* lddata, S32 enable, S32 use_async);
    void (*setSpot)(HIPLOADDATA* lddata, S32 spot);
    READ_ASYNC_STATUS (*pollRead)(HIPLOADDATA* lddata);
} HIPLOADFUNCS;

HIPLOADFUNCS* get_HIPLFuncs();

#endif