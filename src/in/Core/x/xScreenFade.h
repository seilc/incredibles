#ifndef XSCREENFADE_H
#define XSCREENFADE_H

#include "xDynAsset.h"
#include "xColor.h"

enum eScreenFadeState
{
    eScreenFadeStateOff,
    eScreenFadeStateFadeDown,
    eScreenFadeStateWait,
    eScreenFadeStateFadeUp,
    eScreenFadeStateMax
};

struct xScreenFade : xBase
{
    struct asset_type : xDynAsset
    {
        xColor dest;
        F32 fadeDownTime;
        F32 waitTime;
        F32 fadeUpTime;
    };

    asset_type* asset;
    eScreenFadeState state;
    xColor src;
    xColor current_color;
    F32 time_passed;
    xBase* owner;

    void load(asset_type& a);
    void StartFade();
    void reset();

    static void load(xBase& data, xDynAsset& asset, size_t);
    static void cb_dispatch(xBase* from, xBase* to, U32 event, const F32*, xBase*, U32);
};

#endif