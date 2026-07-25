#ifndef ZUIUSERSTRING_H
#define ZUIUSERSTRING_H

#include "zUIText.h"

struct zUIUserStringAsset : zUITextAsset
{
    U8 hardMaxChars;
    U8 softMaxChars;
};

class zUIUserString : public zUIText
{
public:
    zUIUserString() : zUIText()
    {
    }

    zUIUserStringAsset* GetAsset() const
    {
        return (zUIUserStringAsset*)zUI::GetAsset();
    }

    virtual U32 GetSortKey() const;
    virtual void DoInit();
    virtual void DoReset();
    virtual void DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam,
                               xBase* toParamWidget, U32 toParamWidgetID);

private:
    char* textBuffer;
    S32 length;
    S32 maxLength;
};

void zUIUserString_Init(xBase& data, xDynAsset& asset, ulong32);
void zUIUserString_Init(zUIUserString* ui, zUIUserStringAsset* asset);

#endif