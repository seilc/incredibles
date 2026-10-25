#include "xstransvc.h"

#include "xDebug.h"
#include "xGlobals.h"
#include "xMemMgr.h"
#include "xOutputMgr.h"
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
static U32 g_localDiscLanguageConfig;

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
        xVERBOSE("STM: Starting Scene Transition Module Services\n");

        xVALIDATE_L(87, handlers);
        g_typeHandlers = handlers;
        if (!g_typeHandlers) {
            xWARN("xSTStartup: No Asset Type Handlers?!?\n");
        }

        XST_reset_raw();

        PKRStartup();
        g_pkrf = PKRGetReadFuncs(1);
        xVALIDATE_L(101, g_pkrf);
    }

    return g_straninit;
}

S32 xSTShutdown()
{
    if (--g_straninit == 0) {
        xVERBOSE("STM: Shutting Down Scene Transition Services\n");

        g_typeHandlers = NULL;

        XST_unlock_all();
        PKRShutdown();
    }

    xVALIDATE_L(134, g_straninit>=0);

    return g_straninit;
}

S32 xSTPreLoadScene(U32 sid, void* userdata, S32 flg_hiphop, S32 fileflags, const char* filename)
{
    S32 result = 1;
    st_STRAN_SCENE* sdata = NULL;
    const char* sfile = NULL;
    S32 cltver, cnt_retry;

    if (fileflags & 0x4) {
        sfile = filename;
    }

    cltver = 0;
    cnt_retry = 0;

    xVALIDATE_L(170, g_pkrf);
    xVALIDATE_L(171, sid);

    if ((flg_hiphop & XST_OPTS_MASKHH) == XST_OPTS_HOP) {
        xASSERT_L(177, sid != IDTAG('F','O','N','T'));
        xASSERT_L(178, sid != IDTAG('B','O','O','T'));
        xASSERT_L(179, (sid & 0xFFFFFF00) != IDTAG('M','N','U','4'));

        sdata = XST_lock_next();
        xASSERT_FMT_L(183, sdata, "No pack manager instance for '%s'\n", xUtil_idtag2string(sid, 0));

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
            xVALIDATE_L(215, !sdata->spkg);
            XST_unlock(sdata);
            sdata = NULL;
            result = 0;
#if DEBUG
            char* cp = (char*)&sid;
            xWARN("HOP open failed: %c%c%c%c\n", cp[0], cp[1], cp[2], cp[3]);
#endif
        } else {
            result = cltver;
        }
    } else {
        xASSERT_L(233, (flg_hiphop & XST_OPTS_MASKHH) == XST_OPTS_HIP || (flg_hiphop & XST_OPTS_MASKHH) == XST_OPTS_LOCAL);

        xOutInfo("PACKER", "PreLoading 0x%08X (attempt #%d)\n", sid, ++cnt_retry);

        sdata = XST_lock_next();
        xVALIDATE_L(240, sdata);

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

        if ((flg_hiphop & XST_OPTS_MASKHH) != XST_OPTS_LOCAL) {
            xASSERT_FMT_L(277, cltver != 0, "File not found:  '%s'\n", sfile);
        }

        if (cltver == 0) {
            xVALIDATE_L(283, !sdata->spkg);
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

    xVALIDATE_L(302, g_pkrf);
    xVALIDATE_L(303, sid);

    sdata = XST_find_bySID(sid, flg_hiphop);
    xASSERT_L(308, sdata);
    if (!sdata) result = 0;
    else if (sdata->spkg) {
        xVERBOSE("Queuing Layer Load (default packed order)\n");
        g_pkrf->LoadLayer(sdata->spkg, PKR_LTYPE_ALL);
    }

    return result;
}

// RESTORED: from Incredibles GC
void xSTUnLoadScene(U32 sid, S32 flg_hiphop)
{
    st_STRAN_SCENE* sdata = NULL;
    S32 cnt = 0, i = 0, doTheHOP = 0;

    if (sid == 0) {
        cnt = XST_cnt_locked();
        for (i = 0; i < cnt; i++) {
            sdata = XST_nth_locked(i);
            xASSERT(sdata);
            if (sdata->spkg) {
                g_pkrf->Done(sdata->spkg);
            }
            sdata->spkg = NULL;
        }
        XST_unlock_all();
    } else {
        sdata = XST_find_bySID(sid, flg_hiphop);
        xVALIDATE(sdata);
        if (sdata) {
            if (sdata->spkg) {
                g_pkrf->Done(sdata->spkg);
            }
            sdata->spkg = NULL;
        }
        XST_unlock(sdata);
    }
}
// END RESTORED

void xSTUnLoadSceneMany(U32 sceneID, S32 type)
{
    if (type & 0x1) {
        st_STRAN_SCENE* sdata = XST_find_bySID(sceneID, XST_OPTS_HIP);
        if (sdata) {
            if (sdata->spkg) {
                g_pkrf->Done(sdata->spkg);
            }
            sdata->spkg = NULL;
            XST_unlock(sdata);
        }
    }

    if (type & 0x4) {
        st_STRAN_SCENE* sdata = XST_find_bySID(sceneID, XST_OPTS_LOCAL);
        if (sdata) {
            if (sdata->spkg) {
                g_pkrf->Done(sdata->spkg);
            }
            sdata->spkg = NULL;
            XST_unlock(sdata);
        }
    }

    if (type & 0x2) {
        st_STRAN_SCENE* sdata = XST_find_bySID(sceneID, XST_OPTS_HOP);
        if (sdata) {
            if (sdata->spkg) {
                g_pkrf->Done(sdata->spkg);
            }
            sdata->spkg = NULL;
            XST_unlock(sdata);
        }
    }
}

F32 xSTLoadStep(U32)
{
    F32 pct = 0.0f;
    S32 rc = 0;

    xVERBOSE("STM: Processing Stepper Load Queue\n");

    rc = PKRLoadStep(0);
    if (rc)
        pct = 0.0f;
    else
        pct = 1.00001f;

    if (pct > 1.0f) {
        xVERBOSE("STM: Stepper Loading Complete\n");
    }

    iTRC_CHECK_L(429);
    iFileAsyncService();

    return pct;
}

void xSTDisconnect(U32 sid, S32 flg_hiphop)
{
    st_STRAN_SCENE* sdata = NULL;
    S32 doTheHOP = 0;

    sdata = XST_find_bySID(sid, flg_hiphop);

    if (!doTheHOP) xVALIDATE_L(448, sdata);

    if (sdata) {
        g_pkrf->PkgDisconnect(sdata->spkg);
    }
}

S32 xSTSwitchScene(U32 sid, void* userdata, S32(*progmon)(void*, F32))
{
    st_STRAN_SCENE* sdata = NULL;
    S32 rc = 0, i = 0;

    xASSERT_L(482, sid);

    S32 types[3] = { XST_OPTS_HIP, XST_OPTS_HOP, XST_OPTS_LOCAL };
    for (i = 2; i >= 0; i--) {
        sdata = XST_find_bySID(sid, types[i]);
        if (sdata) {
            if (progmon) progmon(userdata, 0.0f);
            xVERBOSE("STM: Switching Scene to Active\n");

            rc = g_pkrf->SetActive(sdata->spkg, PKR_LTYPE_ALL);
            xVERBOSE("STM: Scene Switch Completed with %s\n", rc ? "Success" : "Failure");

            if (progmon) {
                progmon(userdata, 1.0f);
            }
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
        xASSERT_L(522, sdata);
        if (sdata->spkg) {
            aname = g_pkrf->AssetName(sdata->spkg, aid);
            if (aname) break;
        }
    }

    return (char*)aname;
}

char* xSTAssetName(void* raw_HIP_asset)
{
    const char* aname = NULL;
    S32 scncnt, i;
    st_STRAN_SCENE* sdata;
    U32 aid;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);
        xASSERT_L(541, sdata);
        xVALIDATE_L(542, sdata->spkg);
        aid = PKRAssetIDFromInst(raw_HIP_asset);
        aname = g_pkrf->AssetName(sdata->spkg, aid);
        if (aname) break;
    }

    return (char*)aname;
}

char* xSTAssetNameFromPointer(void* model)
{
    PKRAssetTOCInfo ainfo;
    memset(&ainfo, 0, sizeof(PKRAssetTOCInfo));
    xSTRandMemToAsset(&ainfo, model);
    return xSTAssetName(ainfo.aid);
}

en_pkrfindmyass xSTRandMemToAsset(PKRAssetTOCInfo* ainfo, const void* addr)
{
    S32 scncnt, i;
    st_STRAN_SCENE* sdata;
    en_pkrfindmyass rc_cur;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);
        xASSERT_L(571, sdata);
        xVALIDATE_L(572, sdata->spkg);

        rc_cur = PKRFindMyAsset(sdata->spkg, ainfo, addr);
        if (rc_cur != PKR_FIND_FAILED) {
            return rc_cur;
        }
    }

    return PKR_FIND_FAILED;
}

void* xSTFindAsset(U32 aid, U32* size)
{
    void* memloc;
    S32 scncnt, i;
    st_STRAN_SCENE* sdata;
    S32 asssize;

    xASSERT_FMT_L(598, aid, "xSTFindAsset function is called with 0 asset ID ");
    if (!aid) {
        return NULL;
    }

    memloc = NULL;
    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);
        xASSERT_L(623, sdata);

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

st_PACKER_ATOC_NODE* xSTFindAssetNode(U32 aid, U32* size)
{
    st_PACKER_ATOC_NODE* memloc;
    S32 scncnt, i;
    st_STRAN_SCENE* sdata;
    S32 asssize;

    xASSERT_FMT_L(646, aid, "xSTFindAsset function is called with 0 asset ID ");
    if (!aid) {
        return NULL;
    }

    memloc = NULL;
    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);
        xASSERT_L(671, sdata);

        asssize = 0;
        if (!sdata->spkg) {
            return NULL;
        }

        memloc = PKR_FindAssetNode(sdata->spkg, aid, &asssize);
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
    S32 sum = 0, cnt = 0;
    st_STRAN_SCENE* sdata = NULL;
    S32 scncnt = 0, i = 0;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);
        xASSERT_L(710, sdata);
        xVALIDATE_L(711, sdata->spkg);
        cnt = g_pkrf->AssetCount(sdata->spkg, type);
        sum += cnt;
    }

    return sum;
}

void* xSTFindAssetByType(U32 type, S32 idx, U32* size)
{
    st_STRAN_SCENE* sdata = NULL;
    void* memptr = NULL;
    S32 scncnt = 0, i = 0, sum = 0, cnt = 0;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);
        xASSERT_L(747, sdata);
        xASSERT_L(748, sdata->spkg);
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
        xVALIDATE_L(773, sdata);
        xVALIDATE_L(774, sdata->spkg);

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
    S32 found = 0, sum = 0, cnt = 0, scncnt, i;
    st_STRAN_SCENE* sdata;
    S32 rc;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);
        xVALIDATE_L(811, sdata);
        xVALIDATE_L(812, sdata->spkg);

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
    S32 rc = 0, scncnt = 0, i = 0;

    scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++) {
        sdata = XST_nth_locked(i);
        xVALIDATE_L(905, sdata);
        xVALIDATE_L(906, sdata->spkg);

        rc = g_pkrf->PkgHasAsset(sdata->spkg, aid);
        if (rc) {
            da_hipname = sdata->fnam;
            if (sceneID) *sceneID = sdata->scnid;
            break;
        }
    }

    return (char*)da_hipname;
}

#if DEBUG
void xSTReportLoadTimes(U32 sceneID, S32 toScreen)
{
    st_STRAN_SCENE* sdata = NULL;
    S32 scncnt = 0, i = 0, rptopts = 0;

    if (g_pkrf->dbgrpt_loadtimes) {
        if (toScreen) {
            rptopts = 0xD;
        } else {
            rptopts = 0x6;
        }

        if (!sceneID && !toScreen) rptopts |= 0x10;

        scncnt = XST_cnt_locked();
        for (i = 0; i < scncnt; i++) {
            sdata = XST_nth_locked(i);
            xVALIDATE_L(986, sdata);
            xVALIDATE_L(987, sdata->spkg);

            if (!sceneID || sceneID == sdata->scnid) {
                g_pkrf->dbgrpt_loadtimes(sdata->spkg, rptopts);
                if (sceneID) break;
            }
        }
    }
}
#endif

static S32 XST_PreLoadScene(st_STRAN_SCENE* sdata, const char* sfile, S32 fileflags)
{
    S32 cltver = 0;

    xVALIDATE_L(1017, g_pkrf);
    xASSERT_L(1018, sdata);
    xASSERT_L(1019, sfile);

    sdata->spkg = g_pkrf->Init(sdata->userdata, sfile, 0x2E, &cltver, g_typeHandlers, fileflags);
    xVALIDATE_L(1026, sdata->spkg);

    if (!sdata->spkg) {
        return 0;
    } else {
        return cltver;
    }
}

static char* XST_translate_sid(U32 sid, char* exten, bool local)
{
    static char fname[64];

    xASSERT_L(1060, exten);
    fname[0] = '\0';

    if (IsPS2Demo() && xglobals->PS2demo->subdir[0] != '\0') {
        strcat(fname, xglobals->PS2demo->subdir);
        strcat(fname, "\\");
    }

    strcat(fname, xUtil_idtag2string(sid, 0));
    if (local) {
        strcat(fname, g_localCode);
    }
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
    if (local) {
        strcat(fname, g_localCode);
    }
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

    xASSERT_MSG_L(1140, sdata, "No pack manager available");

    if (sdata) {
        sdata->lockid = uselock;
    }

    return sdata;
}

static void XST_unlock(st_STRAN_SCENE* sdata)
{
    xVALIDATE_L(1152, sdata);
    if (sdata) {
        xVALIDATE_L(1156, !sdata->spkg);

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

    xVALIDATE_L(1172, !g_xstdata.loadlock);
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
        if (g_xstdata.loadlock & (1 << i)) {
            cnt++;
        }
    }

    return cnt;
}

static st_STRAN_SCENE* XST_nth_locked(S32 nth)
{
    st_STRAN_SCENE* sdata = NULL;
    S32 cnt = 0, i = 0;

    xVALIDATE_L(1214, nth>=0);
    xVALIDATE_L(1215, nth<STRAN_MAX_SCENES);

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

    xVALIDATE_L(1239, sid);

    for (i = 0; i < STRAN_MAX_SCENES; i++) {
        if (g_xstdata.loadlock & (1 << i)) {
            tmp_sdata = &g_xstdata.hipscn[i];
            xVALIDATE_L(1245, tmp_sdata->scnid);
            if (tmp_sdata->scnid == sid) {
                if ((tmp_sdata->flgHipHop & XST_OPTS_MASKHH) == (flagHipHop & XST_OPTS_MASKHH)) {
                    da_sdata = tmp_sdata;
                    break;
                }
            }
        }
    }

    xVALIDATE_L(1257, da_sdata);

    return da_sdata;
}

void xSTSetLocalizationCode(const char* code)
{
    g_localCode[0] = '_';
    strcpy(g_localCode + 1, code);

    g_localEnum = xRegionString(code);
    xASSERT_L(1267, g_localEnum!=eRegionUnknown);
}

char* xSTGetLocalizationCode()
{
    return g_localCode;
}

xRegion xSTGetLocalizationEnum()
{
    return g_localEnum;
}

void xSTAddToDiscLanguageConfig(xRegion region)
{
    if (region > eRegionUnknown && region < eRegionCount) {
        g_localDiscLanguageConfig |= 1 << region;
    }
}

U32 xSTGetDiscLanguageConfig()
{
    return g_localDiscLanguageConfig;
}

S32 xSTGetAssetTypes(U32* types, S32 max_types)
{
    xASSERT_L(1294, max_types > 0);

    U32* type_buffer = (U32*)xMEMPUSHTEMP_L(0x1000, 0, 1297);
    U32* type = type_buffer, *end_type = type + 0x1000 / sizeof(U32);

    S32 scenes = XST_cnt_locked();
    for (S32 i = 0; i < scenes; i++) {
        st_STRAN_SCENE* sdata = XST_nth_locked(i);
        st_PACKER_READ_DATA* pr = sdata->spkg;
        const PKRAssetType* atype = pr->types;
        for (; atype->typetag != 0 && type != end_type; type++, atype++)
            *type = atype->typetag;
    }

    S32 itotal = type - type_buffer;
    quicksort_uint(type_buffer, itotal, sizeof(U32), 0);

    U32 last_type_value = 0;
    U32* otype = types, *end_otype = otype + max_types;
    const U32* itype = type_buffer, *end_itype = itype + itotal;
    for (; itype != end_itype; itype++) {
        if (*itype != last_type_value) {
            *otype = last_type_value = *itype;
            otype++;
            if (otype == end_otype) break;
        }
    }

    xMEMPOPTEMP(type_buffer);
    return otype - types;
}
