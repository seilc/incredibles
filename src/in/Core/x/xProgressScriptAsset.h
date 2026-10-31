#ifndef XPROGRESSSCRIPTASSET_H
#define XPROGRESSSCRIPTASSET_H

#include "xBaseAsset.h"

struct xProgressScriptAsset : xBaseAsset
{
    U32 eventCount;
};

struct xProgressScriptEventAsset
{
    F32 percent;
    S32 flags;
    U32 widget;
    U32 paramEvent;
    F32 param[4];
    U32 paramWidget;
};

#endif
