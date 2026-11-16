#include "xCommon.h"
#include "isavegame.h"

#include "xMath.h"
#include "xRegionSupport.h"
#include "xstransvc.h"
#include "xSndMgr.h"
#include "iFile.h"
#include "iTRC.h"
#include "zGlobals.h"
#include "zMain.h"

#include "decomp.h"

#include <dolphin.h>
#include <stdio.h>
#include <string.h>

#define ISG_MAX_TARGET 2
#define ISG_MAX_SAVEGAME 3

#define ISG_ROUNDUP(n, a) (((n) + (a) - 1) & -(a))

enum en_ISGMC_ERRSTATUS
{
    ISGMC_ERR_NONE,
    ISGMC_ERR_NOMEMCARD,
    ISGMC_ERR_MKDIR,
    ISGMC_ERR_OPEN,
    ISGMC_ERR_CLOSE,
    ISGMC_ERR_READ,
    ISGMC_ERR_WRITE
};

struct st_ISG_MEMCARD_DATA
{
    S32 ismounted;
    S32 myphys;
    S32 sectsize;
    CARDFileInfo cur_finf;
    CARDStat cur_stat;
    S64 cur_ftime;
    S32 off_nextwrite;
    CARDFileInfo finfcache;
    CARDStat statcache;
    S64 timecache;
    en_ISGMC_ERRSTATUS mcerr;
    S32 cardIsIll;
};

enum en_ISG_WHYFAIL
{
    ISG_WHYERR_NONE,
    ISG_WHYERR_NOCARD,
    ISG_WHYERR_NOROOM,
    ISG_WHYERR_DAMAGE,
    ISG_WHYERR_OTHER,
    ISG_WHYERR_NOMORE
};

struct st_ISGSESSION
{
    st_ISG_MEMCARD_DATA physcard[ISG_MAX_TARGET];
    S32 idx_actvcard;
    en_CHGCODE chgcode;
    en_ASYNC_OPERR as_operr;
    en_ASYNC_OPSTAT as_fakestat;
    en_ISG_WHYFAIL whyFail;
    ISGChangeCallback chgfunc;
    void* cltdata;
};

struct st_ISG_TPL_TEXHEADER
{
    U16 height;
    U16 width;
    u32 format;
    char* data;
    GXTexWrapMode wrapS;
    GXTexWrapMode wrapT;
    GXTexFilter minFilter;
    GXTexFilter magFilter;
    F32 LODBias;
    U8 edgeLODEnable;
    U8 minLOD;
    U8 maxLOD;
    U8 unpacked;
};

struct st_ISG_TPL_CLUTHEADER
{
    U16 numEntries;
    U8 unpacked;
    U8 pad8;
    GXTlutFmt format;
    char* data;
};

struct st_ISG_TPL_TEXDESCRIP
{
    st_ISG_TPL_TEXHEADER* textureHeader;
    st_ISG_TPL_CLUTHEADER* CLUTHeader;
};

struct st_ISG_TPL_TEXPALETTE
{
    u32 versionNumber;
    u32 numDescriptors;
    st_ISG_TPL_TEXDESCRIP* descriptorArray;
};

struct st_ISG_CARD_ICONBLOCK
{
    U8 Title[32];
    U8 Description[32];
    U8 Banner[6144];
    U8 Icons[8][2048];
    U8 buncho[512];
    char integchk[512];
};

enum en_ISG_IOMODE
{
    ISG_IOMODE_READ = 1,
    ISG_IOMODE_WRITE,
    ISG_IOMODE_APPEND
};

static char* ISG_GC_GAMENAME = "GICE";
static S32 g_isginit;
static st_ISGSESSION g_isgdata_MAIN = { 0 };
static char cardwork[ISG_MAX_TARGET][CARD_WORKAREA_SIZE] ATTRIBUTE_ALIGN(32);
static char* g_rawicon;
static char* g_rawbanr;
static S32 g_iconsize;
static S32 g_banrsize;
static S32 g_legalSectSize[3] = { 0x2000, 0, -1 };
static bool isMounted;

static S32 iSG_start_your_engines();
static S32 iSG_mc_exists(S32 cardslot);
static S32 iSG_mc_isformatted(st_ISG_MEMCARD_DATA* mcdata);
static S32 iSG_mc_isGCcard(st_ISG_MEMCARD_DATA* mcdata, S32* badencode, S32* wornout);
static S32 iSG_isSpaceForFile(st_ISG_MEMCARD_DATA* mcdata, S32 fsize, const char* fname, S32* amtneed, S32* amtfree,
                              S32* needFiles);
static S32 iSG_mc_settgt(st_ISG_MEMCARD_DATA* mcdata, S32 cardslot);
static S32 iSG_get_finfo(st_ISG_MEMCARD_DATA* mcdata, const char* fname);
static S32 iSG_get_fsize(st_ISG_MEMCARD_DATA* mcdata, const char* fname);
static S32 iSG_get_fmoddate(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32* sec, S32* min, S32* hr, S32* mon,
                            S32* day, S32* yr);
static S32 iSG_cubeicon_size(S32 cardslot, S32 sectsize);
static void iSG_timestamp(CARDStat*);
static char* iSG_bfr_icondata(char* buffer, CARDStat*, char* label, S32 sectsize);
static void iSG_upd_icostat(CARDStat*, CARDStat* realstat);
static S32 iSG_mc_fopen(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32 fsize, en_ISG_IOMODE mode,
                        en_ASYNC_OPERR* whyFail);
static S32 iSG_mc_fclose(st_ISG_MEMCARD_DATA* mcdata, CARDStat* statcopy);
static S32 iSG_mc_fdel(st_ISG_MEMCARD_DATA* mcdata, const char* fname);
static S32 iSG_mc_fclose(st_ISG_MEMCARD_DATA* mcdata);
static S32 iSG_mc_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize, S32 skipbyte);
static S32 iSG_mcqa_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize, S32 skipbyte);
static S32 iSG_mc_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* data, S32 n);
static S32 iSG_mcqa_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* data, S32 n);
static void iSG_cb_asyndone(s32, s32 retcode);
static S32 iSG_curKosher(CARDStat* stat, CARDFileInfo* finf);
static S32 iSG_load_icondata();
static S32 iSG_chk_icondata();
static void iSG_discard_icondata();
static S32 iSG_tpl_unpack(st_ISG_TPL_TEXPALETTE* pal);
static st_ISG_TPL_TEXDESCRIP* iSG_tpl_TEXGet(st_ISG_TPL_TEXPALETTE* pal, U32 idx);
static S32 iSG_bnr_unpack(st_ISG_TPL_TEXPALETTE* pal);
static S32 iSG_mc_mount(S32 cardslot);
static S32 iSG_mc_unmount(S32 cardslot);
static void iSG_cb_unmount(s32 chan, s32 retcode);
static S32 iSG_mc_format(st_ISG_MEMCARD_DATA* mcdata, S32 force, S32* canRecover);

S32 iSGStartup()
{
    if (g_isginit++) return g_isginit;

    iSG_start_your_engines();

    iSG_load_icondata();

    return g_isginit;
}

S32 iSGShutdown()
{
    iSG_discard_icondata();

    return 1;
}

char* iSGMakeName(en_NAMEGEN_TYPE type, const char* base, S32 idx)
{
    static char rotatebuf[8][32] = { 0 };
    static S32 rotate = 0;
    char* use_buf;
    const char* fmt_sb = "%s%08s";
    const char* fmt_sd = "%s%02d";
    const char* fmt_sbd = "%s%06s%02d";

    use_buf = rotatebuf[rotate++];
    if (rotate == 8) rotate = 0;

    use_buf[0] = '\0';

    switch (type) {
    case ISG_NGTYP_GAMEFILE:
        if (base) sprintf(use_buf, fmt_sd, base, idx + 1);
        else sprintf(use_buf, fmt_sd, "Incredibles", idx + 1);
        break;
    case ISG_NGTYP_GAMEDIR:
    case ISG_NGTYP_CONFIG:
    case ISG_NGTYP_ICONTHUM:
        break;
    }

    return use_buf;
}

st_ISGSESSION* iSGSessionBegin(void* cltdata, ISGChangeCallback chgfunc, S32)
{
    st_ISGSESSION* isgdata = NULL;

    isgdata = &g_isgdata_MAIN;

    iTRC_CHECK();

    memset(isgdata, 0, sizeof(st_ISGSESSION));
    isgdata->idx_actvcard = -1;
    isgdata->chgfunc = chgfunc;
    isgdata->cltdata = cltdata;
    iSG_chk_icondata();

    return isgdata;
}

void iSGSessionEnd(st_ISGSESSION* isgdata)
{
    S32 i = 0;
    st_ISG_MEMCARD_DATA* mcd = NULL;

    iTRC_CHECK();

    for (i = 0; i < ISG_MAX_TARGET; i++) {
        mcd = &isgdata->physcard[i];
        if (!mcd->ismounted) continue;
        iSG_mc_unmount(i);
        mcd->ismounted = 0;
    }

    memset(isgdata, 0, sizeof(st_ISGSESSION));
}

S32 iSGTgtCount(st_ISGSESSION*, S32* max)
{
    S32 cnt = 0;
    S32 rc = 0;
    s32 memsize = 0;
    s32 sectorsize = 0;
    S32 i = 0;

    if (max) *max = ISG_MAX_TARGET;

    for (i = 0; i < ISG_MAX_TARGET; i++) {
        iTRC_CHECK();

        do {
            rc = CARDProbeEx(i, &memsize, &sectorsize);
        } while (rc == CARD_RESULT_BUSY);

        if (rc == CARD_RESULT_READY) {
            cnt++;
        }
    }

    return cnt;
}

S32 iSGTgtPhysSlotIdx(st_ISGSESSION*, S32 tidx)
{
    S32 physslot = -1;
    S32 rc = 0;

    rc = iSG_mcidx2slot(tidx, &physslot, NULL);
    if (rc) return physslot;
    return -1;
}

DECOMP_FORCEACTIVE("Slot %c", "Memory Card Slot %c", "Nintendo GameCube Memory Card in Slot %c",
                   "Nintendo GameCube%s Memory Card in Slot %c", "^", "Nintendo GameCube%s Memory Card");

S32 iSGTgtFormat(st_ISGSESSION* isgdata, S32 tgtidx, S32, S32* canRecover)
{
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 slot = 0;
    S32 rc = 0;

    iTRC_CHECK();

    rc = iSG_mc_exists(isgdata->idx_actvcard);
    if (!rc) return 0;

    rc = iSG_mcidx2slot(tgtidx, &slot, NULL);

    if (slot != isgdata->idx_actvcard) return 0;

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    if (!mcdata->ismounted) return 0;

    rc = iSG_mc_isGCcard(mcdata, NULL, NULL);
    if (!rc) return 0;

    rc = iSG_mc_format(mcdata, 0, canRecover);

    if (rc == -1) {
        return -1;
    } else if (rc == 0) {
        return 0;
    }

    return 1;
}

U32 iSGTgtState(st_ISGSESSION* isgdata, S32 tgtidx, const char*)
{
    U32 state = 0;
    S32 rc = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 physslot = 0;
    S32 prob_encode = 0;
    S32 prob_wornout = 0;
    S32 i = 0;
    S32 isok = 0;

    iTRC_CHECK();

    rc = iSG_mcidx2slot(tgtidx, &physslot, NULL);
    if (physslot != isgdata->idx_actvcard) {
        rc = iSGTgtSetActive(isgdata, tgtidx);
    }

    if (isgdata->idx_actvcard < 0) {
        state |= 0x1000000;
        return state;
    }

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];
    if (!mcdata->ismounted) {
        return state;
    }

    rc = iSG_mc_exists(isgdata->idx_actvcard);
    if (rc) state |= 0x1;
    else return state;

    if (mcdata->cardIsIll) {
        state |= 0x1000000;
        return state;
    }

    rc = iSG_mc_isGCcard(mcdata, &prob_encode, &prob_wornout);
    if (prob_encode) {
        state |= 0x4000000;
    }
    if (prob_wornout) {
        state |= 0x2000000;
    }
    if (rc) {
        state |= 0x4;
    } else {
        return state;
    }

    if (g_legalSectSize[0] > 0) {
        i = 0;
        while (g_legalSectSize[i] > 0) {
            if (mcdata->sectsize == g_legalSectSize[i]) {
                isok = 1;
                break;
            }
            i++;
        }
        if (!isok) {
            state |= 0x8000000;
            state &= ~0x4;
            return state;
        }
    }

    rc = iSG_mc_isformatted(mcdata);
    if (rc) state |= 0xA;
    else return state;

    return state;
}

DECOMP_FORCEACTIVE("is damaged and cannot be used", "unsupprted sector size", "is formatted for another market",
                   "encountered unexpected error (%d)");

S32 iSGTgtSetActive(st_ISGSESSION* isgdata, S32 tgtidx)
{
    S32 result = 0;
    S32 rc = 0;
    S32 slot = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;

    rc = iSG_mcidx2slot(tgtidx, &slot, NULL);
    if (!rc) return result;

    mcdata = &isgdata->physcard[slot];

    iTRC_CHECK();

    if (mcdata->ismounted) result = 1;
    else {
        rc = iSG_mc_settgt(mcdata, slot);
        if (rc) {
            isgdata->idx_actvcard = slot;
            result = 1;
        }
    }

    return result;
}

S32 iSGTgtHaveRoom(st_ISGSESSION* isgdata, S32, S32 fsize, const char*, const char* fname, S32* needbyte,
                   S32* availbyte, S32* needFile)
{
    S32 result = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 i = 0;

    if (isgdata->idx_actvcard < 0) return result;

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    if (!mcdata->ismounted) return result;

    S32 gameFileCount = 0;
    if (!fname) {
        for (i = 0; i < ISG_MAX_SAVEGAME; i++) {
            iTRC_CHECK();

            fname = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i);
            if (iSG_get_finfo(mcdata, fname)) {
                gameFileCount++;
            }
        }
    }

    result = iSG_isSpaceForFile(mcdata, fsize, fname, needbyte, availbyte, needFile);

    if (gameFileCount > 0 && *needbyte > *availbyte) {
        if (needFile && *needbyte > *availbyte) {
            *needFile = 0;
            return 0;
        }
        return 1;
    }

    S32 fileCount = 0;
    for (i = 0; i < CARD_MAX_FILE && isgdata->idx_actvcard >= 0 && isgdata->idx_actvcard < ISG_MAX_TARGET; i++) {
        CARDFileInfo fileInfo;
        S32 gcResult = CARDFastOpen(isgdata->idx_actvcard, i, &fileInfo);
        if (gcResult == CARD_RESULT_READY) {
            fileCount++;
            CARDClose(&fileInfo);
        }
    }

    if (fileCount >= CARD_MAX_FILE) {
        if (gameFileCount > 0) {
            if (needFile) {
                *needFile = 0;
            }
            return 1;
        }
        return 0;
    }

    if (*needbyte > *availbyte) {
        return 0;
    }

    return result;
}

S32 iSGTgtHaveRoomStartup(st_ISGSESSION* isgdata, S32, S32 fsize, const char*, const char* fname, S32* needbyte,
                          S32* availbyte, S32* needFile)
{
    S32 result = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 i = 0;

    if (isgdata->idx_actvcard < 0) return result;

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    if (!mcdata->ismounted) return result;

    S32 gameFileCount = 0;
    if (!fname) {
        for (i = 0; i < ISG_MAX_SAVEGAME; i++) {
            iTRC_CHECK();

            fname = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i);
            if (iSG_get_finfo(mcdata, fname)) {
                gameFileCount++;
            }
        }
    }

    result = iSG_isSpaceForFile(mcdata, fsize, fname, needbyte, availbyte, needFile);

    if (gameFileCount > 0 && *needbyte > *availbyte) {
        if (needFile && *needbyte > *availbyte) {
            *needFile = 0;
            return 0;
        }
        return 1;
    }

    S32 fileCount = 0;
    for (i = 0; i < CARD_MAX_FILE && isgdata->idx_actvcard >= 0 && isgdata->idx_actvcard < ISG_MAX_TARGET; i++) {
        CARDFileInfo fileInfo;
        S32 gcResult = CARDFastOpen(isgdata->idx_actvcard, i, &fileInfo);
        if (gcResult == CARD_RESULT_READY) {
            fileCount++;
            CARDClose(&fileInfo);
        }
    }

    if (fileCount >= CARD_MAX_FILE) {
        if (gameFileCount > 0) {
            if (needFile) {
                *needFile = 0;
            }
            return 0;
        }
        *needFile = -1;
        return 0;
    }

    if (*needbyte > *availbyte) {
        return 0;
    }

    if (gameFileCount >= ISG_MAX_SAVEGAME) {
        return 0;
    }

    return result;
}

S32 iSGFileSize(st_ISGSESSION* isgdata, const char* fname)
{
    S32 size = -1;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 skipsize = 0;

    if (isgdata->idx_actvcard < 0) return size;

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    iTRC_CHECK();

    size = iSG_get_fsize(mcdata, fname);

    if (size >= 0) {
        skipsize = iSG_cubeicon_size(mcdata->myphys, mcdata->sectsize);
        size -= skipsize;
        if (size < 0) size = -1;
    }

    return size;
}

const char* iSGFileModDate(st_ISGSESSION* isgdata, const char* fname)
{
    const char* date_str = NULL;
    date_str = iSGFileModDate(isgdata, fname, NULL, NULL, NULL, NULL, NULL, NULL);
    return date_str;
}

char* iSGFileModDate(st_ISGSESSION* isgdata, const char* fname, S32* usr_sec, S32* usr_min, S32* usr_hr,
                     S32* usr_mon, S32* usr_day, S32* usr_yr)
{
    static char datestr[64] = { 0 };
    S32 rc = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 sec = 0, min = 0, hr = 0, mon = 0, day = 0, yr = 0;

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    rc = iSG_get_fmoddate(mcdata, fname, &sec, &min, &hr, &mon, &day, &yr);
    if (rc) {
        if (zMainIsPAL())
            sprintf(datestr, "%02d.%02d.%04d %02d:%02d:%02d", day, mon, yr, hr, min, sec);
        else
            sprintf(datestr, "%02d/%02d/%04d %02d:%02d:%02d", mon, day, yr, hr, min, sec);

        if (usr_sec) *usr_sec = sec;
        if (usr_min) *usr_min = min;
        if (usr_hr) *usr_hr = hr;
        if (usr_mon) *usr_mon = mon;
        if (usr_day) *usr_day = day;
        if (usr_yr) *usr_yr = yr;
    } else {
        sprintf(datestr, "<Unknown Modification>");
    }

    return datestr;
}

void iSGMakeTimeStamp(char* str)
{
    OSCalendarTime caltym = { 0 };

    OSTicksToCalendarTime(OSGetTime(), &caltym);

    if (zMainIsPAL())
        sprintf(str, "%02d.%02d.%04d %02d:%02d:%02d", caltym.mday, caltym.mon + 1, caltym.year, caltym.hour,
                caltym.min, caltym.sec);
    else
        sprintf(str, "%02d/%02d/%04d %02d:%02d:%02d", caltym.mon + 1, caltym.mday, caltym.year, caltym.hour,
                caltym.min, caltym.sec);
}

S32 iSGSelectGameDir(st_ISGSESSION* isgdata, const char*)
{
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 rc = 0;

    if (isgdata->idx_actvcard < 0) return 0;
    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    if (!mcdata->ismounted) return 0;

    rc = iSG_mc_isGCcard(mcdata, NULL, NULL);
    if (!rc) return 0;

    rc = iSG_mc_isformatted(mcdata);
    if (!rc) return 0;

    S32 fileCount = 0;

    for (S32 i = 0; i < ISG_MAX_SAVEGAME; i++) {
        const char* fname = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i);
        if (iSG_get_finfo(mcdata, fname)) {
            fileCount++;
        }
    }
    if (!fileCount) {
        return 0;
    }

    return 1;
}

S32 iSGSetupGameDir(st_ISGSESSION*, const char*, S32)
{
    S32 result = 1;
    return result;
}

S32 iSGSaveFile(st_ISGSESSION* isgdata, const char* fname, char* data, S32 n, S32, char* label)
{
    S32 result = 0;
    S32 rc = 0;
    en_ASYNC_OPERR whyFail = ISG_OPERR_NONE;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    CARDStat dumystat = { 0 };
    CARDStat poststat = { 0 };
    char* fulbuf;
    S32 fulsiz = 0;
    char* bufptr;
    S32 bufsiz;
    char* runptr;
    S32 iconpart = 0;
    S32 sizemult = 0;

    ResetButton::DisableReset();

    if (isgdata->idx_actvcard < 0) {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_NOCARD;
        return result;
    }

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    if (mcdata->cardIsIll) {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_DAMAGE;
        return result;
    }

    iTRC_CHECK();

    iconpart = iSG_cubeicon_size(isgdata->idx_actvcard, mcdata->sectsize);

    sizemult = ISG_ROUNDUP(512, mcdata->sectsize);
    bufsiz = iconpart + ISG_ROUNDUP(n, sizemult);
    fulsiz = bufsiz + 31;

    fulbuf = (char*)xMEMPUSHTEMP(fulsiz);
    memset(fulbuf, 0, fulsiz);

    bufptr = (char*)OSRoundUp32B(fulbuf);

    runptr = iSG_bfr_icondata(bufptr, &dumystat, label, mcdata->sectsize);

    memcpy(runptr, data, n);

    iTRC_CHECK();

    rc = iSG_mc_fopen(mcdata, fname, bufsiz, ISG_IOMODE_WRITE, &whyFail);
    if (rc) {
        rc = iSG_mc_fwrite(mcdata, bufptr, bufsiz);
        if (rc) result = 1;
        else {
            whyFail = ISG_OPERR_SVWRITE; result = 0;
        }

        if (result) {
            iSG_upd_icostat(&dumystat, &mcdata->cur_stat);

            iSG_timestamp(&mcdata->cur_stat);

            do {
                rc = CARDSetStatus(mcdata->myphys, mcdata->cur_finf.fileNo, &mcdata->cur_stat);
            } while (rc == CARD_RESULT_BUSY);

            if (rc != CARD_RESULT_READY) {
                result = 0;
            }
        }

        if (result) {
            U8 attrib = CARD_ATTR_PUBLIC;
            do {
                rc = CARDSetAttributes(mcdata->myphys, mcdata->cur_finf.fileNo, attrib);
            } while (rc == CARD_RESULT_BUSY);
        }

        iSG_mc_fclose(mcdata, &poststat);
    }

    xMEMPOPTEMP(fulbuf);

    if (!result && !mcdata->cardIsIll) {
        iSG_mc_fdel(mcdata, fname);
    }

    ResetButton::EnableReset();

    iTRC_CHECK();

    isgdata->as_operr = ISG_OPERR_NONE;
    if (result) isgdata->as_fakestat = ISG_OPSTAT_SUCCESS;
    else {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        if (whyFail) isgdata->as_operr = whyFail;
        else isgdata->as_operr = ISG_OPERR_UNKNOWN;
    }

    return result;
}

S32 iSGLoadFile(st_ISGSESSION* isgdata, const char* fname, char* databuf, S32 async)
{
    S32 result = 1;
    S32 size = 1;

    size = iSGFileSize(isgdata, fname);
    result = iSGReadLeader(isgdata, fname, databuf, size, async);
    return result;
}

S32 iSGReadLeader(st_ISGSESSION* isgdata, const char* fname, char* databuf, S32 numbytes, S32)
{
    S32 result = 0;
    S32 rc = 0;
    st_ISG_MEMCARD_DATA* mcdata = NULL;
    S32 offset = 0;
    char* fulbuf = NULL;
    S32 fulsiz = 0;
    char* bufptr = NULL;
    S32 bufsiz = 0;
    S32 sizemult = 0;
    en_ASYNC_OPERR whyFail = ISG_OPERR_NONE;

    if (isgdata->idx_actvcard < 0) {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_NOCARD;
        return result;
    }

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];

    if (mcdata->cardIsIll) {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        isgdata->as_operr = ISG_OPERR_DAMAGE;
        return result;
    }

    iTRC_CHECK();

    offset = iSG_cubeicon_size(mcdata->myphys, mcdata->sectsize);

    sizemult = ISG_ROUNDUP(512, mcdata->sectsize);
    if ((S32)databuf % 32 || numbytes % sizemult) {
        fulsiz = ISG_ROUNDUP(numbytes, 512) + 31;

        fulbuf = (char*)xMEMPUSHTEMP(fulsiz);
        memset(fulbuf, 0, fulsiz);

        bufptr = (char*)OSRoundUp32B(fulbuf);
        bufsiz = ISG_ROUNDUP(numbytes, 512);
    } else {
        bufptr = databuf;
        bufsiz = numbytes;
    }

    iTRC_CHECK();

    rc = iSG_mc_fopen(mcdata, fname, -1, ISG_IOMODE_READ, &whyFail);
    if (rc) {
        rc = iSG_mc_fread(mcdata, bufptr, bufsiz, offset);
        if (rc) result = 1;
        else result = 0;

        iSG_mc_fclose(mcdata);
    }

    if (result) {
        if (fulbuf) {
            memcpy(databuf, bufptr, numbytes);
        }
    }

    if (fulbuf) {
        xMEMPOPTEMP(fulbuf);
        fulbuf = NULL;
        bufsiz = 0;
    }

    isgdata->as_operr = ISG_OPERR_NONE;
    if (result) isgdata->as_fakestat = ISG_OPSTAT_SUCCESS;
    else {
        isgdata->as_fakestat = ISG_OPSTAT_FAILURE;
        if (mcdata->cardIsIll) isgdata->as_operr = ISG_OPERR_DAMAGE;
        else isgdata->as_operr = ISG_OPERR_OTHER;
    }

    return result;
}

en_ASYNC_OPSTAT iSGPollStatus(st_ISGSESSION* isgdata, en_ASYNC_OPCODE*, S32)
{
    return isgdata->as_fakestat;
}

en_ASYNC_OPERR iSGOpError(st_ISGSESSION* isgdata, char* errbuf)
{
    static char* errmsgs[ISG_OPERR_NOMORE + 1] = {
        "No current error",
        "No operation in async queue",
        "Too many async ops queued simultaneously",
        "Init Failed",
        "Unable to access Save Game Directory",
        "Access Error - no card ?!? (eg yanked out)",
        "Access Error - no room on card (file handles free bytes, etc)",
        "Access Error - card is damaged or something bad",
        "Access Error - file being loaded appears to be corrupt (I-Level)",
        "Access Error - general problem",
        "Save Error - Not enough free space to save file",
        "Save Error - during initalization (async queue)",
        "Save Error - during write",
        "Save Error - opening file",
        "Load Error - during initalization (async queue)",
        "Load Error - during read",
        "Load Error - opening file",
        "Target problem (general error)",
        "Target Error - media removed or changed",
        "Target Error - Not ready for I/O (unformatted?)",
        "Operation encountered unknown error",
        NULL
    };

    if (!errbuf) return isgdata->as_operr;

    if (isgdata->as_operr < ISG_OPERR_NOMORE) {
        strncpy(errbuf, errmsgs[isgdata->as_operr], 128);
    } else {
        strncpy(errbuf, errmsgs[ISG_OPERR_UNKNOWN], 128);
    }

    errbuf[127] = '\0';

    return isgdata->as_operr;
}

static S32 iSG_start_your_engines()
{
    S32 result = 1;

    CARDInit();

    return result;
}

S32 iSG_mcidx2slot(S32 mcidx, S32* slot, S32* concnt)
{
    S32 result = 0;
    S32 rc = 0;
    S32 idx = 0;
    s32 duma = 0;
    s32 dumb = 0;
    S32 haveit[ISG_MAX_TARGET] = { 0 };
    S32 i = 0;

    *slot = -1;

    for (i = 0; i < ISG_MAX_TARGET; i++) {
        do {
            rc = CARDProbeEx(i, &duma, &dumb);
        } while (rc == CARD_RESULT_BUSY);

        if (rc == CARD_RESULT_READY) haveit[i] = 1;
    }

    for (i = 0; i < ISG_MAX_TARGET; i++) {
        if (!haveit[i]) continue;
        if (idx == mcidx) {
            *slot = i;
            result = 1;
            break;
        }
        idx++;
    }

    if (concnt) {
        for (i = 0; i < ISG_MAX_TARGET; i++) {
            concnt[i] = haveit[i];
        }
    }

    return result;
}

static S32 iSG_mc_exists(S32 cardslot)
{
    S32 result = 0;
    S32 rc = 0;
    s32 memsize = 0;
    s32 sectsize = 0;

    if (cardslot < -1) return result;

    do {
        rc = CARDProbeEx(cardslot, &memsize, &sectsize);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;

    return result;
}

static S32 callbackResult;
static bool checkExInProgress;

void CheckExCallback(s32, s32 result)
{
    callbackResult = result; checkExInProgress = false;
}

inline S32 iSG_CARDCheckEx(S32 chan, s32* xferBytes)
{
    checkExInProgress = true;
    S32 rc = CARDCheckExAsync(chan, xferBytes, CheckExCallback);
    if (rc != CARD_RESULT_READY) {
        checkExInProgress = false;
        return rc;
    }

    while (checkExInProgress)
        iTRC_CHECK();
    return callbackResult;
}

static S32 iSG_mc_tryRepair(st_ISG_MEMCARD_DATA* mcdata)
{
    S32 result = 0;
    S32 rc = 0;
    s32 repaired = 0;

    if (!mcdata->ismounted) return result;

    if (mcdata->cardIsIll) return result;

    do {
        rc = iSG_CARDCheckEx(mcdata->myphys, &repaired);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;
    else if (rc == CARD_RESULT_ENCODING) result = 1;
    else {
        if (rc != CARD_RESULT_BROKEN) mcdata->cardIsIll = 1;

        result = 0;
    }

    return result;
}

static S32 iSG_mc_isformatted(st_ISG_MEMCARD_DATA* mcdata)
{
    S32 is_fmtd = 0;
    S32 rc = 0;
    s32 repaired = 0;

    if (!mcdata->ismounted) return is_fmtd;

    if (mcdata->cardIsIll) return is_fmtd;

    do {
        rc = CARDCheckEx(mcdata->myphys, &repaired);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) is_fmtd = 1;
    else if (rc == CARD_RESULT_BROKEN) is_fmtd = 0;
    else {
        is_fmtd = 0;
    }

    return is_fmtd;
}

static S32 iSG_mc_isGCcard(st_ISG_MEMCARD_DATA* mcdata, S32* badencode, S32* wornout)
{
    S32 is_gc = 0;
    S32 rc = 0;
    s32 repaired = 0;
    u16 typ_enc = 0;
    s32 duma = 0;
    s32 dumb = 0;

    if (badencode) *badencode = 0;
    if (wornout) *wornout = 0;

    if (!mcdata->ismounted) return is_gc;

    if (mcdata->cardIsIll) return is_gc;

    do {
        rc = CARDProbeEx(mcdata->myphys, &duma, &dumb);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) is_gc = 1;

    if (is_gc) {
        do {
            rc = CARDCheckEx(mcdata->myphys, &repaired);
        } while (rc == CARD_RESULT_BUSY);

        if (rc == CARD_RESULT_READY) is_gc = 1;
        else if (rc == CARD_RESULT_BROKEN) is_gc = 2;
        else if (rc == CARD_RESULT_ENCODING) {
            is_gc = 3;
            if (badencode) *badencode = 1;
        } else {
            is_gc = 0;
            if (rc == CARD_RESULT_ENCODING && badencode) *badencode = 1;
            if (rc == CARD_RESULT_IOERROR && wornout) *wornout = 1;
        }
    }

    if (is_gc == 1) {
        do {
            rc = CARDGetEncoding(mcdata->myphys, &typ_enc);
        } while (rc == CARD_RESULT_BUSY);
        if (rc == CARD_RESULT_READY) {
            u16 target_enc = CARD_ENCODE_ANSI;
            if (xSTGetLocalizationEnum() == eRegionJP) {
                target_enc = CARD_ENCODE_SJIS;
            }

            if (typ_enc != target_enc) {
                if (badencode) *badencode = 1;
            }
        }
    }

    return is_gc;
}

static S32 iSG_isSpaceForFile(st_ISG_MEMCARD_DATA* mcdata, S32 fsize, const char* fname, S32* amtneed, S32* amtfree,
                              S32* needFiles)
{
    S32 haveroom = 0;
    S32 needamt = 0;
    S32 rc = 0;
    s32 freebyte = 0;
    s32 freefile = 0;

    if (!mcdata->ismounted) {
        return haveroom;
    }

    needamt = iSG_cubeicon_size(mcdata->myphys, mcdata->sectsize);

    needamt += fsize;
    needamt = ISG_ROUNDUP(needamt, mcdata->sectsize);

    do {
        rc = CARDFreeBlocks(mcdata->myphys, &freebyte, &freefile);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) {
        if (amtfree) {
            *amtfree = freebyte / mcdata->sectsize;
        }
        if (amtneed) {
            *amtneed = needamt / mcdata->sectsize;
        }
    } else {
        return haveroom;
    }

    if (needFiles) *needFiles = 1;

    rc = iSG_get_finfo(mcdata, fname);
    if (rc) {
        if (needFiles) *needFiles = MAX(0, *needFiles - 1);

        if (needamt <= mcdata->statcache.length + freebyte) {
            haveroom = 1;
        }
    } else {
        if (needamt <= freebyte && freefile > 0) {
            haveroom = 1;
        }
    }

    return haveroom;
}

static S32 iSG_mc_settgt(st_ISG_MEMCARD_DATA* mcdata, S32 cardslot)
{
    S32 rc = 0;

    rc = iSG_mc_mount(cardslot);
    if (rc) {
        mcdata->ismounted = 1;
        mcdata->myphys = cardslot;

        S32 trc = iSG_mc_tryRepair(mcdata);

        rc = CARDGetSectorSize(cardslot, (u32*)&mcdata->sectsize);
    } else {
        mcdata->ismounted = 0;
    }

    return mcdata->ismounted ? 1 : 0;
}

static S32 iSG_get_finfo(st_ISG_MEMCARD_DATA* mcdata, const char* fname)
{
    S32 result = 0;
    S32 rc = 0;
    en_ASYNC_OPERR whyFail = ISG_OPERR_NONE;

    rc = iSG_mc_fopen(mcdata, fname, -1, ISG_IOMODE_READ, &whyFail);
    if (rc) {
        result = 1;
        memcpy(&mcdata->statcache, &mcdata->cur_stat, sizeof(CARDStat));

        memcpy(&mcdata->finfcache, &mcdata->cur_finf, sizeof(CARDFileInfo));

        iSG_mc_fclose(mcdata);
    }

    return result;
}

static S32 iSG_curKosher(CARDStat* stat, CARDFileInfo* finf)
{
    S32 isok = 1;

    if (!((stat->iconAddr >= 1 && stat->iconAddr <= 0x7FFFFFFF) ||
          (stat->commentAddr >= 1 && stat->commentAddr <= 0x7FFFFFFF))) {
        isok = 0;
    } else {
        S32 bs = 0;
        char* b = NULL;
        char* ba = NULL;
        s32 ik = 0;
        st_ISG_CARD_ICONBLOCK* blkmap = NULL;
        char tb[512] = { 0 };
        S32 ak;

        sprintf(tb, "The Incredibles");

        bs = sizeof(st_ISG_CARD_ICONBLOCK);
        bs = ISG_ROUNDUP(bs, 512);
        b = (char*)xMEMPUSHTEMP(bs + 31);
        ba = (char*)OSRoundUp32B(b);

        do {
            ik = CARDRead(finf, ba, bs, 0);
        } while (ik == CARD_RESULT_BUSY);

        if (ik == CARD_RESULT_READY) {
            blkmap = (st_ISG_CARD_ICONBLOCK*)ba;
            ak = memcmp(blkmap->integchk, tb, strlen(tb) - 1);
            if (ak) {
                isok = 0;
            }

            ak = memcmp(blkmap->Title, "The Incredibles", strlen("The Incredibles") - 1);
            if (ak) {
                isok = 0;
            }
        }

        xMEMPOPTEMP(b);
    }

    return isok;
}

static S32 iSG_fileKosher(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32 tossRotten, S32* didit)
{
    S32 result = -1;
    S32 rc = 0;
    CARDStat* stat = NULL;
    CARDFileInfo* finfo = NULL;
    en_ASYNC_OPERR whyFail = ISG_OPERR_NONE;

    if (didit) *didit = 0;

    rc = iSG_mc_fopen(mcdata, fname, -1, ISG_IOMODE_READ, &whyFail);
    if (!rc) return result;

    rc = iSG_curKosher(&mcdata->cur_stat, &mcdata->cur_finf);

    iSG_mc_fclose(mcdata);

    if (!rc) {
        result = 0;
        if (tossRotten) {
            rc = iSG_mc_fdel(mcdata, fname);
            if (rc && didit) *didit = 1;
        }
    } else {
        result = 1;
    }

    return result;
}

static S32 iSG_get_fsize(st_ISG_MEMCARD_DATA* mcdata, const char* fname)
{
    S32 size = -1;
    S32 rc = 0;

    rc = iSG_get_finfo(mcdata, fname);
    if (rc) size = mcdata->statcache.length;
    if (size < 0) size = -1;

    return size;
}

static S32 iSG_get_fmoddate(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32* sec, S32* min, S32* hr, S32* mon,
                            S32* day, S32* yr)
{
    S32 result = 1;
    S32 rc = 0;
    const CARDStat* stat = NULL;
    OSCalendarTime caltym = { 0 };

    rc = iSG_get_finfo(mcdata, fname);
    if (!rc) result = 0;
    else {
        stat = &mcdata->statcache;
        S64 biggun = stat->time; biggun = OSSecondsToTicks(biggun);
        OSTicksToCalendarTime(biggun, &caltym);

        if (sec) *sec = caltym.sec;
        if (min) *min = caltym.min;
        if (hr) *hr = caltym.hour;
        if (mon) *mon = caltym.mon + 1;
        if (day) *day = caltym.mday;
        if (mon) *mon = caltym.mon + 1;
        if (day) *day = caltym.mday;
        if (yr) *yr = caltym.year;
    }

    return result;
}

static void iSG_timestamp(CARDStat*)
{
}

static S32 iSG_cubeicon_size(S32 cardslot, S32 sectsize)
{
    S32 amt = 0;
    S32 jump = 0;
    S32 fixed = 0;

    if (!((cardslot == 0) || (cardslot == 1))) return -1;

    jump = ISG_ROUNDUP(512, sectsize); fixed = sizeof(st_ISG_CARD_ICONBLOCK);
    amt = ISG_ROUNDUP(fixed, jump);

    return amt;
}

static S32 iSG_chk_icondata()
{
    S32 result = 1;

    return result;
}

static S32 iSG_load_icondata()
{
    g_rawicon = (char*)iFileLoad("/ICON.TPL", NULL, (U32*)&g_iconsize);

    g_rawbanr = (char*)iFileLoad("/BANNER.TPL", NULL, (U32*)&g_banrsize);

    iSG_tpl_unpack((st_ISG_TPL_TEXPALETTE*)g_rawicon);

    iSG_bnr_unpack((st_ISG_TPL_TEXPALETTE*)g_rawbanr);

    return (g_rawicon && g_iconsize && g_rawbanr && g_banrsize) ? 1 : 0;
}

static void iSG_discard_icondata()
{
    OSFree(g_rawicon);
    OSFree(g_rawbanr);
    g_rawicon = NULL; g_iconsize = 0;
    g_rawbanr = NULL; g_banrsize = 0;
}

static char* iSG_bfr_icondata(char* buffer, CARDStat*, char* label, S32 sectsize)
{
    st_ISG_CARD_ICONBLOCK block = { 0 };
    char* destptr = NULL;
    static st_ISG_TPL_TEXPALETTE* ico_pal;
    static st_ISG_TPL_TEXDESCRIP* ico_desc;
    S32 runsum = 0;
    static u32 i = 0;

    sprintf(block.integchk, "The Incredibles");

    strncpy((char*)block.Title, "The Incredibles", sizeof(block.Title));

    block.Title[sizeof(block.Title) - 1] = '\0';
    if (label) strncpy((char*)block.Description, label, sizeof(block.Description));
    block.Description[sizeof(block.Description) - 1] = '\0';

    ico_pal = (st_ISG_TPL_TEXPALETTE*)g_rawbanr;

    ico_desc = iSG_tpl_TEXGet(ico_pal, 0);

    memcpy(block.Banner, ico_desc->textureHeader->data, sizeof(block.Banner));

    ico_pal = (st_ISG_TPL_TEXPALETTE*)g_rawicon;

    U32 texcount = CARD_ICON_MAX;
    if (texcount > ico_pal->numDescriptors)
        texcount = ico_pal->numDescriptors;
    for (i = 0; i < texcount; i++) {
        ico_desc = iSG_tpl_TEXGet(ico_pal, i);

        memcpy(block.Icons[i], ico_desc->textureHeader->data, sizeof(block.Icons[i]));
    }

    memcpy(buffer, &block, sizeof(block));

    S32 sizemult = 0;
    sizemult = ISG_ROUNDUP(512, sectsize);
    destptr = buffer + ISG_ROUNDUP(sizeof(block), sizemult);

    return destptr;
}

static void iSG_upd_icostat(CARDStat*, CARDStat* realstat)
{
    S32 i = 0;

    CARDSetCommentAddress(realstat, 0);

    CARDSetBannerFormat(realstat, CARD_STAT_BANNER_RGB5A3);
    CARDSetIconAddress(realstat, CARD_COMMENT_SIZE);
    CARDSetIconAnim(realstat, CARD_STAT_ANIM_LOOP);

    for (i = 0; i < CARD_ICON_MAX; i++) {
        CARDSetIconFormat(realstat, i, CARD_STAT_ICON_RGB5A3);
        CARDSetIconSpeed(realstat, i, CARD_STAT_SPEED_MIDDLE);
    }
}

static S32 iSG_tpl_unpack(st_ISG_TPL_TEXPALETTE* pal)
{
    S32 i = 0;
    S32 rc = 0;
    st_ISG_TPL_TEXDESCRIP* tmpdesc = NULL;
    u32 palpos = (u32)pal;
    u32 tmppos = 0;

    if (pal->versionNumber != 2142000) return 0;

    tmppos = palpos + (u32)pal->descriptorArray;
    pal->descriptorArray = (st_ISG_TPL_TEXDESCRIP*)tmppos;

    for (i = 0; i < pal->numDescriptors; i++) {
        tmpdesc = &pal->descriptorArray[i];

        if (tmpdesc->textureHeader) {
            tmppos = palpos + (u32)tmpdesc->textureHeader;
            tmpdesc->textureHeader = (st_ISG_TPL_TEXHEADER*)tmppos;

            if (!tmpdesc->textureHeader->unpacked) {
                tmppos = palpos + (u32)tmpdesc->textureHeader->data;
                tmpdesc->textureHeader->data = (char*)tmppos;
                tmpdesc->textureHeader->unpacked = 1;
            }
        }

        if (tmpdesc->CLUTHeader) {
            tmppos = palpos + (u32)tmpdesc->CLUTHeader;
            tmpdesc->CLUTHeader = (st_ISG_TPL_CLUTHEADER*)tmppos;

            if (!tmpdesc->CLUTHeader->unpacked) {
                tmppos = palpos;
                tmpdesc->CLUTHeader->data = (char*)palpos;
                tmpdesc->CLUTHeader->unpacked = 1;
            }
        }
    }

    return 1;
}

static S32 iSG_bnr_unpack(st_ISG_TPL_TEXPALETTE* pal)
{
    return iSG_tpl_unpack(pal);
}

static st_ISG_TPL_TEXDESCRIP* iSG_tpl_TEXGet(st_ISG_TPL_TEXPALETTE* pal, U32 idx)
{
    return &pal->descriptorArray[idx];
}

static S32 iSG_mc_mount(S32 cardslot)
{
    S32 result = 0;
    volatile S32 rc = 0;

    do {
        rc = CARDMount(cardslot, cardwork[cardslot], iSG_cb_unmount);
        isMounted = true;
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;
    else if (rc == CARD_RESULT_BROKEN || rc == CARD_RESULT_ENCODING) {
        result = 1;
    }

    return result;
}

static S32 iSG_mc_unmount(S32 cardslot)
{
    S32 result = 0;
    S32 rc = 0;

    do {
        rc = CARDUnmount(cardslot);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;
    else if (rc == CARD_RESULT_NOCARD) result = 1;

    return result;
}

static void iSG_cb_unmount(s32 chan, s32 retcode)
{
    st_ISGSESSION* isgdata = &g_isgdata_MAIN;

    if (chan == isgdata->idx_actvcard) {
        isgdata->idx_actvcard = -1;
        if (isgdata->chgfunc) {
            isgdata->chgfunc(isgdata->cltdata, ISG_CHG_TARGET);
        }
        memset(&isgdata->physcard[chan], 0, sizeof(st_ISG_MEMCARD_DATA));
    }
}

static S32 iSG_mc_format(st_ISG_MEMCARD_DATA* mcdata, S32 force, S32* canRecover)
{
    S32 result = 0;
    S32 rc = 0;

    if (mcdata->cardIsIll) {
        if (canRecover) *canRecover = 0;
        return result;
    }
    if (canRecover) *canRecover = 1;

    do {
        rc = CARDFormat(mcdata->myphys);
    } while (rc == CARD_RESULT_BUSY);
    if (rc == CARD_RESULT_READY) result = 1;
    else {
        result = 0;

        switch (rc) {
        case CARD_RESULT_IOERROR:
            mcdata->cardIsIll = 1;
            if (canRecover) {
                *canRecover = 0;
            }
            break;

        case CARD_RESULT_NOCARD:
            if (canRecover) {
                *canRecover = -1;
            }
            result = -1;
            break;

        case CARD_RESULT_NOFILE:
        case CARD_RESULT_FATAL_ERROR: rc = 0; break;
        case CARD_RESULT_READY: case CARD_RESULT_BUSY: break;
        }
    }

    return result;
}

static bool cardCreateInProgress;

void CardCreateCallback(s32, s32 result)
{
    callbackResult = result; cardCreateInProgress = false;
}

inline S32 iSG_CARDCreate(s32 chan, const char* fileName, u32 size, CARDFileInfo* fileInfo)
{
    cardCreateInProgress = true;
    S32 rc = CARDCreateAsync(chan, fileName, size, fileInfo, CardCreateCallback);
    if (rc != CARD_RESULT_READY) {
        cardCreateInProgress = false;
        return rc;
    }

    while (cardCreateInProgress)
        iTRC_CHECK();
    return callbackResult;
}

static S32 iSG_mc_fopen(st_ISG_MEMCARD_DATA* mcdata, const char* fname, S32 fsize, en_ISG_IOMODE mode,
                        en_ASYNC_OPERR* whyFail)
{
    S32 result = 0;
    S32 rc = 0;
    CARDFileInfo* finf = &mcdata->cur_finf;
    CARDStat* stat = &mcdata->cur_stat;

    if (whyFail) *whyFail = ISG_OPERR_NONE;

    if (mode == ISG_IOMODE_READ) {
        do {
            rc = CARDOpen(mcdata->myphys, fname, finf);
        } while (rc == CARD_RESULT_BUSY);

        if (rc == CARD_RESULT_READY) {
            result = 1;
        }
    } else {
        rc = iSG_mc_fdel(mcdata, fname);

        if (!mcdata->cardIsIll && rc > 0) {
            do {
                rc = iSG_CARDCreate(mcdata->myphys, fname, fsize, finf);
            } while (rc == CARD_RESULT_BUSY);

            if (rc == CARD_RESULT_READY) {
                mcdata->off_nextwrite = 0; result = 1;
            } else {
                if (whyFail) {
                    switch (rc) {
                    case CARD_RESULT_READY:
                    case CARD_RESULT_BUSY:
                        break;
                    case CARD_RESULT_WRONGDEVICE:
                    case CARD_RESULT_NOCARD:
                        *whyFail = ISG_OPERR_NOCARD;
                        break;
                    case CARD_RESULT_IOERROR:
                    case CARD_RESULT_BROKEN:
                        mcdata->cardIsIll = 1;
                        *whyFail = ISG_OPERR_DAMAGE;
                        break;
                    case CARD_RESULT_EXIST:
                    case CARD_RESULT_NOPERM:
                        break;
                    case CARD_RESULT_NOENT:
                    case CARD_RESULT_INSSPACE:
                        *whyFail = ISG_OPERR_NOROOM;
                        break;
                    default:
                        *whyFail = ISG_OPERR_OTHER;
                        break;
                    }
                }
            }
        }
    }

    if (result) {
        do {
            rc = CARDGetStatus(mcdata->myphys, finf->fileNo, stat);
        } while (rc == CARD_RESULT_BUSY);

        if (rc == CARD_RESULT_READY) {
            result = 1;
        }
    }

    if (zMainIsNTSC())
        ISG_GC_GAMENAME = "GICE";
    else if (zMainIsPAL())
        ISG_GC_GAMENAME = "GICE";

    finf = &mcdata->cur_finf;
    stat = &mcdata->cur_stat;
    stat = NULL;

    return result;
}

static S32 iSG_mc_fclose(st_ISG_MEMCARD_DATA* mcdata)
{
    return iSG_mc_fclose(mcdata, NULL);
}

static S32 iSG_mc_fclose(st_ISG_MEMCARD_DATA* mcdata, CARDStat* statcopy)
{
    S32 result = 0;
    S32 rc = 0;

    do {
        rc = CARDClose(&mcdata->cur_finf);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;

    if (statcopy) memcpy(statcopy, &mcdata->cur_stat, sizeof(CARDStat));

    memset(&mcdata->cur_finf, 0, sizeof(CARDFileInfo));
    memset(&mcdata->cur_stat, 0, sizeof(CARDStat));

    return result;
}

static bool cardDeleteInProgress;

void CardDeleteCallback(s32, s32 result)
{
    callbackResult = result; cardDeleteInProgress = false;
}

inline S32 iSG_CARDDelete(s32 chan, const char* fileName)
{
    cardDeleteInProgress = true;
    S32 rc = CARDDeleteAsync(chan, fileName, CardDeleteCallback);
    if (rc != CARD_RESULT_READY) {
        cardDeleteInProgress = false;
        return rc;
    }

    while (cardDeleteInProgress)
        iTRC_CHECK();
    return callbackResult;
}

static S32 iSG_mc_fdel(st_ISG_MEMCARD_DATA* mcdata, const char* fname)
{
    S32 result = 0;
    S32 rc = 0;
    st_ISGSESSION* isgdata;

    if (mcdata->myphys < 0 || mcdata->myphys > 1) {
        return 0;
    }

    isgdata = &g_isgdata_MAIN;
    if (!isMounted) {
        return 0;
    }

    do {
        rc = iSG_CARDDelete(mcdata->myphys, fname);
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_NOFILE) result = 1;
    else if (rc == CARD_RESULT_READY) result = 1;
    else {
        result = 0;

        en_ASYNC_OPERR whyFail;
        switch (rc) {
        case CARD_RESULT_NOFILE:
            whyFail = ISG_OPERR_NOCARD;
        case CARD_RESULT_NOCARD:
            whyFail = ISG_OPERR_NOCARD;
        case CARD_RESULT_NOPERM:
            whyFail = ISG_OPERR_OTHER;
            break;

        case CARD_RESULT_IOERROR:
            mcdata->cardIsIll = 1;
            whyFail = ISG_OPERR_DAMAGE;
            break;
        case CARD_RESULT_FATAL_ERROR:
            whyFail = ISG_OPERR_OTHER;
            break;
        case CARD_RESULT_READY:
        case CARD_RESULT_BUSY:
            whyFail = ISG_OPERR_NONE;
            break;
        }
    }

    return result;
}

static void iSG_cb_asyndone(s32, s32 retcode)
{
}

static S32 iSG_mc_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize, S32 skipbyte)
{
    S32 result = 1;
    S32 rc = 0;
    en_ASYNC_OPERR whyFail;

    return iSG_mcqa_fread(mcdata, buf, bufsize, skipbyte);
}

static S32 iSG_mcqa_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, S32 bufsize, S32 skipbyte)
{
    S32 result = 0;
    S32 rc = 0;
    S32 updintv = 1000;
    S32 polltillready = 1;

    if (polltillready) {
        CARDGetResultCode(mcdata->myphys);
        do {
            rc = CARDGetResultCode(mcdata->myphys);
        } while (rc == CARD_RESULT_BUSY);
        if (rc != CARD_RESULT_READY) {
            return result;
        }
    }

    CARDGetXferredBytes(mcdata->myphys);

    do {
        rc = CARDReadAsync(&mcdata->cur_finf, buf, bufsize, skipbyte, iSG_cb_asyndone);
    } while (rc == CARD_RESULT_BUSY);

    if (rc != CARD_RESULT_READY) {
        return result;
    }

    rc = CARD_RESULT_BUSY;

    do {
        if (updintv++ > 500) {
            xSndMgrUpdate();

            CARDGetXferredBytes(mcdata->myphys);

            rc = CARDGetResultCode(mcdata->myphys);

            updintv = 0;
        }

        iTRC_CHECK();
    } while (rc == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) result = 1;
    else {
        result = 0;

        en_ASYNC_OPERR whyFail;
        switch (rc) {
        case CARD_RESULT_NOPERM:
        case CARD_RESULT_CANCELED:
        case CARD_RESULT_NOFILE: whyFail = ISG_OPERR_LDREAD;
        case CARD_RESULT_NOCARD: whyFail = ISG_OPERR_NOCARD;
            break;

        case CARD_RESULT_READY:
        case CARD_RESULT_BUSY:
            whyFail = ISG_OPERR_NONE;
            break;
        }
    }

    return result;
}

static S32 iSG_mc_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* data, S32 n)
{
    S32 result = 0;
    S32 rc = 0;
    CARDFileInfo* finf = &mcdata->cur_finf;
    CARDStat* stat = &mcdata->cur_stat;

    return iSG_mcqa_fwrite(mcdata, data, n);
}

static S32 iSG_mcqa_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* data, S32 n)
{
    S32 result = 0;
    S32 rc = 0;
    S32 updintv = 1000;
    S32 asynpoll = 0;
    CARDFileInfo* finf = &mcdata->cur_finf;
    CARDStat* stat = &mcdata->cur_stat;
    S32 polltillready = 1;

    if (polltillready) {
        do {
            rc = CARDGetResultCode(mcdata->myphys);
        } while (rc == CARD_RESULT_BUSY);
        if (rc != CARD_RESULT_READY) {
            return result;
        }
    }

    CARDGetXferredBytes(mcdata->myphys);

    do {
        rc = CARDWriteAsync(&mcdata->cur_finf, data, n, mcdata->off_nextwrite, iSG_cb_asyndone);
    } while (rc == CARD_RESULT_BUSY);

    if (rc != CARD_RESULT_READY) {
        return result;
    }

    asynpoll = CARD_RESULT_BUSY;

    do {
        if (updintv++ > 500) {
            asynpoll = CARDGetResultCode(mcdata->myphys);

            xSndMgrUpdate();

            CARDGetXferredBytes(mcdata->myphys);

            updintv = 0;
        }

        iTRC_CHECK();
    } while (asynpoll == CARD_RESULT_BUSY);

    if (rc == CARD_RESULT_READY) {
        mcdata->off_nextwrite += n;

        do {
            rc = CARDGetStatus(mcdata->myphys, finf->fileNo, stat);
        } while (rc == CARD_RESULT_BUSY);
        if (rc == CARD_RESULT_READY) result = 1;
    }

    return result;
}

static S32 iSG_is_MCOP_realerr(S32 mcop, S32 que_rc); // UNUSED

S32 iSGCheckForWrongDevice()
{
    char* workArea = (char*)RwMalloc(CARD_WORKAREA_SIZE, rwMEMHINTDUR_NADURATION);
    S32 resultCode;

    S32 badCard = -1;

    for (S32 i = 0; i < ISG_MAX_TARGET; i++) {
        resultCode = CARDMount(i, workArea, NULL);
        switch (resultCode) {
        case CARD_RESULT_READY:
        case CARD_RESULT_BROKEN:
        case CARD_RESULT_ENCODING:
            CARDUnmount(i);
            break;

        case CARD_RESULT_WRONGDEVICE:
            badCard = i;

            i = ISG_MAX_TARGET;
            break;
        }
    }
    RwFree(workArea);

    return badCard;
}

S32 iSGCheckForCorruptFiles(st_ISGSESSION* isgdata, char (*fileName)[64])
{
    S32 result = 1, i = 0, rc = 0;
    const char* fname = NULL;
    st_ISG_MEMCARD_DATA* mcdata = NULL;

    if (isgdata->idx_actvcard < 0) {
        return 0;
    }

    mcdata = &isgdata->physcard[isgdata->idx_actvcard];
    S32 fileCount = 0;

    // BUG: clears only the first 12 bytes
    memset(fileName, 0, sizeof(fileName) * ISG_MAX_SAVEGAME);

    for (i = 0; i < ISG_MAX_SAVEGAME; i++) {
        fname = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i);
        rc = iSG_fileKosher(mcdata, fname, 0, NULL);
        if (rc == 0) {
            strcpy(fileName[fileCount], fname);
            fileCount++;
        }
    }

    return fileCount;
}

bool iSGIsGameCorrupt(st_ISGSESSION* sess, S32 gidx)
{
    S32 result = 1;
    S32 rc = 0;
    const char* fname = NULL;
    st_ISG_MEMCARD_DATA* mcdata = NULL;

    if (sess->idx_actvcard < 0) {
        return 0;
    }

    mcdata = &sess->physcard[sess->idx_actvcard];

    fname = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, gidx);

    rc = iSG_fileKosher(mcdata, fname, 0, NULL);
    if (rc == 0) {
        return true;
    }
    return false;
}

bool iSGCheckMemoryCard(st_ISGSESSION*, S32 index)
{
    s32 memSize, sectorSize;

    S32 resultCode = CARDProbeEx(index, &memSize, &sectorSize);
    switch (resultCode) {
    case CARD_RESULT_READY:
        return true;

    default:
        return false;
    }
}

void iSGAutoSave_Startup()
{
}

st_ISGSESSION* iSGAutoSave_Connect(S32 idx_target, void* cltdata, ISGChangeCallback chg)
{
    st_ISGSESSION* isg = iSGSessionBegin(cltdata, chg, 1);
    if (!isg) {
        return isg;
    }

    S32 rc = iSGTgtSetActive(isg, idx_target);
    if (!rc) {
        iSGSessionEnd(isg);
        isg = NULL;
    }

    return isg;
}

void iSGAutoSave_Disconnect(st_ISGSESSION* isg)
{
    iSGSessionEnd(isg);
}

S32 iSGAutoSave_Monitor(st_ISGSESSION* isg, S32 idx_target)
{
    if (!isg) return 0;

    U32 stat = iSGTgtState(isg, idx_target, NULL);
    if (!stat || !(stat & 0x1)) {
        globals.autoSaveFeature = false;
        return 0;
    }

    return 1;
}
