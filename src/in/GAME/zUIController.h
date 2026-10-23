#ifndef ZUICONTROLLER_H
#define ZUICONTROLLER_H

#include "zUI.h"

struct zUIControllerAsset : zUIAsset
{
};

class zUIController : public zUI
{
public:
    zUIControllerAsset* GetAsset() const
    {
        return (zUIControllerAsset*)zUI::GetAsset();
    }

    U32 GetSortKey() const
    {
        return GetAsset()->id;
    }

    void DoInit();
    void DoRender() const;
};

void zUIController_Init(xBase& data, xDynAsset& asset, size_t);
void zUIController_Init(zUIController* ui, zUIControllerAsset* asset);

#endif