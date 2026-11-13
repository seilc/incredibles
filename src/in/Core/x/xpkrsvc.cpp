#include "xCommon.h"
#include "xpkrsvc.h"
#include "xhipio.h"
#include "xMemMgr.h"
#include "xOutputMgr.h"
#include "xutil.h"
#include "zMain.h"

#include <rwcore.h>
#include <string.h>

#define SIMPLE_SHADOW_TABLE_ID 0x98A3F56C // xStrHash("simple shadow table")

#define PKR_MAX_NAMLEN 128
#define PKR_MAX_TYPES 128
#define PKR_MAX_INSTANCES 20

#define PKR_INFO_SOURCE_FILE 0x1
#define PKR_INFO_SOURCE_VIRTUAL 0x2
#define PKR_INFO_READ_TRANSFORM 0x4
#define PKR_INFO_WRITE_TRANSFORM 0x8

#define PKR_LDASS_ACTIVE 0x10000
#define PKR_LDASS_READY 0x80000
#define PKR_LDASS_ISDUP 0x100000
#define PKR_LDASS_ISNIL 0x200000

#define PKR_LDLAY_INPROG 0x1000000
#define PKR_LDLAY_ISDONE 0x2000000

enum en_PKR_LAYER_LOAD_DEST
{
    PKR_LDDEST_SKIP,
    PKR_LDDEST_KEEPSTATIC,
    PKR_LDDEST_KEEPMALLOC,
    PKR_LDDEST_RWHANDOFF,
    PKR_LDDEST_NOMORE,
    PKR_LDDEST_FORCE = FORCEENUMSIZEINT
};

struct st_PACKER_LTOC_NODE
{
    en_LAYER_TYPE laytyp;
    st_XORDEREDARRAY assref;
    S32 flg_ldstat;
    S32 danglecnt;
    U32 chksum;
    S32 laysize;
    char* laymem;
    char* laytru;
};

static st_PACKER_READ_DATA* PKR_ReadInit(void* userdata, const char* pkgfile, U32 opts, S32* cltver, PKRAssetType* typelist, S32 fileflags);
static void PKR_ReadDone(st_PACKER_READ_DATA* pr);
static S32 PKR_SetActive(st_PACKER_READ_DATA* pr, en_LAYER_TYPE layer);
static S32 PKR_parse_TOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 PKR_LoadStep_Async();
static char* PKR_LayerMemReserve(st_PACKER_READ_DATA* pr, st_PACKER_LTOC_NODE* layer);
static void PKR_LayerMemRelease(st_PACKER_READ_DATA*, st_PACKER_LTOC_NODE* layer);
static en_PKR_LAYER_LOAD_DEST PKR_layerLoadDest(en_LAYER_TYPE laytyp);
static S32 PKR_layerTypeNeedsXForm(en_LAYER_TYPE laytyp);
static S32 PKR_findNextLayerToLoad(st_PACKER_READ_DATA** work_on_pkg, st_PACKER_LTOC_NODE** next_layer);
static void PKR_updateLayerAssets(st_PACKER_LTOC_NODE* laynode);
static void PKR_xformLayerAssets(st_PACKER_LTOC_NODE* laynode);
static void PKR_xform_asset(st_PACKER_ATOC_NODE* assnode, S32 dumpable_layer);
static void* PKR_FindAsset(st_PACKER_READ_DATA* pr, U32 aid, S32* assetsize);
static S32 PKR_LoadLayer(st_PACKER_READ_DATA*, en_LAYER_TYPE);
static void* PKR_LoadAsset(st_PACKER_READ_DATA* pr, U32 aid, const char*, void*, S32* assetsize);
static U32 PKR_GetAssetSize(st_PACKER_READ_DATA* pr, U32 aid);
static S32 PKR_AssetCount(st_PACKER_READ_DATA* pr, U32 type);
static void* PKR_AssetByType(st_PACKER_READ_DATA* pr, U32 type, S32 idx, U32* size);
static S32 PKR_IsAssetReady(st_PACKER_READ_DATA* pr, U32 aid);
static U32 PKR_getPackTimestamp(st_PACKER_READ_DATA* pr);
static void PKR_Disconnect(st_PACKER_READ_DATA* pr);
static const char* PKR_AssetName(st_PACKER_READ_DATA* pr, U32 aid);
static U32 PKR_GetBaseSector(st_PACKER_READ_DATA* pr);
static S32 PKR_GetAssetInfo(st_PACKER_READ_DATA* pr, U32 aid, PKRAssetTOCInfo* tocinfo);
static S32 PKR_GetAssetInfoByType(st_PACKER_READ_DATA* pr, U32 type, S32 idx, PKRAssetTOCInfo* tocinfo);
static S32 PKR_PkgHasAsset(st_PACKER_READ_DATA* pr, U32 aid);
static S32 PKR_FRIEND_assetIsGameDup(U32 aid, const st_PACKER_READ_DATA* skippr, S32 oursize, U32 ourtype, U32 chksum, char* our_fnam);
static S32 PKR_makepool_anode(st_PACKER_READ_DATA* pr, S32 cnt);
static void PKR_kiilpool_anode(st_PACKER_READ_DATA* pr);
static st_PACKER_ATOC_NODE* PKR_newassnode(st_PACKER_READ_DATA* pr, U32 aid);
static st_PACKER_LTOC_NODE* PKR_newlaynode(en_LAYER_TYPE laytyp, S32 refcnt);
static void PKR_oldlaynode(st_PACKER_LTOC_NODE* laynode);
static S32 OrdComp_R_Asset(void* vkey, void* vitem);
static S32 OrdTest_R_AssetID(const void* vkey, void* vitem);
static S32 LOD_r_HIPA(st_HIPLOADDATA*, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PACK(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PVER(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PFLG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*);
static S32 LOD_r_PCNT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PCRT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PMOD(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_PLAT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_DICT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_ATOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_AINF(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*);
static S32 LOD_r_AHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_ADBG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr, st_PACKER_ATOC_NODE* assnode);
static S32 LOD_r_LTOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_LINF(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*);
static S32 LOD_r_LHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_LDBG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr, st_PACKER_LTOC_NODE* laynode);
static S32 LOD_r_STRM(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
static S32 LOD_r_DHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*);
static S32 LOD_r_DPAK(st_HIPLOADDATA*, st_PACKER_READ_DATA*);
static void PKR_bld_typecnt(st_PACKER_READ_DATA* pr);
static S32 PKR_typeHdlr_idx(st_PACKER_READ_DATA* pr, U32 type);
static void PKR_spew_verhist();
static PKRAssetType* PKR_type2typeref(U32 type, PKRAssetType* typelist);
static void PKR_alloc_chkidx();
static void* PKR_getmem(U32 id, S32 amount, U32 aid, S32 align);
static void* PKR_getmem(U32 id, S32 amount, U32 aid, S32 align, S32 isTemp, char** memtru);
static void PKR_relmem(U32 id, S32 blksize, void* memptr, U32 aid, S32 isTemp);
static void PKR_special_loadbuf_killed(RwResEntry*); // UNUSED
static void* PKR_specialGet_loadbuf(st_PACKER_READ_DATA* pr, S32 amount, S32 align); // UNUSED
static S32 PKR_push_memmark();
static S32 PKR_pop_memmark();

static PKRReadFuncs g_pkr_read_funcmap_original = {
    1,
    PKR_ReadInit,
    PKR_ReadDone,
    PKR_LoadLayer,
    PKR_GetAssetSize,
    PKR_LoadAsset,
    PKR_AssetByType,
    PKR_AssetCount,
    PKR_IsAssetReady,
    PKR_SetActive,
    PKR_AssetName,
    PKR_GetBaseSector,
    PKR_GetAssetInfo,
    PKR_GetAssetInfoByType,
    PKR_PkgHasAsset,
    PKR_getPackTimestamp,
    PKR_Disconnect,
};
static PKRReadFuncs g_pkr_read_funcmap = g_pkr_read_funcmap_original;

static st_HIPLOADFUNCS* g_hiprf;
static st_PACKER_READ_DATA g_readdatainst[PKR_MAX_INSTANCES] = {};
static U32 g_loadlock;
static S32 pkr_sector_size;
static S32 g_packinit;
static S32 g_memalloc_pair;
static S32 g_memalloc_runtot;
static S32 g_memalloc_runfree;
static char* g_strz_layername[] = {
    "DEFAULT",
    "TEXTURE",
    "TEXTURE_STRM",
    "BSP",
    "MODEL",
    "ANIMATION",
    "VRAM",
    "SRAM",
    "SNDTOC",
    "CUTSCENE",
    "CUTSCENETOC",
    "JSPINFO"
};

PKRReadFuncs* PKRGetReadFuncs(S32 apiver)
{
    switch (apiver) {
    case 1:
        return &g_pkr_read_funcmap;
    }
    return NULL;
}

S32 PKRStartup()
{
    if (!g_packinit++) {
        g_pkr_read_funcmap = g_pkr_read_funcmap_original;

        g_hiprf = get_HIPLFuncs();

        pkr_sector_size = 32;
    }
    return g_packinit;
}

S32 PKRShutdown()
{
    --g_packinit;
    return g_packinit;
}

S32 PKRLoadStep(S32 block)
{
    S32 more_todo = TRUE;

    more_todo = PKR_LoadStep_Async();

    return more_todo;
}

static st_PACKER_READ_DATA* PKR_ReadInit(void* userdata, const char* pkgfile, U32 opts, S32* cltver, PKRAssetType* typelist, S32 fileflags)
{
    st_PACKER_READ_DATA* pr = NULL;
    S32 i = 0;
    S32 uselock = -1;
    char* tocbuf_RAW = NULL;
    char* tocbuf_aligned = NULL;

    PKR_alloc_chkidx();

    tocbuf_aligned = (char*)PKR_getmem('PTOC', 0x8000, 'PTOC', 64, TRUE, &tocbuf_RAW);

    for (i = 0; i < PKR_MAX_INSTANCES; i++) {
        if (!(g_loadlock & (1 << i))) {
            g_loadlock |= (1 << i);
            pr = &g_readdatainst[i];
            uselock = i;
            break;
        }
    }

    if (pr) {
        memset(pr, 0, sizeof(st_PACKER_READ_DATA));

        pr->lockid = uselock;
        pr->userdata = userdata;
        pr->opts = opts;
        pr->types = typelist;
        pr->cltver = -1;

        strncpy(pr->packfile, pkgfile, PKR_MAX_NAMLEN);

        if (!tocbuf_aligned) {
            pr->pkg = g_hiprf->create(pkgfile, NULL, 0, fileflags);
        } else {
            pr->pkg = g_hiprf->create(pkgfile, tocbuf_aligned, 0x8000, fileflags);
        }

        if (pr->pkg) {
            pr->base_sector = g_hiprf->basesector(pr->pkg);

            PKR_parse_TOC(pr->pkg, pr);

            *cltver = pr->cltver;

            g_hiprf->setBypass(pr->pkg, TRUE, TRUE);
        } else {
            PKR_ReadDone(pr);

            pr = NULL;
            *cltver = -1;
        }
    } else {
        pr = NULL;
        *cltver = -1;
    }

    PKR_relmem('PTOC', 0x8000, tocbuf_RAW, 'PTOC', TRUE);

    tocbuf_RAW = NULL;

    return pr;
}

static void PKR_ReadDone(st_PACKER_READ_DATA* pr)
{
    S32 i = 0;
    S32 j = 0;
    S32 lockid = -1;
    st_PACKER_ATOC_NODE* assnode = NULL;
    st_PACKER_LTOC_NODE* laynode = NULL;
    st_XORDEREDARRAY* tmplist = NULL;

    if (!pr) {
        return;
    }

    for (j = pr->laytoc.cnt - 1; j >= 0; j--) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[j];

        for (i = laynode->assref.cnt - 1; i >= 0; i--) {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];

            if (assnode->typeref)
                if (assnode->typeref->assetUnloaded)
                    if (!(assnode->loadflag & PKR_LDASS_ISDUP))
                        assnode->typeref->assetUnloaded(assnode->memloc, assnode->aid);
        }
    }

    for (i = 0; i < pr->laytoc.cnt; i++) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        if (!laynode->laymem) {
            continue;
        }

        PKR_LayerMemRelease(pr, laynode);
        laynode->laymem = NULL;
    }

    PKR_kiilpool_anode(pr);

    for (i = 0; i < pr->laytoc.cnt; i++) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        PKR_oldlaynode(laynode);
    }

    XOrdDone(&pr->asstoc, FALSE);
    XOrdDone(&pr->laytoc, FALSE);

    for (i = 0; i < PKR_MAX_TYPES+1; i++) {
        tmplist = &pr->typelist[i];
        if (tmplist->max)
            XOrdDone(tmplist, FALSE);
    }

    if (pr->pkg) {
        g_hiprf->destroy(pr->pkg);
        pr->pkg = NULL;
    }

    lockid = pr->lockid;
    memset(pr, 0, sizeof(st_PACKER_READ_DATA));
    g_loadlock &= ~(1 << lockid);
}

static S32 PKR_SetActive(st_PACKER_READ_DATA* pr, en_LAYER_TYPE layer)
{
    S32 result = TRUE;
    S32 rc = 0;
    S32 i = 0;
    S32 j = 0;
    st_PACKER_ATOC_NODE* assnode = NULL;
    st_PACKER_LTOC_NODE* laynode = NULL;

    if (!pr) return FALSE;

    for (i = 0; i < pr->laytoc.cnt; i++) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];

        if (layer > PKR_LTYPE_DEFAULT && laynode->laytyp != layer) {
            continue;
        }

        for (j = 0; j < laynode->assref.cnt; j++) {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[j];

            xOutBabble("PACKER", "PKR: Setting Active LID: %d (%s) AID: 0x%08X (%s)\n", laynode->laytyp,
                       g_strz_layername[laynode->laytyp], assnode->aid,
                       (assnode->loadflag & PKR_LDASS_READY) ? "Ready" : "Not Ready! - Skipping");

            if (assnode->loadflag & PKR_LDASS_ACTIVE) continue;
            if (!(assnode->loadflag & PKR_LDASS_READY)) {
                continue;
            }

            if (!assnode->typeref) {
                xOutWarn("PACKER", "PKR: Asset '%s' has unhandled type %s (default support: Retrieve asset by ID only!)\n",
                         assnode->name, xUtil_idtag2string(assnode->asstype, 0));
                continue;
            }

            if (!assnode->typeref->assetLoaded) continue;

            rc = assnode->typeref->assetLoaded(pr->userdata, assnode->aid, assnode->memloc, assnode->d_size);
            if (!rc) {
                result = FALSE;
            } else {
                assnode->loadflag |= PKR_LDASS_ACTIVE;
            }
        }
    }

    return result;
}

static S32 PKR_parse_TOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 is_ok = FALSE;
    U32 cid = 0;
    S32 done = FALSE;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'HIPA':
            is_ok = LOD_r_HIPA(pkg, pr);
            break;
        case 'PACK':
            LOD_r_PACK(pkg, pr);
            if (pr->asscnt <= 0)
                xOutWarn("PACKER", "PKR: No assets\n");
            if (is_ok) {
                if (pr->asscnt > 0) {
                    XOrdInit(&pr->asstoc, pr->asscnt, FALSE);
                    PKR_makepool_anode(pr, pr->asscnt);
                }
                if (pr->laycnt > 0) XOrdInit(&pr->laytoc, pr->laycnt, FALSE);
            }
            break;
        case 'DICT':
            LOD_r_DICT(pkg, pr);
            PKR_bld_typecnt(pr);
            done = TRUE;
            break;
        case 'STRM':
            LOD_r_STRM(pkg, pr);
            break;
        }
        if (!is_ok) {
            break;
        }
        g_hiprf->exit(pkg);
        if (done) break; cid = g_hiprf->enter(pkg);
    }

    return is_ok;
}

static S32 PKR_LoadStep_Async()
{
    S32 moretodo = TRUE;
    S32 rc = 0;
    en_READ_ASYNC_STATUS readstat = HIP_RDSTAT_NONE;
    st_PACKER_ATOC_NODE* tmpass;

    static st_PACKER_READ_DATA* curpr = NULL;
    static st_PACKER_LTOC_NODE* asynlay = NULL;

    if (!asynlay) {
        PKR_findNextLayerToLoad(&curpr, &asynlay);
        if (asynlay) {
            en_PKR_LAYER_LOAD_DEST loaddest = PKR_layerLoadDest(asynlay->laytyp);
            if (loaddest != PKR_LDDEST_SKIP && asynlay->laysize > 1 && asynlay->assref.cnt > 0) {
                asynlay->laymem = PKR_LayerMemReserve(curpr, asynlay);

                PKR_drv_guardLayer(asynlay);

                tmpass = (st_PACKER_ATOC_NODE*)asynlay->assref.list[0];

                g_hiprf->setSpot(tmpass->ownpkg, tmpass->d_off);

                rc = g_hiprf->readBytes(tmpass->ownpkg, asynlay->laymem, asynlay->laysize);
                if (rc) {
                    asynlay->flg_ldstat |= PKR_LDLAY_INPROG;
                } else {
                    PKR_LayerMemRelease(curpr, asynlay);
                    asynlay->flg_ldstat &= ~(PKR_LDLAY_INPROG | PKR_LDLAY_ISDONE);
                    curpr = NULL;
                    asynlay = NULL;
                }

                moretodo = TRUE;
            } else {
                asynlay->flg_ldstat |= PKR_LDLAY_ISDONE;
                asynlay = NULL;
                moretodo = TRUE;
            }
        } else {
            curpr = NULL;
            asynlay = NULL;
            moretodo = FALSE;
        }
    } else {
        readstat = g_hiprf->pollRead(curpr->pkg);
        if (readstat == HIP_RDSTAT_SUCCESS) {
            readstat = PKR_drv_guardVerify(asynlay);
        }

        if (readstat == HIP_RDSTAT_INPROG) {
            moretodo = TRUE;
        }
        else if (readstat == HIP_RDSTAT_SUCCESS) {
            PKR_updateLayerAssets(asynlay);

            if (PKR_layerTypeNeedsXForm(asynlay->laytyp)) {
                PKR_xformLayerAssets(asynlay);
            }

            en_PKR_LAYER_LOAD_DEST loaddest = PKR_layerLoadDest(asynlay->laytyp);
            if (loaddest == PKR_LDDEST_RWHANDOFF) {
                PKR_LayerMemRelease(curpr, asynlay);
            }

            asynlay->flg_ldstat |= PKR_LDLAY_ISDONE;
            asynlay = NULL;
            moretodo = TRUE;
        } else {
            en_PKR_LAYER_LOAD_DEST loaddest = PKR_layerLoadDest(asynlay->laytyp);
            if (asynlay->laymem && loaddest == PKR_LDDEST_RWHANDOFF) {
                PKR_LayerMemRelease(curpr, asynlay);
            }

            asynlay->flg_ldstat &= ~(PKR_LDLAY_INPROG | PKR_LDLAY_ISDONE);
            asynlay = NULL;
            curpr = NULL;
            moretodo = TRUE;
        }
    }

    return moretodo;
}

static char* PKR_LayerMemReserve(st_PACKER_READ_DATA* pr, st_PACKER_LTOC_NODE* layer)
{
    char* mem = NULL;
    en_PKR_LAYER_LOAD_DEST loaddest;

    if (layer->laymem) return layer->laymem;

    loaddest = PKR_layerLoadDest(layer->laytyp);
    switch (loaddest) {
    case PKR_LDDEST_SKIP:
        break;
    case PKR_LDDEST_KEEPSTATIC:
        mem = (char*)PKR_getmem('LYR\0', layer->laysize, 0x8000 + layer->laytyp, 64);
        break;
    case PKR_LDDEST_KEEPMALLOC:
        mem = (char*)PKR_getmem('LYR\0', layer->laysize, 0x8000 + layer->laytyp, 64, TRUE,
                                &layer->laytru);
        break;
    case PKR_LDDEST_RWHANDOFF:
        PKR_push_memmark();
        mem = (char*)PKR_getmem('LYR\0', layer->laysize, 0x8000 + layer->laytyp, 64);
        break;
    }

    return mem;
}

static void PKR_LayerMemRelease(st_PACKER_READ_DATA*, st_PACKER_LTOC_NODE* layer)
{
    en_PKR_LAYER_LOAD_DEST loaddest = PKR_layerLoadDest(layer->laytyp);

    switch (loaddest) {
    case PKR_LDDEST_SKIP:
        break;
    case PKR_LDDEST_RWHANDOFF:
        PKR_relmem('LYR\0', layer->laysize, layer->laymem, 0x8000 + layer->laytyp, FALSE);
        PKR_pop_memmark();
        layer->laymem = NULL;
        break;
    case PKR_LDDEST_KEEPSTATIC:
        PKR_relmem('LYR\0', layer->laysize, layer->laymem, 0x8000 + layer->laytyp, FALSE);
        break;
    case PKR_LDDEST_KEEPMALLOC:
        PKR_relmem('LYR\0', layer->laysize, layer->laytru, 0x8000 + layer->laytyp, TRUE);
        layer->laymem = NULL;
        layer->laytru = NULL;
        break;
    }
}

void PKR_drv_guardLayer(st_PACKER_LTOC_NODE*)
{
}

en_READ_ASYNC_STATUS PKR_drv_guardVerify(st_PACKER_LTOC_NODE*)
{
    en_READ_ASYNC_STATUS confirm = HIP_RDSTAT_SUCCESS;

    return confirm;
}

static en_PKR_LAYER_LOAD_DEST PKR_layerLoadDest(en_LAYER_TYPE laytyp)
{
    en_PKR_LAYER_LOAD_DEST load_place = PKR_LDDEST_KEEPSTATIC;

    switch (laytyp) {
    case PKR_LTYPE_TEXTURE_STRM:
    case PKR_LTYPE_SRAM:
    case PKR_LTYPE_SNDTOC:
    case PKR_LTYPE_CUTSCENE:
        load_place = PKR_LDDEST_SKIP;
        break;
    case PKR_LTYPE_TEXTURE:
    case PKR_LTYPE_BSP:
    case PKR_LTYPE_MODEL:
        load_place = PKR_LDDEST_RWHANDOFF;
        break;
    case PKR_LTYPE_ANIMATION:
    case PKR_LTYPE_JSPINFO:
        load_place = PKR_LDDEST_KEEPMALLOC;
        break;
    case PKR_LTYPE_DEFAULT:
    case PKR_LTYPE_VRAM:
    case PKR_LTYPE_CUTSCENETOC:
        load_place = PKR_LDDEST_KEEPSTATIC;
        break;
    default:
        load_place = PKR_LDDEST_KEEPSTATIC;
        break;
    }

    return load_place;
}

static S32 PKR_layerTypeNeedsXForm(en_LAYER_TYPE laytyp)
{
    S32 do_xform = FALSE;

    switch (laytyp) {
    case PKR_LTYPE_TEXTURE:
    case PKR_LTYPE_BSP:
    case PKR_LTYPE_MODEL:
        do_xform = TRUE;
        break;
    case PKR_LTYPE_DEFAULT:
    case PKR_LTYPE_ANIMATION:
    case PKR_LTYPE_VRAM:
    case PKR_LTYPE_SNDTOC:
    case PKR_LTYPE_CUTSCENETOC:
    case PKR_LTYPE_JSPINFO:
        do_xform = TRUE;
        break;
    case PKR_LTYPE_SRAM:
    case PKR_LTYPE_CUTSCENE:
        do_xform = FALSE;
        break;
    default:
        do_xform = FALSE;
        break;
    }

    return do_xform;
}

static S32 PKR_findNextLayerToLoad(st_PACKER_READ_DATA** work_on_pkg, st_PACKER_LTOC_NODE** next_layer)
{
    st_PACKER_READ_DATA* tmppr = NULL;
    st_PACKER_LTOC_NODE* tmplay = NULL;
    S32 i = 0;
    S32 j = 0;

    *next_layer = NULL;

    tmppr = *work_on_pkg;
    if (tmppr) {
        for (i = 0; i < tmppr->laytoc.cnt; i++) {
            tmplay = (st_PACKER_LTOC_NODE*)tmppr->laytoc.list[i];
            if (tmplay->flg_ldstat & PKR_LDLAY_ISDONE) continue;

            *next_layer = tmplay;
            *work_on_pkg = tmppr;
            break;
        }
    }

    if (!*next_layer) {
        for (j = 0; j < PKR_MAX_INSTANCES; j++) {
            if (!(g_loadlock & (1 << j))) continue;

            tmppr = &g_readdatainst[j];
            if (tmppr == *work_on_pkg) continue;

            for (i = 0; i < tmppr->laytoc.cnt; i++) {
                tmplay = (st_PACKER_LTOC_NODE*)tmppr->laytoc.list[i];
                if (tmplay->flg_ldstat & PKR_LDLAY_ISDONE) continue;

                *next_layer = tmplay;
                *work_on_pkg = tmppr;
                break;
            }
            if (*next_layer) break;
        }
    }

    return *next_layer ? TRUE : FALSE;
}

static void PKR_updateLayerAssets(st_PACKER_LTOC_NODE* laynode)
{
    S32 i = 0;
    st_PACKER_ATOC_NODE* tmpass = NULL;
    for (i = 0; i < laynode->assref.cnt; i++) {
        tmpass = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];
        if (tmpass->d_off > 0 && tmpass->d_size > 0) break;
        tmpass = NULL;
    }

    S32 lay_hip_pos = tmpass->d_off;

    for (i = 0; i < laynode->assref.cnt; i++) {
        tmpass = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];

        if (tmpass->loadflag & PKR_LDASS_ISDUP) {
            continue;
        }

        if (tmpass->loadflag & PKR_LDASS_ISNIL) {
            tmpass->memloc = NULL;
            continue;
        }

        S32 reloff = tmpass->d_off - lay_hip_pos;

        tmpass->memloc = laynode->laymem + reloff;
        tmpass->loadflag |= PKR_LDASS_READY;
    }
}

static void PKR_xformLayerAssets(st_PACKER_LTOC_NODE* laynode)
{
    S32 i = 0;
    S32 will_be_dumped = FALSE;
    st_PACKER_ATOC_NODE* tmpass = NULL;

    en_PKR_LAYER_LOAD_DEST loaddest = PKR_layerLoadDest(laynode->laytyp);
    if (loaddest == PKR_LDDEST_RWHANDOFF) will_be_dumped = TRUE;

    for (i = 0; i < laynode->assref.cnt; i++) {
        tmpass = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];
        if (tmpass->loadflag & PKR_LDASS_ISDUP) {
            continue;
        }

        PKR_xform_asset(tmpass, will_be_dumped);

        if (will_be_dumped && tmpass->x_size < 1) {
            tmpass->memloc = NULL;
        }
    }
}

static void PKR_xform_asset(st_PACKER_ATOC_NODE* assnode, S32 dumpable_layer)
{
    char* xformloc = NULL;
    const PKRAssetType* atype;

    if (!(assnode->infoflag & PKR_INFO_READ_TRANSFORM)) {
        if (assnode->typeref && assnode->typeref->readXForm) {
            xOutWarn("PACKER", "PKR: Asset '%s', (of type %s) supports read transformations but was not flagged for transform processing during loading - is this intentional?\n",
                     assnode->name, xUtil_idtag2string(assnode->asstype, 0));
        } else {
            return;
        }
    }

    atype = assnode->typeref;
    if (!atype) {
        return;
    }

    if (!atype->readXForm) {
        return;
    }

    if (assnode->d_size == 0) {
        xOutWarn("PACKER", "PKR: Read XForm of %s (%s) has no data - processing anyway\n", assnode->name,
                 xUtil_idtag2string(assnode->asstype, 0));
        assnode->memloc = NULL;
    }

    xformloc = (char*)atype->readXForm(assnode->ownpr->userdata, assnode->aid, assnode->memloc, assnode->d_size,
                                       (U32*)&assnode->x_size);

    if (!dumpable_layer && assnode->memloc == xformloc && assnode->x_size) {
    }
    else if (assnode->d_size == 0 || assnode->x_size == 0) {
        assnode->memloc = NULL;
        assnode->loadflag |= PKR_LDASS_ISNIL;
    } else
        assnode->memloc = xformloc;
}

static void* PKR_FindAsset(st_PACKER_READ_DATA* pr, U32 aid, S32* assetsize)
{
    S32 idx = -1;
    st_PACKER_ATOC_NODE* assnode = NULL;

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx >= 0) {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
    }

    if (assetsize) {
        if (!assnode) {
            *assetsize = 0;
        }
        else if (!assnode->memloc) {
            *assetsize = 0;
        }
        else if (assnode->x_size > 0) {
            *assetsize = assnode->x_size;
        } else {
            *assetsize = assnode->d_size;
        }
    }

    if (assnode) {
        return assnode->memloc;
    }

    return NULL;
}

static S32 PKR_LoadLayer(st_PACKER_READ_DATA*, en_LAYER_TYPE)
{
    return 0;
}

static void* PKR_LoadAsset(st_PACKER_READ_DATA* pr, U32 aid, const char*, void*, S32* assetsize)
{
    return PKR_FindAsset(pr, aid, assetsize);
}

static U32 PKR_GetAssetSize(st_PACKER_READ_DATA* pr, U32 aid)
{
    S32 idx = -1;
    const st_PACKER_ATOC_NODE* assnode = NULL;

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx > -1) {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
    }

    if (assnode) {
        if (assnode->x_size > 0) return assnode->x_size;
        return assnode->d_size;
    }

    return 0;
}

static S32 PKR_AssetCount(st_PACKER_READ_DATA* pr, U32 type)
{
    S32 cnt = 0;
    S32 idx = -1;
    st_XORDEREDARRAY* tmplist = NULL;

    if (type == 0) return pr->asstoc.cnt;

    idx = PKR_typeHdlr_idx(pr, type);

    if (idx >= 0) {
        tmplist = &pr->typelist[idx];
        cnt = tmplist->cnt;
    }

    return cnt;
}

static void* PKR_AssetByType(st_PACKER_READ_DATA* pr, U32 type, S32 idx, U32* size)
{
    void* memloc = NULL;
    S32 typidx = 0;
    st_XORDEREDARRAY* typlist = NULL;
    st_PACKER_ATOC_NODE* assnode = NULL;

    if (size) *size = 0;

    if (idx < 0) idx = 0;

    typidx = PKR_typeHdlr_idx(pr, type);
    if (typidx < 0) return memloc;

    typlist = &pr->typelist[typidx];
    if (idx >= typlist->cnt) return memloc;

    assnode = (st_PACKER_ATOC_NODE*)typlist->list[idx];

    if (size) *size = assnode->d_size;

    memloc = assnode->memloc;

    return memloc;
}

static S32 PKR_IsAssetReady(st_PACKER_READ_DATA* pr, U32 aid)
{
    S32 is_ok = FALSE;
    st_PACKER_ATOC_NODE* assnode;
    S32 idx = -1;

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx < 0) {
    } else {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];

        if (assnode->loadflag & PKR_LDASS_READY) is_ok = TRUE;
        else is_ok = FALSE;
    }

    return is_ok;
}

static U32 PKR_getPackTimestamp(st_PACKER_READ_DATA* pr)
{
    return pr->time_made;
}

static void PKR_Disconnect(st_PACKER_READ_DATA* pr)
{
    if (pr->pkg) {
        g_hiprf->destroy(pr->pkg);
        pr->pkg = NULL;
    }
}

static const char* PKR_AssetName(st_PACKER_READ_DATA* pr, U32 aid)
{
    const char* da_name = NULL;
    S32 idx = -1;
    st_PACKER_ATOC_NODE* assnode = NULL;

    if (!aid) return da_name;

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx >= 0) {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
        da_name = assnode->name;
    }

    return da_name;
}

static U32 PKR_GetBaseSector(st_PACKER_READ_DATA* pr)
{
    return pr->base_sector;
}

static S32 PKR_GetAssetInfo(st_PACKER_READ_DATA* pr, U32 aid, PKRAssetTOCInfo* tocinfo)
{
    S32 idx = -1;
    st_PACKER_ATOC_NODE* assnode = NULL;

    memset(tocinfo, 0, sizeof(PKRAssetTOCInfo));

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx < 0) return FALSE;

    assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];

    tocinfo->aid = aid;
    tocinfo->typeref = assnode->typeref;
    tocinfo->sector = assnode->d_off / pkr_sector_size;
    tocinfo->plus_offset = assnode->d_off % pkr_sector_size;
    tocinfo->size = assnode->d_size;
    tocinfo->mempos = assnode->memloc;

    return TRUE;
}

static S32 PKR_GetAssetInfoByType(st_PACKER_READ_DATA* pr, U32 type, S32 idx, PKRAssetTOCInfo* tocinfo)
{
    st_PACKER_ATOC_NODE* assnode = NULL;
    S32 typidx = 0;
    st_XORDEREDARRAY* typlist = NULL;

    memset(tocinfo, 0, sizeof(PKRAssetTOCInfo));

    if (idx < 0) idx = 0;

    typidx = PKR_typeHdlr_idx(pr, type);
    if (typidx < 0) return FALSE;

    typlist = &pr->typelist[typidx];
    if (idx >= typlist->cnt) return FALSE;

    assnode = (st_PACKER_ATOC_NODE*)typlist->list[idx];

    tocinfo->aid = assnode->aid;
    tocinfo->typeref = assnode->typeref;
    tocinfo->sector = assnode->d_off / pkr_sector_size;
    tocinfo->plus_offset = assnode->d_off % pkr_sector_size;
    tocinfo->size = assnode->d_size;
    tocinfo->mempos = assnode->memloc;

    return TRUE;
}

static S32 PKR_PkgHasAsset(st_PACKER_READ_DATA* pr, U32 aid)
{
    S32 rc = FALSE;
    S32 idx = -1;
    st_PACKER_ATOC_NODE* assnode = NULL;

    idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);

    if (idx < 0) rc = FALSE;
    else {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];

        rc = TRUE;

        if (assnode->loadflag & PKR_LDASS_ISDUP) rc = FALSE;
        else if (assnode->loadflag & PKR_LDASS_ISNIL) rc = FALSE;
    }

    return rc;
}

static S32 PKR_FRIEND_assetIsGameDup(U32 aid, const st_PACKER_READ_DATA* skippr, S32 oursize, U32 ourtype, U32 chksum, char* our_fnam)
{
    S32 is_dup = FALSE;
    st_PACKER_READ_DATA* tmp_pr = NULL;
    const st_PACKER_ATOC_NODE* tmp_ass = NULL;
    S32 i = 0;
    S32 idx = -1;

    if (aid == SIMPLE_SHADOW_TABLE_ID) return is_dup;

    for (i = 0; i < PKR_MAX_INSTANCES; i++) {
        if (!(g_loadlock & (1 << i))) {
            continue;
        }

        tmp_pr = &g_readdatainst[i];
        if (tmp_pr == skippr) {
            continue;
        }

        idx = XOrdLookup(&tmp_pr->asstoc, (void*)aid, OrdTest_R_AssetID);
        if (idx < 0) {
            continue;
        }

        tmp_ass = (st_PACKER_ATOC_NODE*)tmp_pr->asstoc.list[idx];

        if (!(tmp_ass->loadflag & PKR_LDASS_READY)) {
            if (tmp_ass->asstype != 'SND ' && tmp_ass->asstype != 'SNDS') {
                continue;
            }
        }

        xOutWarn("PACKER", "PKR: Dupe Game Asset 0x%08X '%s' (%s)\n\t(Orig: '%s' Dupe: '%s')\n", tmp_ass->aid, tmp_ass->name,
                 xUtil_idtag2string(tmp_ass->asstype, 0), tmp_pr->packfile, !skippr ? "<unknown>" : skippr->packfile);

        if (ourtype != 0 && ourtype != tmp_ass->asstype) {
            xOutCrit("PACKER", "---: ---- '%s' - inconsistent types (%s/%s)\n", tmp_ass->name,
                     xUtil_idtag2string(tmp_ass->asstype, 0), xUtil_idtag2string(ourtype, 1));
        }

        if (oursize >= 0 && oursize != tmp_ass->d_size) {
            xOutErr("PACKER", "---: ---- '%s' - inconsistent sizes (%d/%d) (%s)\n", tmp_ass->name, tmp_ass->d_size, oursize,
                    xUtil_idtag2string(tmp_ass->asstype, 0));
        }

        if (chksum != 0 && chksum != tmp_ass->d_chksum) {
            xOutErr("PACKER", "---: ---- '%s' - inconsistent data (0x%08X/0x%08X) (%s)\n", tmp_ass->name, tmp_ass->d_chksum,
                    chksum, xUtil_idtag2string(tmp_ass->asstype, 0));
        }

        is_dup = TRUE;
        break;
    }

    return is_dup;
}

static S32 PKR_makepool_anode(st_PACKER_READ_DATA* pr, S32 cnt)
{
    st_PACKER_ATOC_NODE* asspool = NULL;
    S32 amount = 0;

    if (!cnt) return amount;

    amount = cnt * sizeof(st_PACKER_ATOC_NODE);
    asspool = (st_PACKER_ATOC_NODE*)PKR_getmem('ANOD', amount, 'FAKE', 64);

    if (asspool) {
        pr->pool_anode = asspool;
        pr->pool_nextaidx = 0;
    }

    return asspool ? amount : 0;
}

static void PKR_kiilpool_anode(st_PACKER_READ_DATA* pr)
{
    S32 amount = 0;

    if (pr->asscnt) {
        amount = pr->asscnt * sizeof(st_PACKER_ATOC_NODE);
        PKR_relmem('ANOD', amount, pr->pool_anode, 'FAKE', FALSE);
        pr->pool_anode = NULL;
        pr->pool_nextaidx = 0;
    }
}

static st_PACKER_ATOC_NODE* PKR_newassnode(st_PACKER_READ_DATA* pr, U32 aid)
{
    st_PACKER_ATOC_NODE* assnode = NULL;

    assnode = &pr->pool_anode[pr->pool_nextaidx];
    pr->pool_nextaidx++;

    memset(assnode, 0, sizeof(st_PACKER_ATOC_NODE));
    assnode->aid = aid;

    return assnode;
}

static st_PACKER_LTOC_NODE* PKR_newlaynode(en_LAYER_TYPE laytyp, S32 refcnt)
{
    st_PACKER_LTOC_NODE* laynode = NULL;

    laynode = (st_PACKER_LTOC_NODE*)PKR_getmem('LNOD', sizeof(st_PACKER_LTOC_NODE), 0x8000 + laytyp, 64);
    memset(laynode, 0, sizeof(st_PACKER_LTOC_NODE));

    laynode->laytyp = laytyp;
    XOrdInit(&laynode->assref, (refcnt > 1) ? refcnt : 2, FALSE);

    return laynode;
}

static void PKR_oldlaynode(st_PACKER_LTOC_NODE* laynode)
{
    XOrdDone(&laynode->assref, FALSE);
    PKR_relmem('LNOD', sizeof(st_PACKER_LTOC_NODE), laynode, 0x8000 + laynode->laytyp, FALSE);
}

static S32 OrdComp_R_Asset(void* vkey, void* vitem)
{
    S32 rc = 0;
    st_PACKER_ATOC_NODE* key = (st_PACKER_ATOC_NODE*)vkey;
    st_PACKER_ATOC_NODE* item = (st_PACKER_ATOC_NODE*)vitem;

    if (key->aid < item->aid) rc = -1;
    else if (key->aid > item->aid) rc = 1;
    else rc = 0;

    return rc;
}

static S32 OrdTest_R_AssetID(const void* vkey, void* vitem)
{
    S32 rc = 0;
    U32 key = (U32)vkey;
    st_PACKER_ATOC_NODE* item = (st_PACKER_ATOC_NODE*)vitem;

    if (key < item->aid) rc = -1;
    else if (key > item->aid) rc = 1;
    else rc = 0;

    return rc;
}

static S32 LOD_r_HIPA(st_HIPLOADDATA*, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;

    pr->pkgver = 'HIPA';

    return result;
}

static S32 LOD_r_PACK(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'PVER':
            LOD_r_PVER(pkg, pr);
            break;
        case 'PFLG':
            LOD_r_PFLG(pkg, pr);
            break;
        case 'PCNT':
            LOD_r_PCNT(pkg, pr);
            break;
        case 'PCRT':
            LOD_r_PCRT(pkg, pr);
            break;
        case 'PMOD':
            LOD_r_PMOD(pkg, pr);
            break;
        case 'PLAT':
            LOD_r_PLAT(pkg, pr);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    return result;
}

static S32 LOD_r_PVER(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    S32 ver = 0;
    S32 amt = 0;

    amt = g_hiprf->readLongs(pkg, &ver, 1);
    pr->subver = ver;
    if (ver < 2) {
        PKR_spew_verhist();
    }

    ver = -1;
    amt = g_hiprf->readLongs(pkg, &ver, 1);
    pr->cltver = ver;

    ver = -1;
    amt = g_hiprf->readLongs(pkg, &ver, 1);
    if (amt != 1) {
        pr->compatver = 1;
    } else {
        pr->compatver = ver;
    }

    return result;
}

static S32 LOD_r_PFLG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*)
{
    S32 result = TRUE;
    S32 pkgflags = 0;

    g_hiprf->readLongs(pkg, &pkgflags, 1);

    return result;
}

static S32 LOD_r_PCNT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    S32 ival = 0;

    g_hiprf->readLongs(pkg, &ival, 1);
    pr->asscnt = ival;

    g_hiprf->readLongs(pkg, &ival, 1);
    pr->laycnt = ival;

    g_hiprf->readLongs(pkg, &ival, 1);

    g_hiprf->readLongs(pkg, &ival, 1);

    g_hiprf->readLongs(pkg, &ival, 1);

    return result;
}

static S32 LOD_r_PCRT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    S32 ival = 0;
    char tymbuf[256] = {};

    g_hiprf->readLongs(pkg, &ival, 1);
    pr->time_made = ival;

    if (pr->subver > 1) {
        g_hiprf->readString(pkg, tymbuf);
    }

    return result;
}

static S32 LOD_r_PMOD(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    S32 ival = 0;

    g_hiprf->readLongs(pkg, &ival, 1);
    pr->time_mod = ival;

    return result;
}

static S32 ValidatePlatform(st_HIPLOADDATA*, st_PACKER_READ_DATA*, char* plat, char* vid, char* lang, char* title)
{
    char fullname[128] = {};
    sprintf(fullname, "%s %s %s %s", plat, vid, lang, title);

    S32 rc = !stricmp(plat, "GC");
    if (!rc) return FALSE;

    if (zMainIsNTSC())
        rc = !stricmp(vid, "NTSC");
    else if (zMainIsPAL())
        rc = !stricmp(vid, "PAL");
    if (!rc) return FALSE;

    rc = !stricmp(title, "Incredibles");
    if (!rc) return FALSE;

    return TRUE;
}

static S32 LOD_r_PLAT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    char platname[32] = {};
    char vidname[32] = {};
    char langname[32] = {};
    char titlename[32] = {};
    S32 n, rc;

    n = g_hiprf->readString(pkg, platname);

    n = g_hiprf->readString(pkg, vidname);

    n = g_hiprf->readString(pkg, langname);

    n = g_hiprf->readString(pkg, titlename);
    if (!n) strcpy(titlename, "<Unknown>");

    rc = ValidatePlatform(pkg, pr, platname, vidname, langname, titlename);
    if (!rc) result = FALSE;

    return result;
}

static S32 LOD_r_DICT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'ATOC':
            LOD_r_ATOC(pkg, pr);

            XOrdSort(&pr->asstoc, OrdComp_R_Asset);
            break;
        case 'LTOC':
            LOD_r_LTOC(pkg, pr);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    return result;
}

static S32 LOD_r_ATOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'AINF':
            LOD_r_AINF(pkg, pr);
            break;
        case 'AHDR':
            LOD_r_AHDR(pkg, pr);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    return result;
}

static S32 LOD_r_AINF(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*)
{
    S32 result = TRUE;
    S32 ival = 0;

    g_hiprf->readLongs(pkg, &ival, 1);

    return result;
}

static S32 LOD_r_AHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;
    S32 ival = 0;
    S32 isdup = FALSE;
    st_PACKER_ATOC_NODE* assnode = NULL;

    g_hiprf->readLongs(pkg, &ival, 1);

    assnode = PKR_newassnode(pr, ival);
    assnode->ownpr = pr;
    assnode->ownpkg = pkg;
    XOrdAppend(&pr->asstoc, assnode);

    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->asstype = ival;
    assnode->typeref = PKR_type2typeref(assnode->asstype, pr->types);

    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->d_off = ival;

    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->d_size = ival;
    assnode->readrem = ival;
    assnode->readcnt = 0;
    if (assnode->d_size < 1) assnode->loadflag |= PKR_LDASS_ISNIL;

    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->d_pad = ival;

    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->infoflag = ival;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'ADBG':
            LOD_r_ADBG(pkg, pr, assnode);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    isdup = PKR_FRIEND_assetIsGameDup(assnode->aid, pr, assnode->d_size, assnode->asstype, assnode->d_chksum, NULL);
    if (isdup) assnode->loadflag |= PKR_LDASS_ISDUP;

    return result;
}

static S32 LOD_r_ADBG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr, st_PACKER_ATOC_NODE* assnode)
{
    S32 result = TRUE;
    S32 ival = 0;
    char tmpbuf[256] = {};

    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->assalign = ival;

    g_hiprf->readString(pkg, tmpbuf);
    strncpy(assnode->name, tmpbuf, sizeof(assnode->name) - 1);
    assnode->name[sizeof(assnode->name) - 1] = '\0';

    tmpbuf[0] = '\0';
    g_hiprf->readString(pkg, tmpbuf);

    if (pr->subver > 1) {
        g_hiprf->readLongs(pkg, &ival, 1);
        assnode->d_chksum = ival;
    }

    return result;
}

static S32 LOD_r_LTOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'LINF':
            LOD_r_LINF(pkg, pr);
            break;
        case 'LHDR':
            LOD_r_LHDR(pkg, pr);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    return result;
}

static S32 LOD_r_LINF(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*)
{
    S32 result = TRUE;
    S32 ival = 0;

    g_hiprf->readLongs(pkg, &ival, 1);

    return result;
}

static S32 LOD_r_LHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;
    S32 ival = 0;
    en_LAYER_TYPE laytyp = PKR_LTYPE_NOMORE;
    S32 refcnt = 0;
    S32 idx = -1;
    S32 i = 0;
    st_PACKER_LTOC_NODE* laynode = NULL;
    st_PACKER_ATOC_NODE* assnode = NULL;

    g_hiprf->readLongs(pkg, &ival, 1);
    laytyp = (en_LAYER_TYPE)ival;

    g_hiprf->readLongs(pkg, &refcnt, 1);

    laynode = PKR_newlaynode(laytyp, refcnt);
    XOrdAppend(&pr->laytoc, laynode);

    for (i = 0; i < refcnt; i++) {
        g_hiprf->readLongs(pkg, &ival, 1);

        idx = XOrdLookup(&pr->asstoc, (void*)ival, OrdTest_R_AssetID);
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
        XOrdAppend(&laynode->assref, assnode);

        if (i != refcnt - 1) {
            laynode->laysize += assnode->d_size + assnode->d_pad;
        } else {
            laynode->laysize += assnode->d_size;
        }
    }

    if (laynode->laysize > 0) {
        laynode->laysize = (laynode->laysize + 2047) & ~2047;
    }

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'LDBG':
            LOD_r_LDBG(pkg, pr, laynode);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    return result;
}

static S32 LOD_r_LDBG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr, st_PACKER_LTOC_NODE* laynode)
{
    S32 result = TRUE;
    S32 ival = 0;

    if (pr->subver > 1) {
        g_hiprf->readLongs(pkg, &ival, 1);
        laynode->chksum = ival;
    }

    return result;
}

static S32 LOD_r_STRM(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    S32 result = TRUE;
    U32 cid = 0;

    cid = g_hiprf->enter(pkg);
    while (cid) {
        switch (cid) {
        case 'DHDR':
            LOD_r_DHDR(pkg, pr);
            break;
        case 'DPAK':
            LOD_r_DPAK(pkg, pr);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    return result;
}

static S32 LOD_r_DHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA*)
{
    S32 result = TRUE;
    S32 ival = 0;

    g_hiprf->readLongs(pkg, &ival, 1);

    return result;
}

static S32 LOD_r_DPAK(st_HIPLOADDATA*, st_PACKER_READ_DATA*)
{
    S32 result = TRUE;

    return result;
}

static void PKR_spew_verhist()
{
}

static PKRAssetType* PKR_type2typeref(U32 type, PKRAssetType* typelist)
{
    PKRAssetType* da_type = NULL;
    PKRAssetType* tmptype = NULL;

    tmptype = typelist;
    if (tmptype) {
        while (tmptype->typetag) {
            if (tmptype->typetag == type) {
                da_type = tmptype;
                break;
            }

            tmptype++;
        }
    }

    if (!da_type) {
        xOutErr("PACKER", "PKR: Unknown asset type '%s'\n", xUtil_idtag2string(type, 0));
    }

    return da_type;
}

static void PKR_bld_typecnt(st_PACKER_READ_DATA* pr)
{
    st_PACKER_LTOC_NODE* laynode = NULL;
    st_PACKER_ATOC_NODE* assnode = NULL;
    S32 i = 0;
    S32 j = 0;
    S32 idx = -1;
    S32 typcnt[PKR_MAX_TYPES+1] = {};
    st_XORDEREDARRAY* tmplist = NULL;
    U32 lasttype = 0;
    S32 lasttidx = 0;

    for (j = 0; j < pr->laytoc.cnt; j++) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[j];

        for (i = 0; i < laynode->assref.cnt; i++) {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];

            if (assnode->loadflag & PKR_LDASS_ISDUP) {
                continue;
            }

            if (assnode->loadflag & PKR_LDASS_ISNIL) {
                continue;
            }

            if (lasttype && assnode->asstype == lasttype) {
                idx = lasttidx;
            } else {
                idx = PKR_typeHdlr_idx(pr, assnode->asstype);

                lasttype = assnode->asstype; lasttidx = idx;
            }

            if (idx < 0) {
                xOutWarn("PACKER", "PKR: Unknown asset type '%s' for AID: 0x%08X (%s)\n",
                         xUtil_idtag2string(assnode->asstype, 0), assnode->aid, assnode->name);
                typcnt[PKR_MAX_TYPES]++;
            } else {
                typcnt[idx]++;
            }
        }
    }

    for (i = 0; i < PKR_MAX_TYPES+1; i++) {
        if (typcnt[i] < 1) {
            continue;
        }

        tmplist = &pr->typelist[i];
        XOrdInit(tmplist, (typcnt[i] > 1) ? typcnt[i] : 2, FALSE);
    }

    for (j = 0; j < pr->laytoc.cnt; j++) {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[j];

        for (i = 0; i < laynode->assref.cnt; i++) {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];

            if (assnode->loadflag & PKR_LDASS_ISDUP) continue;
            if (assnode->loadflag & PKR_LDASS_ISNIL) {
                continue;
            }

            if (lasttype && assnode->asstype == lasttype) {
                idx = lasttidx;
            } else {
                idx = PKR_typeHdlr_idx(pr, assnode->asstype);

                lasttype = assnode->asstype; lasttidx = idx;
            }

            if (idx < 0) tmplist = &pr->typelist[PKR_MAX_TYPES];
            else
                tmplist = &pr->typelist[idx];

            XOrdAppend(tmplist, assnode);
        }
    }
}

static S32 PKR_typeHdlr_idx(st_PACKER_READ_DATA* pr, U32 type)
{
    S32 da_idx = -1;
    S32 cnt = 0;
    const PKRAssetType* atype = NULL;

    atype = pr->types;
    while (atype->typetag) {
        if (atype->typetag == type) {
            da_idx = cnt;
            break;
        }

        cnt++;
        atype++;
    }

    return da_idx;
}

static void PKR_alloc_chkidx()
{
}

static void* PKR_getmem(U32 id, S32 amount, U32 aid, S32 align)
{
    return PKR_getmem(id, amount, aid, align, FALSE, NULL);
}

static void* PKR_getmem(U32 id, S32 amount, U32 aid, S32 align, S32 isTemp, char** memtru)
{
    void* memptr = NULL;

    if (amount == 0) return memptr;

    if (isTemp) {
        memptr = RwMallocL(amount + align, rwMEMHINTDUR_NADURATION, 5055);
        if (memtru) *memtru = (char*)memptr;
        if (align) memptr = (void*)(((S32)memptr + align - 1) & -align);
    } else {
        memptr = xMEMALLOC(amount, align);
    }

    if (memptr) memset(memptr, 0, amount);

    g_memalloc_pair++; g_memalloc_runtot += amount;
    if (g_memalloc_runtot < 0) g_memalloc_runtot = amount;

    if (memptr) {
        xOutInfo("PACKER", "PKR: MID:%s AID:0x%08X grab at 0x%08X size %8d (paired #%3d)\n", xUtil_idtag2string(id, 0), aid,
                 memptr, amount, g_memalloc_pair);
    } else {
        xOutWarn("PACKER", "PKR: Allocation for %s size %d failed\n", xUtil_idtag2string(id, 0), amount);
    }

    return memptr;
}

static void PKR_relmem(U32 id, S32 blksize, void* memptr, U32 aid, S32 isTemp)
{
    g_memalloc_pair--; g_memalloc_runfree += blksize;
    if (g_memalloc_runfree < 0) g_memalloc_runfree = blksize;

    xOutInfo("PACKER", "PKR: MID:%s AID:0x%08X free at 0x%08X size %8d (paired #%3d)\n", xUtil_idtag2string(id, 0), aid,
             memptr, blksize, g_memalloc_pair);

    if (memptr) {
        if (blksize > 0) {
            if (isTemp) RwFree(memptr);
            else
                xOutInfo("PACKER", "PKR: ... note - skipped dealloc (game) (MID: %s 0x%08X)\n", xUtil_idtag2string(id, 1),
                         memptr);
        }
    }
}

static S32 PKR_push_memmark()
{
    S32 curlvl = 0;

    curlvl = xMemPushBase();
    return curlvl;
}

static S32 PKR_pop_memmark()
{
    S32 curlvl = 0;

    curlvl = xMemGetBase() - 1;
    curlvl = xMemPopBase(curlvl);
    return curlvl;
}
