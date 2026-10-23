#ifndef XSTRANSVC_H
#define XSTRANSVC_H

#include "xpkrsvc.h"
#include "xRegionSupport.h"

typedef struct st_STRAN_SCENE
{
    U32 scnid;
    S32 lockid;
    PACKER_READ_DATA* spkg;
    S32 flgHipHop;
    void* userdata;
    char fnam[256];
} STRAN_SCENE;

#define STRAN_SCENE_COUNT 16

typedef struct st_STRAN_DATA
{
    STRAN_SCENE hipscn[STRAN_SCENE_COUNT];
    U32 loadlock;
} STRAN_DATA;

#define XST_HIP 0x1
#define XST_HOP 0x2
#define XST_HIPHOP (XST_HIP | XST_HOP)

#define XSTUNLOAD_HIP 0x1
#define XSTUNLOAD_HOP 0x2
#define XSTUNLOAD_HIPHOP 0x4
#define XSTUNLOAD_ALL (XSTUNLOAD_HIP | XSTUNLOAD_HOP | XSTUNLOAD_HIPHOP)

bool xSTIsLoaded();
S32 xSTStartup(PKRAssetType* handlers);
S32 xSTShutdown();
void xSTUnLoadSceneMany(U32 sceneID, S32 type);
const char* xSTAssetName(U32 aid);
void* xSTFindAsset(U32 aid, U32* size = NULL);
S32 xSTAssetCountByType(U32 type);
void* xSTFindAssetByType(U32 type, S32 idx, U32* size = NULL);
S32 xSTGetAssetInfo(U32 aid, PKRAssetTOCInfo* tocainfo);
S32 xSTGetAssetInfoByType(U32 type, S32 idx, PKRAssetTOCInfo* tocainfo);
const char* xST_xAssetID_HIPFullPath(U32 aid);
const char* xST_xAssetID_HIPFullPath(U32 aid, U32* sceneID);
void xSTSetLocalizationCode(const char* code);
const char* xSTGetLocalizationCode();
xRegion xSTGetLocalizationEnum();

#endif