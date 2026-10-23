#ifndef XSAVEGAME_H
#define XSAVEGAME_H

#include "iTime.h"
#include "isavegame.h"

typedef enum en_SAVEGAME_MODE
{
    XSG_MODE_LOAD = 10,
    XSG_MODE_SAVE = 11
} SAVEGAME_MODE;

typedef struct st_XSAVEGAME_DATA XSAVEGAME_DATA;

typedef struct st_XSAVEGAME_READCONTEXT
{
} XSAVEGAME_READCONTEXT;

typedef struct st_XSAVEGAME_WRITECONTEXT
{
} XSAVEGAME_WRITECONTEXT;

typedef S32 (*XSAVEGAME_CLIENT_SAVEINFO)(void* cltdata, XSAVEGAME_DATA* xsg, S32* cur_space,
                                            S32* max_fullgame);
typedef S32 (*XSAVEGAME_CLIENT_SAVEPROC)(void* cltdata, XSAVEGAME_DATA* xsg,
                                            XSAVEGAME_WRITECONTEXT* wctxt);
typedef S32 (*XSAVEGAME_CLIENT_LOADPROC)(void* cltdata, XSAVEGAME_DATA* xsg,
                                            XSAVEGAME_READCONTEXT* rctxt, U32 idtag, S32);

typedef struct st_XSAVEGAME_CLIENT
{
    U32 idtag;
    XSAVEGAME_CLIENT_SAVEINFO cltinfo;
    XSAVEGAME_CLIENT_SAVEPROC cltproc;
    XSAVEGAME_CLIENT_LOADPROC cltload;
    void* cltdata;
    S32 needamt;
    S32 maxamt;
    S32 realamt;
    char* buf_sizepos;
    char* buf_maxpos;
    S32 blokact;
    S32 blokmax;
    char* blokpos;
    char* readpos;
    S32 readamt;
    S32 readremain;
} XSAVEGAME_CLIENT;

struct st_XSAVEGAME_DATA
{
    S32 gfile_idx;
    SAVEGAME_MODE mode;
    U32 stage;
    S32 gslot;
    char label[64];
    S32 progress;
    S32 thumbIconIdx;
    iTime playtime;
    char* membuf;
    S32 memsize;
    char* buf_curpos;
    char* buf_sizespot;
    char* buf_cksmspot;
    S32 totamt;
    U32 chksum;
    U32 upd_tally;
    S32 cltneed;
    S32 cltmax;
    S32 chdrneed;
    S32 stkcnt;
    XSAVEGAME_CLIENT cltstk[128];
    XSAVEGAME_CLIENT dfltloadclt;
    U32 file_chksum;
    U32 read_chksum;
    S32 readsize;
    char* loadbuf;
    S32 loadsize;
    char* walkpos;
    S32 walkremain;
    ISGSESSION* isgsess;
};

S32 xSGAddSaveClient(XSAVEGAME_DATA* xsgdata, U32 clttag, void* cltdata,
                       XSAVEGAME_CLIENT_SAVEINFO infofunc, XSAVEGAME_CLIENT_SAVEPROC procfunc);
S32 xSGAddLoadClient(XSAVEGAME_DATA* xsgdata, U32 clttag, void* cltdata,
                       XSAVEGAME_CLIENT_LOADPROC loadfunc);
S32 xSGWriteData(XSAVEGAME_DATA* xsgdata, XSAVEGAME_WRITECONTEXT* wctxt, char* data, S32 elesiz,
                   S32 n);
S32 xSGWriteData(XSAVEGAME_DATA* xsgdata, XSAVEGAME_WRITECONTEXT* wctxt, char* data, S32 n);
S32 xSGWriteData(XSAVEGAME_DATA* xsgdata, XSAVEGAME_WRITECONTEXT* wctxt, S32* data, S32 n);
S32 xSGWriteData(XSAVEGAME_DATA* xsgdata, XSAVEGAME_WRITECONTEXT* wctxt, F32* data, S32 n);
S32 xSGReadData(XSAVEGAME_DATA* xsgdata, XSAVEGAME_READCONTEXT* rctxt, char* buff, S32 elesiz,
                  S32 n);
S32 xSGReadData(XSAVEGAME_DATA* xsgdata, XSAVEGAME_READCONTEXT* rctxt, char* buff, S32 n);
S32 xSGReadData(XSAVEGAME_DATA* xsgdata, XSAVEGAME_READCONTEXT* rctxt, S32* buff, S32 n);
S32 xSGReadData(XSAVEGAME_DATA* xsgdata, XSAVEGAME_READCONTEXT* rctxt, F32* buff, S32 n);

#endif