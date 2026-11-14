#ifndef XPKRSVC_H
#define XPKRSVC_H

#include "xhipio.h"
#include "xordarray.h"

#include <time.h>

enum en_LAYER_TYPE
{
    PKR_LTYPE_ALL = -1,
    PKR_LTYPE_DEFAULT,
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
};

struct PKRAssetType
{
    U32 typetag;
    U32 tflags;
    S32 typalign;
    void*(*readXForm)(void*, U32, void*, U32, U32*);
    void*(*writeXForm)(void*, U32, void*, void*, U32, U32*);
    S32(*assetLoaded)(void*, U32, void*, S32);
    void*(*makeData)(void*, U32, void*, S32*, S32*);
    void(*cleanup)(void*, U32, void*);
    void(*assetUnloaded)(void*, U32);
    void(*writePeek)(void*, U32, void*, char*);
};

struct PKRAssetTOCInfo
{
    U32 aid;
    PKRAssetType* typeref;
    U32 sector;
    U32 plus_offset;
    U32 size;
    void* mempos;
};

struct st_HIPLOADDATA;
struct st_PACKER_LTOC_NODE;
struct st_PACKER_READ_DATA;

struct PKRReadFuncs
{
    U32 api_ver;
    st_PACKER_READ_DATA*(*Init)(void*, const char*, U32, S32*, PKRAssetType*, S32);
    void(*Done)(st_PACKER_READ_DATA*);
    S32(*LoadLayer)(st_PACKER_READ_DATA*, en_LAYER_TYPE);
    U32(*GetAssetSize)(st_PACKER_READ_DATA*, U32);
    void*(*LoadAsset)(st_PACKER_READ_DATA*, U32, const char*, void*, S32*);
    void*(*AssetByType)(st_PACKER_READ_DATA*, U32, S32, U32*);
    S32(*AssetCount)(st_PACKER_READ_DATA*, U32);
    S32(*IsAssetReady)(st_PACKER_READ_DATA*, U32);
    S32(*SetActive)(st_PACKER_READ_DATA*, en_LAYER_TYPE);
    const char*(*AssetName)(st_PACKER_READ_DATA*, U32);
    U32(*GetBaseSector)(st_PACKER_READ_DATA*);
    S32(*GetAssetInfo)(st_PACKER_READ_DATA*, U32, PKRAssetTOCInfo*);
    S32(*GetAssetInfoByType)(st_PACKER_READ_DATA*, U32, S32, PKRAssetTOCInfo*);
    S32(*PkgHasAsset)(st_PACKER_READ_DATA*, U32);
    U32(*PkgTimeStamp)(st_PACKER_READ_DATA*);
    void(*PkgDisconnect)(st_PACKER_READ_DATA*);
};

// st_PACKER_ATOC_NODE::infoflag
#define PKR_INFO_SOURCE_FILE 0x1
#define PKR_INFO_SOURCE_VIRTUAL 0x2
#define PKR_INFO_READ_TRANSFORM 0x4
#define PKR_INFO_WRITE_TRANSFORM 0x8

// st_PACKER_ATOC_NODE::loadflag
#define PKR_LDASS_ACTIVE 0x10000
#define PKR_LDASS_READY 0x80000
#define PKR_LDASS_ISDUP 0x100000
#define PKR_LDASS_ISNIL 0x200000

struct st_PACKER_ATOC_NODE
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
    st_HIPLOADDATA* ownpkg;
    st_PACKER_READ_DATA* ownpr;
    char name[32];
};

#define PACKER_TYPE_COUNT 129

struct st_PACKER_READ_DATA
{
    PKRAssetType* types;
    void* userdata;
    U32 opts;
    U32 pkgver;
    S32 cltver;
    S32 subver;
    S32 compatver;
    st_HIPLOADDATA* pkg;
    U32 base_sector;
    S32 lockid;
    char packfile[128];
    S32 asscnt;
    S32 laycnt;
    st_XORDEREDARRAY asstoc;
    st_XORDEREDARRAY laytoc;
    st_PACKER_ATOC_NODE* pool_anode;
    S32 pool_nextaidx;
    st_XORDEREDARRAY typelist[PACKER_TYPE_COUNT];
    time_t time_made;
    time_t time_mod;
};

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
S32 PKRLoadStep(S32 block);
void PKR_drv_guardLayer(st_PACKER_LTOC_NODE*);
en_READ_ASYNC_STATUS PKR_drv_guardVerify(st_PACKER_LTOC_NODE*);
inline en_pkrfindmyass PKRFindMyAsset(st_PACKER_READ_DATA*, PKRAssetTOCInfo*, const void*)
{
    return PKR_FIND_FAILED;
}

#endif
