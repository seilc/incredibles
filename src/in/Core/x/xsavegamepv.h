#ifndef XSAVEGAMEPV_H
#define XSAVEGAMEPV_H

#include "xsavegame.h"

#define XSG_MAX_CLIENT 128
#define XSG_MAXLEN_LABEL (64)

struct st_XSAVEGAME_CLIENT
{
    U32 idtag;
    XSGCltInfoFunc cltinfo;
    XSGCltProcFunc cltproc;
    XSGCltLoadFunc cltload;
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
};

struct st_XSAVEGAME_DATA
{
    S32 gfile_idx;
    en_SAVEGAME_MODE mode;
    U32 stage;
    S32 gslot;
    char label[XSG_MAXLEN_LABEL];
    S32 progress;
    S32 thumbIconIdx;
    S64 playtime;
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
    st_XSAVEGAME_CLIENT cltstk[XSG_MAX_CLIENT];
    st_XSAVEGAME_CLIENT dfltloadclt;
    U32 file_chksum;
    U32 read_chksum;
    S32 readsize;
    char* loadbuf;
    S32 loadsize;
    char* walkpos;
    S32 walkremain;
    st_ISGSESSION* isgsess;
};

#endif
