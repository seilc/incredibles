#ifndef XSAVEGAME_H
#define XSAVEGAME_H

#include "isavegame.h"

struct st_XSAVEGAME_CLIENT;
struct st_XSAVEGAME_DATA;
struct st_XSAVEGAME_READCONTEXT;
struct st_XSAVEGAME_WRITECONTEXT;

enum en_SAVEGAME_MODE
{
    XSG_MODE_LOAD = 10,
    XSG_MODE_SAVE
};

enum en_XSG_TGT_NAME_FORMAT
{
    XSG_NAMFMT_BASIC,
    XSG_NAMFMT_SYMBOLS,
    XSG_NAMFMT_PROPER,
    XSG_NAMFMT_PHYSSLOT,
    XSG_NAMFMT_PHYSICAL
};

enum en_XSGASYNC_STATUS
{
    XSG_ASTAT_NOOP,
    XSG_ASTAT_INPROG,
    XSG_ASTAT_SUCCESS,
    XSG_ASTAT_FAILED
};

enum en_XSG_WHYFAIL
{
    XSG_WHYERR_NONE,
    XSG_WHYERR_NOCARD,
    XSG_WHYERR_NOROOM,
    XSG_WHYERR_DAMAGE,
    XSG_WHYERR_CARDYANKED,
    XSG_WHYERR_OTHER,
    XSG_WHYERR_NOMORE
};

typedef S32(*XSGCltInfoFunc)(void*, st_XSAVEGAME_DATA*, S32*, S32*);
typedef S32(*XSGCltProcFunc)(void*, st_XSAVEGAME_DATA*, st_XSAVEGAME_WRITECONTEXT*);
typedef S32(*XSGCltLoadFunc)(void*, st_XSAVEGAME_DATA*, st_XSAVEGAME_READCONTEXT*, U32, S32);

struct XSGAutoData
{
    S32 IsValid();
    void MarkInvalid();
    S32 SetCache(S32 targ, S32 game, S32 physicalSlot);
    void Discard();
    st_ISGSESSION* HWConnect(S32 idx_target);
    void HWDisconnect(st_ISGSESSION* isg);
    S32 HWCheckConnect(S32);
    void Refresh();

    friend void xSGAutoSave_Startup();

protected:
    S32 flg_autodata;
    S32 lastTarg;
    S32 lastGame;
    S32 lastPhysicalSlot;
    st_ISGSESSION* isg_monitor;
};

S32 xSGStartup();
S32 xSGShutdown();
st_XSAVEGAME_DATA* xSGInit(en_SAVEGAME_MODE mode);
S32 xSGDone(st_XSAVEGAME_DATA* xsgdata);
S32 xSGCheckForCorruptFiles(st_XSAVEGAME_DATA* xsgdata, char (*fileName)[64]);
S32 xSGTgtCount(st_XSAVEGAME_DATA* xsgdata, S32* max);
S32 xSGTgtPhysSlotIdx(st_XSAVEGAME_DATA* xsgdata, S32 tidx);
S32 xSGTgtIsFormat(st_XSAVEGAME_DATA* xsgdata, S32 tidx, S32* badEncode);
S32 xSGTgtFormatTgt(st_XSAVEGAME_DATA* xsgdata, S32 tidx, S32* canRecover);
S32 xSGTgtSelect(st_XSAVEGAME_DATA* xsgdata, S32 tidx);
S32 xSGTgtHasGameDir(st_XSAVEGAME_DATA* xsgdata, S32 tidx);
S32 xSGTgtHaveRoom(st_XSAVEGAME_DATA* xsgdata, S32 tidx, S32 fsize, S32 slotidx, S32* bytesNeeded, S32* availOnDisk,
                   S32* needFile);
S32 xSGTgtHaveRoomStartup(st_XSAVEGAME_DATA* xsgdata, S32 tidx, S32 fsize, S32 slotidx, S32* bytesNeeded,
                          S32* availOnDisk, S32* needFile);
bool xSGCheckMemoryCard(st_XSAVEGAME_DATA* xsgdata, S32 index);
void xSGGameSet(st_XSAVEGAME_DATA* xsgdata, S32 gidx);
S32 xSGGameIsEmpty(st_XSAVEGAME_DATA* xsgdata, S32 gidx);
S32 xSGGameSize(st_XSAVEGAME_DATA* xsgdata, S32 gidx);
char* xSGGameModDate(st_XSAVEGAME_DATA* xsgdata, S32 gidx);
char* xSGGameLabel(st_XSAVEGAME_DATA* xsgdata, S32 gidx);
S32 xSGGameThumbIndex(st_XSAVEGAME_DATA* xsgdata, S32 gidx);
S32 xSGGameProgress(st_XSAVEGAME_DATA* xsgdata, S32 gidx);
S32 xSGAddSaveClient(st_XSAVEGAME_DATA* xsgdata, U32 clttag, void* cltdata, XSGCltInfoFunc infofunc,
                     XSGCltProcFunc procfunc);
S32 xSGAddLoadClient(st_XSAVEGAME_DATA* xsgdata, U32 clttag, void* cltdata, XSGCltLoadFunc loadfunc);
S32 xSGSetup(st_XSAVEGAME_DATA* xsgdata);
S32 xSGSetup(st_XSAVEGAME_DATA* xsgdata, S32 gidx, char* label, S32 progress, S64 playtime, S32 thumbIconIdx);
S32 xSGProcess(st_XSAVEGAME_DATA* xsgdata);
S32 xSGWrapup(st_XSAVEGAME_DATA* xsgdata);
en_XSGASYNC_STATUS xSGAsyncStatus(st_XSAVEGAME_DATA* xsgdata, S32 block, en_XSG_WHYFAIL* whyFail, char* errmsg);
S32 xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, char* data, S32 elesiz, S32 n);
S32 xSGWriteStrLen(const char* str);
S32 xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, char* data, S32 n);
S32 xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, S32* data, S32 n);
S32 xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, U32* data, S32 n);
S32 xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, F32* data, S32 n);
S32 xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, char* buff, S32 elesiz, S32 n);
S32 xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, char* buff, S32 n);
S32 xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, S32* buff, S32 n);
S32 xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, U32* buff, S32 n);
S32 xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, F32* buff, S32 n);
S32 xSG_chdir_gamedir(st_XSAVEGAME_DATA* xsgdata);
char* xSG_cm_slotname(st_XSAVEGAME_DATA* xsgdata, S32 slotidx);
S32 xSG_sv_flipinfo(st_XSAVEGAME_DATA* xsgdata);
S32 xSG_sv_prepdest(st_XSAVEGAME_DATA* xsgdata);
S32 xSG_sv_flipproc(st_XSAVEGAME_DATA* xsgdata);
S32 xSG_sv_bldchksum(st_XSAVEGAME_DATA* xsgdata);
S32 xSG_smem_blkopen(st_XSAVEGAME_DATA* xsgdata);
S32 xSG_smem_blkclose(st_XSAVEGAME_DATA* xsgdata);
S32 xSG_smem_cltopen(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_CLIENT* clt);
S32 xSG_smem_cltclose(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_CLIENT* clt);
S32 xSG_sv_commit(st_XSAVEGAME_DATA* xsgdata);
void xSG_cb_ISGChange(void* cltdata, en_CHGCODE what);
S32 xSG_ld_prepload(st_XSAVEGAME_DATA* xsgdata);
S32 xSG_ld_readgame(st_XSAVEGAME_DATA* xsgdata);
S32 xSG_ld_readhead(st_XSAVEGAME_DATA* xsgdata);
S32 xSG_ld_validate(st_XSAVEGAME_DATA* xsgdata);
S32 xSG_ld_findcltblk(st_XSAVEGAME_DATA* xsgdata);
S32 xSG_ld_flipload(st_XSAVEGAME_DATA* xsgdata);
XSGAutoData* xSGAutoSave_GetCache();
void xSGAutoSave_Startup();

#endif
