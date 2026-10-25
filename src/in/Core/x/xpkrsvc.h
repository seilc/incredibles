#ifndef XPKRSVC_H
#define XPKRSVC_H

#include "xhipio.h"
#include "xordarray.h"
#include "iTime.h"

#include <time.h>

typedef enum en_LAYER_TYPE
{
    PKR_LTYPE_ALL = -1,
    PKR_LTYPE_DEFAULT = 0,
    PKR_LTYPE_TEXTURE,
    PKR_LTYPE_TEXTURE_STRM,
    PKR_LTYPE_BSP,
    PKR_LTYPE_MODEL,
    PKR_LTYPE_ANIMATION,
    PKR_LTYPE_VRAM,
    PKR_LTYPE_SRAM,
    PKR_LTYPE_SNDTOC,
    PKR_LTYPE_CUTSCENE,
    PKR_LTYPE_CUTSCENETOC,
    PKR_LTYPE_JSPINFO,
    PKR_LTYPE_NOMORE
} LAYER_TYPE;

typedef enum en_PKR_LAYER_LOAD_DEST
{
    PKR_LDDEST_SKIP,
    PKR_LDDEST_KEEPSTATIC,
    PKR_LDDEST_KEEPMALLOC,
    PKR_LDDEST_RWHANDOFF,
    PKR_LDDEST_NOMORE,
    PKR_LDDEST_FORCE = 0x7fffffff
} PKR_LAYER_LOAD_DEST;

struct PKRAssetType
{
    U32 typetag;
    U32 tflags;
    S32 typalign;
    void* (*readXForm)(void*, U32, void*, U32, U32*);
    void* (*writeXForm)(void*, U32, void*, void*, U32, U32*);
    S32 (*assetLoaded)(void*, U32, void*, S32);
    void* (*makeData)(void*, U32, void*, S32*, S32*);
    void (*cleanup)(void*, U32, void*);
    void (*assetUnloaded)(void*, U32);
    void (*writePeek)(void*, U32, void*, S8*);
};

struct PKRAssetTOCInfo
{
    U32 aid;
    PKRAssetType* typeref;
    U32 sector;
    U32 plus_offset;
    U32 size;
    void* mempos;
#if DEBUG || RELEASE
    const char* assetname;
    const char* filename;
#endif
};

struct st_PACKER_READ_DATA;

struct PKRReadFuncs
{
    U32 api_ver;
    st_PACKER_READ_DATA* (*Init)(void* userdata, const char* pkgfile, U32 opts, S32* cltver,
                                 PKRAssetType* typelist, S32 fileflags);
    void (*Done)(st_PACKER_READ_DATA*);
    S32 (*LoadLayer)(st_PACKER_READ_DATA*, LAYER_TYPE);
    U32 (*GetAssetSize)(st_PACKER_READ_DATA*, U32);
    void* (*LoadAsset)(st_PACKER_READ_DATA*, U32, const char*, void*, S32*);
    void* (*AssetByType)(st_PACKER_READ_DATA*, U32, S32, U32*);
    S32 (*AssetCount)(st_PACKER_READ_DATA*, U32);
    S32 (*IsAssetReady)(st_PACKER_READ_DATA*, U32);
    S32 (*SetActive)(st_PACKER_READ_DATA*, LAYER_TYPE);
    const char* (*AssetName)(st_PACKER_READ_DATA*, U32);
    U32 (*GetBaseSector)(st_PACKER_READ_DATA*);
    S32 (*GetAssetInfo)(st_PACKER_READ_DATA*, U32, PKRAssetTOCInfo*);
    S32 (*GetAssetInfoByType)(st_PACKER_READ_DATA*, U32, S32, PKRAssetTOCInfo*);
    S32 (*PkgHasAsset)(st_PACKER_READ_DATA*, U32);
    U32 (*PkgTimeStamp)(st_PACKER_READ_DATA*);
    void (*PkgDisconnect)(st_PACKER_READ_DATA*);
};

typedef struct st_PACKER_ATOC_NODE
{
    U32 aid;
    U32 asstype;
    S32 d_off;
    S32 d_size;
    S32 d_pad;
    U32 d_chksum;
    S32 assalign;
    S32 infoflag;
    S32 loadflag;
    char* memloc;
    S32 x_size;
    S32 readcnt;
    S32 readrem;
    PKRAssetType* typeref;
    HIPLOADDATA* ownpkg;
    st_PACKER_READ_DATA* ownpr;
    char name[32];
} PACKER_ATOC_NODE;

typedef struct st_PACKER_LTOC_NODE
{
    LAYER_TYPE laytyp;
    XORDEREDARRAY assref;
    S32 flg_ldstat;
    S32 danglecnt;
    U32 chksum;
    S32 laysize;
    char* laymem;
    char* laytru;
} PACKER_LTOC_NODE;

#define PACKER_TYPE_COUNT 129

typedef struct st_PACKER_READ_DATA
{
    PKRAssetType* types;
    void* userdata;
    U32 opts;
    U32 pkgver;
    S32 cltver;
    S32 subver;
    S32 compatver;
    HIPLOADDATA* pkg;
    U32 base_sector;
    S32 lockid;
    char packfile[128];
    S32 asscnt;
    S32 laycnt;
    XORDEREDARRAY asstoc;
    XORDEREDARRAY laytoc;
    PACKER_ATOC_NODE* pool_anode;
    S32 pool_nextaidx;
    XORDEREDARRAY typelist[PACKER_TYPE_COUNT];
    time_t time_made;
    time_t time_mod;
} PACKER_READ_DATA;

enum en_pkrfindmyass
{
    PKR_FIND_FAILED,
    PKR_FIND_EXACT,
    PKR_FIND_EXACT_XFORM,
    PKR_FIND_RANGE,
    PKR_FIND_RANGE_XFORM
};

PKRReadFuncs* PKRGetReadFuncs(S32 apiver);
S32 PKRStartup();
S32 PKRShutdown();
S32 PKRLoadStep(S32);
st_PACKER_ATOC_NODE* PKR_FindAssetNode(st_PACKER_READ_DATA* pr, U32 aid, S32* assetsize);
U32 PKRAssetIDFromInst(void* asset_inst);
void PKR_drv_guardLayer(PACKER_LTOC_NODE*);
READ_ASYNC_STATUS PKR_drv_guardVerify(PACKER_LTOC_NODE*);

#if DEBUG || RELEASE
en_pkrfindmyass PKRFindMyAsset(st_PACKER_READ_DATA* pr, PKRAssetTOCInfo* tocinfo, const void* ptr_raw);
#else
inline en_pkrfindmyass PKRFindMyAsset(st_PACKER_READ_DATA*, PKRAssetTOCInfo*, const void*)
{
    return PKR_FIND_FAILED;
}
#endif

#endif