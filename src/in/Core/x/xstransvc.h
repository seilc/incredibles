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
en_pkrfindmyass xSTRandMemToAsset(PKRAssetTOCInfo* ainfo, const void* addr);
void* xSTFindAsset(U32 aid, U32* size);
S32 xSTAssetCountByType(U32 type);
void* xSTFindAssetByType(U32 type, S32 idx, U32* size);
S32 xSTGetAssetInfo(U32 aid, PKRAssetTOCInfo* tocainfo);
S32 xSTGetAssetInfoByType(U32 type, S32 idx, PKRAssetTOCInfo* tocainfo);
char* xST_xAssetID_HIPFullPath(U32 aid);
char* xST_xAssetID_HIPFullPath(U32 aid, U32* sceneID);
void xSTSetLocalizationCode(const char* code);
char* xSTGetLocalizationCode();
xRegion xSTGetLocalizationEnum();

#endif
