#include "xstransvc.h"

#include "xGlobals.h"
#include "xutil.h"
#include "iFile.h"
#include "iTRC.h"

#include <string.h>

#define STRAN_MAX_SCENES 16

#define XST_OPTS_HIP (1<<0)
#define XST_OPTS_HOP (2<<0)
#define XST_OPTS_LOCAL (3<<0)
#define XST_OPTS_MASKHH (3<<0)

struct st_STRAN_SCENE
{
    U32 scnid;
    S32 lockid;
    st_PACKER_READ_DATA* spkg;
    S32 flgHipHop;
    void* userdata;
    char fnam[256];
};

struct st_STRAN_DATA
{
    st_STRAN_SCENE hipscn[STRAN_MAX_SCENES];
    U32 loadlock;
};

static S32 g_straninit;
static st_STRAN_DATA g_xstdata = {};
static PKRReadFuncs* g_pkrf;
static PKRAssetType* g_typeHandlers;
static char g_localCode[4] = "_US";
static xRegion g_localEnum = eRegionUnknown;

static S32 XST_PreLoadScene(st_STRAN_SCENE* sdata, const char* sfile, S32 fileflags);
static char* XST_translate_sid(U32 sid, char* exten, bool local);
static char* XST_translate_sid_path(U32 sid, char* exten, bool local);
static void XST_reset_raw();
static st_STRAN_SCENE* XST_lock_next();
static void XST_unlock(st_STRAN_SCENE* sdata);
static void XST_unlock_all();
static st_STRAN_SCENE* XST_get_rawinst(S32 idx);
static S32 XST_cnt_locked();
static st_STRAN_SCENE* XST_nth_locked(S32 nth);
static st_STRAN_SCENE* XST_find_bySID(U32 sid, S32 flagHipHop);

bool xSTIsLoaded()
{
    return g_straninit != 0;
}

S32 xSTStartup(PKRAssetType* handlers)
{
    if (g_straninit++ == 0) {
        g_typeHandlers = handlers;

        XST_reset_raw();

        PKRStartup();
        g_pkrf = PKRGetReadFuncs(1);
    }

    return g_straninit;
}

S32 xSTShutdown()
{
    if (--g_straninit == 0) {
        g_typeHandlers = NULL;

        XST_unlock_all();
        PKRShutdown();
    }

    return g_straninit;
}

S32 xSTPreLoadScene(U32 sid, void* userdata, S32 flg_hiphop, S32 fileflags, const char* filename)
{
    S32 result = 1;
    st_STRAN_SCENE* sdata = NULL;
    const char* sfile = NULL;
    S32 cltver;

    if (fileflags & 0x4) {
        sfile = filename;
    }

    cltver = 0;

    if ((flg_hiphop & XST_OPTS_MASKHH) == XST_OPTS_HOP) {
        sdata = XST_lock_next();

        sdata->scnid = sid;
        sdata->userdata = userdata;
        sdata->flgHipHop = flg_hiphop;

        if (!sfile) {
            sfile = XST_translate_sid_path(sid, ".HOP", false);
        }

        if (sfile) {
            strcpy(sdata->fnam, sfile);

            if (iFileExists(sfile, fileflags)) {
                cltver = XST_PreLoadScene(sdata, sfile, fileflags);
            }
        }

        if (cltver == 0) {
            sfile = XST_translate_sid(sid, ".HOP", false);
            if (sfile) {
                strcpy(sdata->fnam, sfile);
                if (iFileExists(sfile, fileflags)) {
                    cltver = XST_PreLoadScene(sdata, sfile, fileflags);
                }
            }
        }

        if (cltver == 0) {
            XST_unlock(sdata);
            sdata = NULL;
            result = 0;
        } else {
            result = cltver;
        }
    } else {
        sdata = XST_lock_next();

        sdata->scnid = sid;
        sdata->userdata = userdata;
        sdata->flgHipHop = flg_hiphop;

        if (sid != IDTAG('B','O','O','T') && sid != IDTAG('P','L','A','T') && sid != IDTAG('D','E','M','O') && sid != IDTAG('F','O','N','T')) {
            if (!sfile) {
                sfile = XST_translate_sid_path(sid, ".HIP", (flg_hiphop & XST_OPTS_MASKHH) == XST_OPTS_LOCAL);
            }

            if (sfile) {
                strcpy(sdata->fnam, sfile);

                if (iFileExists(sfile, fileflags)) {
                    cltver = XST_PreLoadScene(sdata, sfile, fileflags);
                }
            }
        }

        if (cltver == 0) {
            sfile = XST_translate_sid(sid, ".HIP", (flg_hiphop & flg_hiphop & XST_OPTS_MASKHH) == XST_OPTS_LOCAL);
            if (sfile) {
                strcpy(sdata->fnam, sfile);
                if (iFileExists(sfile, fileflags)) {
                    cltver = XST_PreLoadScene(sdata, sfile, fileflags);
                }
            }
        }

        if (cltver == 0) {
            XST_unlock(sdata);

            result = 0;
        } else {
            result = cltver;
        }
    }

    return result;
}

S32 xSTQueueSceneAssets(U32 sid, S32 flg_hiphop)
{
    S32 result = 1;
    st_STRAN_SCENE* sdata = NULL;
    S32 doTheHOP = 0;

    sdata = XST_find_bySID(sid, flg_hiphop);
    if (!sdata) result = 0;
    else if (sdata->spkg) {
        g_pkrf->LoadLayer(sdata->spkg, PKR_LTYPE_ALL);
    }

    return result;
}

void xSTUnLoadScene(U32 sid, S32 flg_hiphop)
{
    st_STRAN_SCENE* sdata = NULL;
    S32 cnt = 0, i = 0, doTheHOP = 0;

    if (sid == 0) {
        cnt = XST_cnt_locked();
        for (i = 0; i < cnt; i++) {
            sdata = XST_nth_locked(i);
            if (sdata->spkg) {
                g_pkrf->Done(sdata->spkg);
            }
            sdata->spkg = NULL;
        }
        XST_unlock_all();
    } else {
        sdata = XST_find_bySID(sid, flg_hiphop);
        if (sdata) {
            if (sdata->spkg) {
                g_pkrf->Done(sdata->spkg);
            }
            sdata->spkg = NULL;
        }
        XST_unlock(sdata);
    }
}

void xSTUnLoadSceneMany(U32 sceneID, S32 type)
{
    if (type & 0x1) {
        st_STRAN_SCENE* sdata = XST_find_bySID(sceneID, XST_OPTS_HIP);
        if (sdata) {
            if (sdata->spkg)
                g_pkrf->Done(sdata->spkg);
            sdata->spkg = NULL;
            XST_unlock(sdata);
        }
    }

    if (type & 0x4) {
        st_STRAN_SCENE* sdata = XST_find_bySID(sceneID, XST_OPTS_LOCAL);
        if (sdata) {
            if (sdata->spkg)
                g_pkrf->Done(sdata->spkg);
            sdata->spkg = NULL;
            XST_unlock(sdata);
        }
    }

    if (type & 0x2) {
        st_STRAN_SCENE* sdata = XST_find_bySID(sceneID, XST_OPTS_HOP);
        if (sdata) {
            if (sdata->spkg)
                g_pkrf->Done(sdata->spkg);
            sdata->spkg = NULL;
            XST_unlock(sdata);
        }
    }
}

F32 xSTLoadStep(U32)
{
    F32 pct = 0.0f;
    S32 rc = 0;

    rc = PKRLoadStep(0);
    if (rc)
        pct = 0.0f;
    else
        pct = 1.00001f;

    iTRC_CHECK();
    iFileAsyncService();

    return pct;
}

void xSTDisconnect(U32 sid, S32 flg_hiphop)
{
    st_STRAN_SCENE* sdata = NULL;

    sdata = XST_find_bySID(sid, flg_hiphop);

    if (sdata) {
        g_pkrf->PkgDisconnect(sdata->spkg);
    }
}

S32 xSTSwitchScene(U32 sid, void* userdata, S32(*progmon)(void*, F32))
{
    st_STRAN_SCENE* sdata = NULL;
    S32 rc = 0;
    S32 i = 0;

    S32 types[3] = { XST_OPTS_HIP, XST_OPTS_HOP, XST_OPTS_LOCAL };
    for (i = 2; i >= 0; i--) {
        sdata = XST_find_bySID(sid, types[i]);
        if (sdata) {
            if (progmon) progmon(userdata, 0.0f);

            rc = g_pkrf->SetActive(sdata->spkg, PKR_LTYPE_ALL);

            if (progmon)
                progmon(userdata, 1.0f);
        }
    }

    return rc;
}

char* xSTAssetName(U32 aid)
{
    const char* aname = NULL;
    S32 scncnt = 0, i = 0;
    st_STRAN_SCENE* sdata = NULL;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);
        if (sdata->spkg) {
            aname = g_pkrf->AssetName(sdata->spkg, aid);
            if (aname) break;
        }
    }

    return (char*)aname;
}

en_pkrfindmyass xSTRandMemToAsset(PKRAssetTOCInfo* ainfo, const void* addr)
{
    S32 scncnt, i;
    st_STRAN_SCENE* sdata;
    en_pkrfindmyass rc_cur;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);

        rc_cur = PKRFindMyAsset(sdata->spkg, ainfo, addr);
        if (rc_cur != PKR_FIND_FAILED) {
            return rc_cur;
        }
    }

    return PKR_FIND_FAILED;
}

void* xSTFindAsset(U32 aid, U32* size)
{
    if (!aid) {
        return NULL;
    }

    void* memloc = NULL;
    S32 scncnt, i;
    st_STRAN_SCENE* sdata;
    S32 asssize;
    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);

        asssize = 0;
        memloc = g_pkrf->LoadAsset(sdata->spkg, aid, NULL, NULL, &asssize);
        if (memloc) {
            if (size) {
                *size = asssize;
            }
            break;
        }
    }

    return memloc;
}

S32 xSTAssetCountByType(U32 type)
{
    S32 sum = 0;
    S32 cnt = 0;
    st_STRAN_SCENE* sdata = NULL;
    S32 scncnt = 0;
    S32 i = 0;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);
        cnt = g_pkrf->AssetCount(sdata->spkg, type);
        sum += cnt;
    }

    return sum;
}

void* xSTFindAssetByType(U32 type, S32 idx, U32* size)
{
    st_STRAN_SCENE* sdata = NULL;
    void* memptr = NULL;
    S32 scncnt = 0;
    S32 i = 0;
    S32 sum = 0;
    S32 cnt = 0;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);
        cnt = g_pkrf->AssetCount(sdata->spkg, type);
        if (idx >= sum && idx < sum + cnt) {
            memptr = g_pkrf->AssetByType(sdata->spkg, type, idx - sum, size);
            break;
        }
        sum += cnt;
    }

    return memptr;
}

S32 xSTGetAssetInfo(U32 aid, PKRAssetTOCInfo* tocainfo)
{
    S32 found = 0, scncnt, i;
    st_STRAN_SCENE* sdata;
    S32 rc;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);

        rc = g_pkrf->GetAssetInfo(sdata->spkg, aid, tocainfo);
        if (rc) {
            found = 1;
            break;
        }
    }

    return found;
}

S32 xSTGetAssetInfoByType(U32 type, S32 idx, PKRAssetTOCInfo* tocainfo)
{
    S32 found = 0;
    S32 sum = 0;
    S32 cnt = 0;
    S32 scncnt, i;
    st_STRAN_SCENE* sdata;
    S32 rc;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);

        cnt = g_pkrf->AssetCount(sdata->spkg, type);
        if (idx >= sum && idx < sum + cnt) {
            rc = g_pkrf->GetAssetInfoByType(sdata->spkg, type, idx - sum, tocainfo);
            if (rc) {
                found = 1;
                break;
            }
        }
        sum += cnt;
    }

    return found;
}

char* xST_xAssetID_HIPFullPath(U32 aid)
{
    return xST_xAssetID_HIPFullPath(aid, NULL);
}

char* xST_xAssetID_HIPFullPath(U32 aid, U32* sceneID)
{
    const char* da_hipname = NULL;
    st_STRAN_SCENE* sdata = NULL;
    S32 rc = 0;
    S32 scncnt = 0;
    S32 i = 0;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);

        rc = g_pkrf->PkgHasAsset(sdata->spkg, aid);
        if (rc) {
            da_hipname = sdata->fnam;
            if (sceneID) *sceneID = sdata->scnid;
            break;
        }
    }

    return (char*)da_hipname;
}

static S32 XST_PreLoadScene(st_STRAN_SCENE* sdata, const char* sfile, S32 fileflags)
{
    S32 cltver = 0;

    sdata->spkg = g_pkrf->Init(sdata->userdata, sfile, 0x2E, &cltver, g_typeHandlers, fileflags);

    if (!sdata->spkg) {
        return 0;
    } else {
        return cltver;
    }
}

static char* XST_translate_sid(U32 sid, char* exten, bool local)
{
    static char fname[64];

    fname[0] = '\0';

    if (IsPS2Demo() && xglobals->PS2demo->subdir[0] != '\0') {
        strcat(fname, xglobals->PS2demo->subdir);
        strcat(fname, "\\");
    }

    strcat(fname, xUtil_idtag2string(sid, 0));
    if (local)
        strcat(fname, g_localCode);
    strcat(fname, exten);

    return fname;
}

static char* XST_translate_sid_path(U32 sid, char* exten, bool local)
{
    char path_delimiter[] = "/";
    static char fname[64];
    char subdir[4];

    fname[0] = '\0';

    if (IsPS2Demo() && xglobals->PS2demo->subdir[0] != '\0') {
        strcat(fname, xglobals->PS2demo->subdir);
        strcat(fname, "\\");
    }

    subdir[0] = xUtil_idtag2string(sid, 0)[0];
    subdir[1] = xUtil_idtag2string(sid, 0)[1];
    subdir[2] = '\0';
    strcat(fname, subdir);
    strcat(fname, path_delimiter);
    strcat(fname, xUtil_idtag2string(sid, 0));
    if (local)
        strcat(fname, g_localCode);
    strcat(fname, exten);

    return fname;
}

static void XST_reset_raw()
{
    memset(&g_xstdata, 0, sizeof(st_STRAN_DATA));
}

static st_STRAN_SCENE* XST_lock_next()
{
    st_STRAN_SCENE* sdata = NULL;
    S32 i = 0, uselock = -1;

    for (i = 0; i < STRAN_MAX_SCENES; i++) {
        if (!(g_xstdata.loadlock & (1 << i))) {
            g_xstdata.loadlock |= 1 << i;
            sdata = &g_xstdata.hipscn[i];
            memset(sdata, 0, sizeof(st_STRAN_SCENE));
            uselock = i;
            break;
        }
    }

    if (sdata) {
        sdata->lockid = uselock;
    }

    return sdata;
}

static void XST_unlock(st_STRAN_SCENE* sdata)
{
    if (sdata) {
        if (g_xstdata.loadlock & (1 << sdata->lockid)) {
            g_xstdata.loadlock &= ~(U32)(1 << sdata->lockid);
            memset(sdata, 0, sizeof(st_STRAN_SCENE));
        }
    }
}

static void XST_unlock_all()
{
    st_STRAN_SCENE* sdata = NULL;
    S32 i = 0;

    if (g_xstdata.loadlock) {
        for (i = 0; i < STRAN_MAX_SCENES; i++) {
            if (g_xstdata.loadlock & (1 << i)) {
                sdata = XST_get_rawinst(i);
                XST_unlock(sdata);
            }
        }
    }
}

static st_STRAN_SCENE* XST_get_rawinst(S32 idx)
{
    return &g_xstdata.hipscn[idx];
}

static S32 XST_cnt_locked()
{
    S32 cnt = 0, i = 0;

    for (i = 0; i < STRAN_MAX_SCENES; i++) {
        if (g_xstdata.loadlock & (1 << i))
            cnt++;
    }

    return cnt;
}

static st_STRAN_SCENE* XST_nth_locked(S32 nth)
{
    st_STRAN_SCENE* sdata = NULL;
    S32 cnt = 0, i = 0;

    for (i = 0; i < STRAN_MAX_SCENES; i++) {
        if (g_xstdata.loadlock & (1 << i)) {
            if (cnt == nth) {
                sdata = &g_xstdata.hipscn[i];
                break;
            }
            cnt++;
        }
    }

    return sdata;
}

static st_STRAN_SCENE* XST_find_bySID(U32 sid, S32 flagHipHop)
{
    st_STRAN_SCENE* da_sdata = NULL;
    S32 i = 0;
    st_STRAN_SCENE* tmp_sdata = NULL;

    for (i = 0; i < STRAN_MAX_SCENES; i++) {
        if (g_xstdata.loadlock & (1 << i)) {
            tmp_sdata = &g_xstdata.hipscn[i];
            if (tmp_sdata->scnid == sid)
                if ((tmp_sdata->flgHipHop & XST_OPTS_MASKHH) == (flagHipHop & XST_OPTS_MASKHH)) {
                    da_sdata = tmp_sdata;
                    break;
                }
        }
    }

    return da_sdata;
}

void xSTSetLocalizationCode(const char* code)
{
    g_localCode[0] = '_';
    strcpy(g_localCode + 1, code);

    g_localEnum = xRegionString(code);
}

char* xSTGetLocalizationCode()
{
    return g_localCode;
}

xRegion xSTGetLocalizationEnum()
{
    return g_localEnum;
}
