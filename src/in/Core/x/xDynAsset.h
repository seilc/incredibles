#ifndef XDYNASSET_H
#define XDYNASSET_H

#include "xBase.h"
#include "xstransvc.h"
#include "xString.h"

struct xDynAsset : xBaseAsset
{
    U32 type;
    U16 version;
    U16 handle;
};

#define xDynAssetValidate_L(line, asset, T, type_name)                                            \
do {                                                                                              \
    xASSERT_FMT_L((line), (asset).version == T::VERSION,                                           \
               "%s %s - Wrong version: %d (expected %d).  Get latest and repack.",                \
               type_name,                                                                         \
               xSTAssetName(&(asset)),                                                            \
               (asset).version,                                                                   \
               T::VERSION);                                                                       \
    size_t expected_asset_size = (asset).linkCount * sizeof(xLinkAsset) + sizeof(T);              \
    xASSERT_FMT_L((line), (asset_size) == expected_asset_size,                                     \
               "%s %s - Wrong asset size: %d (expected %d).  Get latest and repack.",             \
               type_name,                                                                         \
               xSTAssetName(&(asset)),                                                            \
               asset_size,                                                                        \
               expected_asset_size);                                                              \
    xASSERT_L((line), (asset).type == xStrHash(type_name));                                       \
} while (0)

#define xDynAssetValidate(asset, T, type_name) xDynAssetValidate_L(__LINE__, asset, T, type_name)

#endif