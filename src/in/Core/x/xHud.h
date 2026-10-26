#ifndef XHUD_H
#define XHUD_H

#include "xMath3.h"
#include "xDynAsset.h"
#include "xColor.h"
#include "xModel.h"

struct tweak_info;

namespace xhud
{
    struct render_context;
    struct font_context;
    struct motive;
    struct motive_node;
    struct asset;
    class widget;

    struct render_context
    {
        xVec3 loc;
        xVec3 size;
        xVec3 rot;
        F32 r;
        F32 g;
        F32 b;
        F32 a;
    };

    struct font_context
    {
        U32 id;
        S32 justify;
        F32 w;
        F32 h;
        F32 space;
        F32 drop_x;
        F32 drop_y;
        xColor c;
        xColor drop_c;
    };

    struct motive
    {
        union
        {
            F32* value;
            F32 time_to_blink;
        };
        union
        {
            F32 delta;
            F32 start_blink_period;
        };
        union
        {
            F32 max_offset;
            F32 begin_scale;
            F32 end_blink_period;
        };
        union
        {
            F32 accel;
            F32 final_scale;
            F32 fade_blink_time;
        };
        union
        {
            void* context;
            F32 begin_alpha;
            xColor on_blink_color;
        };
        union
        {
            F32 period;
            F32 end_alpha;
            xColor off_blink_color;
        };
        union
        {
            F32 elapsed;
            F32 orig_width;
        };
        union
        {
            F32 offset;
            F32 current_scale;
            F32 current_blink_time;
        };
        bool(*fp_update)(widget&, motive&, F32);
        bool(*fp_finish)(widget&, motive&);
        union
        {
            F32 start_delta;
            F32 orig_height;
            F32 period_blink_time;
        };
        bool hold;
        bool inverse;
    };

    struct motive_node
    {
        motive m;
        motive_node* next;
    };

    class widget
    {
    public:
        enum move_direction
        {
            LEFT,
            RIGHT,
            TOP,
            BOTTOM
        };

        struct
        {
            bool visible;
            bool enabled;
            bool active;
            F32 timer;
        } flag;
        render_context rc;
        render_context start_rc;
        asset* a;
        enum
        {
            ACT_NONE,
            ACT_SHOW,
            ACT_HIDE,
            MAX_ACT
        } activity;

        widget(const asset& a);

        // RESTORED: from Rat MAP
        ~widget() {}
        // END RESTORED

        static void init_base(xBase& data, const xBaseAsset& asset, size_t chunk_size);

        void enable();
        void disable();
        void destruct();
        void presetup();
        void updater(F32 dt);
        void show(move_direction dir, bool clearmotives, bool bSkipShowMotive);
        void hide(move_direction d, bool clearmotives, bool bSkipHideMotive);
        void clear_motives();
        bool active();
        bool enabled() const { return flag.enabled; }
        bool visible() const { return flag.visible && enabled(); }

        virtual void destroy();
        virtual U32 type() const;
        virtual bool is(U32 typeID) const;
        virtual void init();
        virtual void setup();
        virtual void update(F32 dt);
        virtual void render();
        virtual void dispatch(xBase*, U32 event, const F32*, xBase*, U32);

    private:
        motive_node* _motive_top;
        motive_node* _motive_temp;
        motive_node** _motive_temp_tail;
    };

    struct widget_chunk : xBase
    {
        widget w;
    };

    void init();
    void destroy();
    void update(F32 dt);
    void render();
    xModelInstance* load_model(U32 id);
    void render_model(xModelInstance& m, const render_context& rc);

#if DEBUG || RELEASE
    void screenToNormalizedX(const tweak_info&, void* value);
    void screenToNormalizedY(const tweak_info&, void* value);
    void normalizedToScreenX(const tweak_info&, void* value);
    void normalizedToScreenY(const tweak_info&, void* value);
#endif

    inline void widget::disable() { flag.enabled = false; }
} // namespace xhud

#endif