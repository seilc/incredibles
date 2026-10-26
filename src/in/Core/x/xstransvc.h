#ifndef XSTRANSVC_H
#define XSTRANSVC_H

#include "xRegionSupport.h"
#include "xpkrsvc.h"

#include <stdio.h>

bool xSTIsLoaded();
S32 xSTStartup(PKRAssetType* handlers);
S32 xSTShutdown();
S32 xSTPreLoadScene(U32 sid, void* userdata, S32 flg_hiphop, S32 fileflags, const char* filename);
S32 xSTQueueSceneAssets(U32 sid, S32 flg_hiphop);
void xSTUnLoadScene(U32 sid, S32 flg_hiphop);
void xSTUnLoadSceneMany(U32 sceneID, S32 type);
F32 xSTLoadStep(U32);
void xSTDisconnect(U32 sid, S32 flg_hiphop);
S32 xSTSwitchScene(U32 sid, void* userdata, S32(*progmon)(void*, F32));
char* xSTAssetName(U32 aid);
char* xSTAssetName(void* raw_HIP_asset); // inferred from BFBB GC, TSSM GC, Rat GC
en_pkrfindmyass xSTRandMemToAsset(PKRAssetTOCInfo* ainfo, const void* addr);
void* xSTFindAsset(U32 aid, U32* size);
S32 xSTAssetCountByType(U32 type);
void* xSTFindAssetByType(U32 type, S32 idx, U32* size);
S32 xSTGetAssetInfo(U32 aid, PKRAssetTOCInfo* tocainfo);
S32 xSTGetAssetInfoByType(U32 type, S32 idx, PKRAssetTOCInfo* tocainfo);
S32 xSTGetAssetInfoInHxP(U32 aid, PKRAssetTOCInfo* tocainfo, U32 hash); // inferred from BFBB GC, Rat MAP
char* xST_xAssetID_HIPFullPath(U32 aid);
char* xST_xAssetID_HIPFullPath(U32 aid, U32* sceneID);
void xSTSetLocalizationCode(const char* code);
char* xSTGetLocalizationCode();
xRegion xSTGetLocalizationEnum();

inline const char* xSTAssetNameOrID(U32 aid)
{
    const char* name = xSTAssetName(aid);
    if (name) {
        return name;
    }

    static char buffer[16];
    sprintf(buffer, "ID:0x%x", aid);
    return buffer;
}

inline const char* xSTAssetNameOrID(void* raw_HIP_asset)
{
    const char* name = xSTAssetName(raw_HIP_asset);
    if (name) {
        return name;
    }

    static char buffer[16];
    sprintf(buffer, "ADDR:0x%x", (U32)raw_HIP_asset);
    return buffer;
}

#endif
