#ifndef XSTRANSVC_H
#define XSTRANSVC_H

#include "xRegionSupport.h"
#include "xpkrsvc.h"

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
char* xSTAssetName(void* raw_HIP_asset);
char* xSTAssetNameFromPointer(void* model);
U32 xSTGetAssetID(void* raw_HIP_asset);
en_pkrfindmyass xSTRandMemToAsset(PKRAssetTOCInfo* ainfo, const void* addr);
void* xSTFindAsset(U32 aid, U32* size);
st_PACKER_ATOC_NODE* xSTFindAssetNode(U32 aid, U32* size);
S32 xSTAssetCountByType(U32 type);
void* xSTFindAssetByType(U32 type, S32 idx, U32* size);
S32 xSTGetAssetInfo(U32 aid, PKRAssetTOCInfo* tocainfo);
S32 xSTGetAssetInfoByType(U32 type, S32 idx, PKRAssetTOCInfo* tocainfo);
S32 xSTGetAssetInfoInHxP(U32 aid, PKRAssetTOCInfo* tocainfo, U32 hash);
char* xST_xAssetID_HIPFullPath(U32 aid);
char* xST_xAssetID_HIPFullPath(U32 aid, U32* sceneID);
U32 xST_getTimeStamp(U32 sceneID);
#if DEBUG
void xSTReportLoadTimes(U32 sceneID, S32 toScreen);
#endif
void xSTSetLocalizationCode(const char* code);
char* xSTGetLocalizationCode();
xRegion xSTGetLocalizationEnum();
void xSTAddToDiscLanguageConfig(xRegion region);
U32 xSTGetDiscLanguageConfig();
S32 xSTGetAssetTypes(U32* types, S32 max_types);

#endif
