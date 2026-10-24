#ifndef ISAVEGAME_H
#define ISAVEGAME_H

#include "types.h"

struct st_ISG_MEMCARD_DATA
{
#ifdef PS2
    S32 mcport;
    S32 mcslot;
    S32 mcfp;
    en_ISG_IOMODE fmode;
    S8 gamepath[64];
    sceMcTblGetDir finfo;
    S32 cur_mcop;
    en_ISGMC_ERRSTATUS mcerr;
    S32 allow_cache;
#endif
};

typedef struct st_ISGSESSION
{
#ifdef PS2
    st_ISG_MEMCARD_DATA* mcdata;
    S8 gameroot[64];
    S8 gamedir[64];
    en_ASYNC_OPCODE as_curop;
    en_ASYNC_OPSTAT as_opstat;
    en_ASYNC_OPERR as_operr;
    void* cltdata;
    en_CHGCODE chgcode;
    void (*chgfunc)(void*, en_CHGCODE);
#endif
} ISGSESSION;

#endif