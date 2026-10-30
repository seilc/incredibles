#ifndef XFONT_H
#define XFONT_H

#include "xColor.h"
#include "xCommon.h"
#include "xMath2.h"
#include "xString.h"

static const basic_rect<F32> screen_bounds = { 0, 0, 1, 1 };

class xfont
{
public:
    U32 id;
    F32 width;
    F32 height;
    F32 space;
    xColor color;
    xColor shadowColor;
    F32 shadowOffsetX;
    F32 shadowOffsetY;
    basic_rect<F32> clip;

    static xfont create(
        U32 id,
        F32 width,
        F32 height,
        F32 space,
        xColor color,
        const basic_rect<F32>& clip = screen_bounds,
        xColor shadowColor = g_BLACK,
        F32 shadowOffsetX = 2.0f,
        F32 shadowOffsetY = 2.0f)
    {
        xfont r;
        r.id = id;
        r.width = width;
        r.height = height;
        r.space = space;
        r.color = color;
        r.clip = clip;
        r.shadowColor = shadowColor;
        r.shadowOffsetX = shadowOffsetX;
        r.shadowOffsetY = shadowOffsetY;
        return r;
    }

    void start_render() const;
    void stop_render() const;
    void irender(const char* text, F32 x, F32 y) const;

    void render(const char* text, F32 x, F32 y) const
    {
        start_render();
        irender(text, x, y);
        stop_render();
    }
};

class xtextbox
{
public:
    struct jot;
    class layout;

    struct split_tag
    {
        substr tag;
        substr name;
        substr action;
        substr value;
    };

    struct tag_type
    {
        substr name;
        void(*parse_tag)(jot&, xtextbox&, xtextbox&, split_tag&);
        void(*reset_tag)(jot&, xtextbox&, xtextbox&, split_tag&);
        void* context;
    };

    struct callback
    {
        void(*render)(jot&, xtextbox&, F32, F32);
        void(*layout_update)(jot&, xtextbox&, xtextbox&);
        void(*render_update)(jot&, xtextbox&, xtextbox&);
    };

    struct jot
    {
        substr s;
        struct
        {
            bool invisible : 1;
            bool ethereal : 1;
            bool merge : 1;
            bool word_break : 1;
            bool word_end : 1;
            bool line_break : 1;
            bool stop : 1;
            bool tab : 1;
            bool insert : 1;
            bool dynamic : 1;
            bool page_break : 1;
            bool stateful : 1;
            bool japanese_break : 1;
            U16 dummy : 3;
        } flag;
        U16 context_size;
        void* context;
        basic_rect<F32> bounds;
        basic_rect<F32> render_bounds;
        const callback* cb;
        const tag_type* tag;

        void reset_flags() { *(U16*)&flag = 0; }
    };

    static callback text_cb;

    xfont font;
    basic_rect<F32> bounds;
    U32 flags;
    F32 line_space;
    F32 tab_stop;
    F32 left_indent;
    F32 right_indent;
    const callback* cb;
    void* context;

    void set_text(const char* text);
    layout& temp_layout(bool cache) const;
    void render(layout& l, S32 begin_jot, S32 end_jot) const;
    F32 yextent(F32 max, S32& size, const layout& l, S32 begin_jot, S32 end_jot) const;

    F32 yextent(bool cache) const { return yextent(temp_layout(cache), 0, -1); }
    F32 yextent(const layout& l, S32 begin_jot, S32 end_jot) const { S32 size; return yextent(HUGE, size, l, begin_jot, end_jot); }

    static xtextbox create();

    static xtextbox create(
        const xfont& font,
        const basic_rect<F32>& bounds = screen_bounds,
        U32 flags = 0x2,
        F32 line_space = 0.0f,
        F32 tab_stop = 0.0f,
        F32 left_indent = 0.0f,
        F32 right_indent = 0.0f)
    {
        xtextbox r;
        r.font = font;
        r.bounds = bounds;
        r.flags = flags;
        r.line_space = line_space;
        r.tab_stop = tab_stop;
        r.left_indent = left_indent;
        r.right_indent = right_indent;
        r.texts_size = 0;
        r.text_hash = 0;
        r.cb = &text_cb;
        return r;
    }

    void render(bool cache) const
    {
        render(temp_layout(cache), 0, -1);
    }

private:
    const char** texts;
    const size_t* text_sizes;
    size_t texts_size;
    substr text;
    U32 text_hash;
};

void xFontPrintTopText();
void render_fill_rect(const basic_rect<F32>& bounds, xColor color);

inline F32 NSCREENX(F32 v)
{
    return 1.0f / SCREEN_WIDTH * v;
}

inline F32 NSCREENY(F32 v)
{
    return 1.0f / SCREEN_HEIGHT * v;
}

#endif
