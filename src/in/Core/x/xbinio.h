#ifndef XBINIO_H
#define XBINIO_H

#include "types.h"
#include <rwcore.h>

struct tag_xFile;

typedef enum en_BIO_ASYNC_ERRCODES
{
    BINIO_ASYNC_FAIL = -1,
    BINIO_ASYNC_NOOP = 0,
    BINIO_ASYNC_INPROG,
    BINIO_ASYNC_DONE,
    BINIO_ASYNC_FORCEENUMSIZEINT = RWFORCEENUMSIZEINT
} BIO_ASYNC_ERRCODES;

typedef enum en_FIOERRCODES
{
    FIOERR_NONE,
    FIOERR_READFAIL,
    FIOERR_WRITEFAIL,
    FIOERR_SEEKFAIL,
    FIOERR_USERABORT
} FIOERRCODES;

typedef struct st_FILELOADINFO
{
    void (*destroy)(st_FILELOADINFO*);
    S32 (*readBytes)(st_FILELOADINFO*, char*, S32);
    S32 (*readMShorts)(st_FILELOADINFO*, S16*, S32);
    S32 (*readMLongs)(st_FILELOADINFO*, S32*, S32);
    S32 (*readMFloats)(st_FILELOADINFO*, F32*, S32);
    S32 (*readMDoubles)(st_FILELOADINFO*, F64*, S32);
    S32 (*readIShorts)(st_FILELOADINFO*, S16*, S32);
    S32 (*readILongs)(st_FILELOADINFO*, S32*, S32);
    S32 (*readIFloats)(st_FILELOADINFO*, F32*, S32);
    S32 (*readIDoubles)(st_FILELOADINFO*, F64*, S32);
    S32 (*skipBytes)(st_FILELOADINFO*, S32);
    S32 (*seekSpot)(st_FILELOADINFO*, S32);
    void (*setDoubleBuf)(st_FILELOADINFO*, char*, S32);
    void (*discardDblBuf)(st_FILELOADINFO*);
    S32 (*asyncIRead)(st_FILELOADINFO*, S32, char*, S32, S32);
    S32 (*asyncMRead)(st_FILELOADINFO*, S32, char*, S32, S32);
    BIO_ASYNC_ERRCODES (*asyncReadStatus)(st_FILELOADINFO*);
    U32 lockid;
    FIOERRCODES error;
    U32 basesector;
    void* privdata;
    void* xtradata;
    void* asyndata;
    S32 filesize;
    S32 remain;
    S32 position;
} FILELOADINFO;

FILELOADINFO* xBinioLoadCreate(const char* filename, S32 fileflags);

void xFileSetUserData(tag_xFile* xf, void* p);
void* xFileGetUserData(tag_xFile* xf);

#endif