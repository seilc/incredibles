#ifndef ZTEXTBOX_H
#define ZTEXTBOX_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xFont.h"

struct ztextbox : xBase
{
    struct asset_type : xDynAsset
    {
        struct color_type
        {
            U8 r, g, b, a;
        };

        U32 text;
        basic_rect<F32> bounds;
        bool yAsBottom;
        bool autoCenterOnScreen;
        U32 font;
        struct
        {
            F32 width;
            F32 height;
        } size;
        struct
        {
            F32 x;
            F32 y;
        } space;
        color_type color;
        struct
        {
            F32 left;
            F32 top;
            F32 right;
            F32 bottom;
        } inset;
        enum
        {
            XJ_LEFT,
            XJ_CENTER,
            XJ_RIGHT
        } xjustify;
        enum
        {
            YJ_TOP,
            YJ_CENTER,
            YJ_BOTTOM
        } yjustify;
        enum
        {
            EX_UP,
            EX_CENTER,
            EX_DOWN,
            MAX_EX
        } expand;
        F32 max_height;
        struct
        {
            U32 type;
            color_type color;
            U32 texture;
            F32 borderU;
            F32 borderV;
            F32 borderWidth;
            F32 borderHeight;
        } backdrop;
        color_type shadowColor;
        F32 shadowOffsetX;
        F32 shadowOffsetY;
    };

    struct
    {
        bool active : 1;
        bool dirty : 1;
        bool show_backdrop : 1;
    } flag;
    const asset_type* asset;
    xtextbox tb;
    char* segments[16];
    size_t segments_size;
    ztextbox* next;
    ztextbox* prev;
    RwRaster* bgtex;

    void refresh();
    void get_text(char* buffer, size_t buffer_size) const;
};

#endif
