#ifndef ZBULLETMARKASSET_H
#define ZBULLETMARKASSET_H

#include "xDynAsset.h"

struct zBulletMarkAsset : xDynAsset
{
    U32 textureID;
    F32 size;
    F32 lifetime;
};

class zBulletMarkAssetMgr
{
public:
    void init();
    zBulletMarkAsset* get(U32 id);
    void add(zBulletMarkAsset* asset);

private:
    U32 size;
    zBulletMarkAsset data[32];
};

extern zBulletMarkAssetMgr bulletmark_mgr;

void zBulletMarkAsset_Init(xBase& data, xDynAsset& asset, ulong32);

#endif