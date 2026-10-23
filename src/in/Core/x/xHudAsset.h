#ifndef XHUDASSET_H
#define XHUDASSET_H

#include "xHud.h"
#include "xDynAsset.h"
#include "xLinkAsset.h"

namespace xhud {

struct asset : xDynAsset
{
    xVec3 loc;
    xVec3 size;
};

struct model_asset : asset
{
    enum { VERSION = 1 };

    U32 model;

    static const char* type_name() { return "hud:model"; }
};

struct text_asset : asset
{
    enum { VERSION = 1 };
    
    U32 text_box;
    U32 text;

    static const char* type_name() { return "hud:text"; }
};

struct meter_asset : asset
{
    F32 start_value;
    F32 min_value;
    F32 max_value;
    F32 increment_time;
    F32 decrement_time;
    struct
    {
        U32 start_increment;
        U32 increment;
        U32 start_decrement;
        U32 decrement;
    } sound;

    static const char* type_name() { return "hud:meter"; }
};

struct font_meter_asset : meter_asset
{
    enum { VERSION = 2 };

    font_context font;

    static const char* type_name() { return "hud:meter:font"; }
};

struct model_info
{
    U32 id;
    xVec3 loc;
    xVec3 size;
};

struct unit_meter_asset : meter_asset
{
    enum { VERSION = 3 };

    model_info model[2];
    xVec3 offset;
    U32 fill_forward;

    static const char* type_name() { return "hud:meter:unit"; }
};

}

#endif
