#ifndef ZUITEXT_H
#define ZUITEXT_H

#include "zUI.h"
#include "xColor.h"

struct zUITextAsset : zUIAsset
{
    U32 textID;
    U8 font;
    U8 fontSizeW;
    U8 fontSizeH;
    U8 fontSpacingX;
    U8 fontSpacingY;
    U8 textBoxInsetTop;
    U8 textBoxInsetLeft;
    U8 textBoxInsetRight;
    U8 textBoxInsetBottom;
    U8 justifyX;
    U8 justifyY;
    U8 textFlags;
    xColor shadowColor;
    F32 shadowOffsetX;
    F32 shadowOffsetY;
    F32 textScaleX;
    F32 textScaleY;
};

class zUIText : public zUI
{
public:
    zUIText();

    void SetText(const char* t);

    const char* GetText() const
    {
        return text;
    }

    virtual U32 GetSortKey() const;
    virtual bool IsUIText();
    virtual void DoInit();
    virtual void DoReset();
    virtual void DoHandleEvent(xBase* from, U32 toEvent, const F32* toParam,
                               xBase* toParamWidget, U32 toParamWidgetID);
    virtual void DoRender() const;
    virtual void DoApplyMotionFrame(const zUIMotionFrame* frame);

private:
    xColor shadowColor;
    const char* text;
};

void zUIText_Init(xBase& data, xDynAsset& asset, size_t);

#endif