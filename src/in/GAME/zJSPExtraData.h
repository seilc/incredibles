#ifndef ZJSPEXTRADATA_H
#define ZJSPEXTRADATA_H

#include "xDynAsset.h"

struct zJSPExtraData : xBase
{
    struct zJSPExtraDataAsset : xDynAsset
    {
        U32 jspID;
        U32 groupID;
    };

    struct jspGroupPair
    {
        U32 jspID;
        U32 groupID;
        xBase* pGroup;
        xBase* pWidget;
    };

    static jspGroupPair sJspGroupPairs[10];
    static S32 sNumPairs;

    static void ShowAssociatedGroup(U32 jspAssetId, bool show);
    static void AddPair(U32 jspID, U32 groupID, zJSPExtraData* data);
    static void Load(xBase& data, xDynAsset& asset, size_t);

    void LoadPrivate(const zJSPExtraDataAsset& asset);
};

#endif