#ifndef XSCRIPTASSET_H
#define XSCRIPTASSET_H

#include "xBaseAsset.h"

struct xScriptAsset : xBaseAsset
{
    F32 scaleFactor;
    U32 eventCount;
    bool loop;
    U8 pad[3];
};

struct xScriptEventAsset
{
    F32 time;
    U32 widget;
    U32 paramEvent;
    F32 param[4];
    U32 paramWidget;
};

#endif
