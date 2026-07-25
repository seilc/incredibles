#ifndef XHUD_H
#define XHUD_H

#include "xMath3.h"
#include "xDynAsset.h"
#include "xColor.h"
#include "xModel.h"

namespace xhud
{
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

    struct asset : xDynAsset
    {
        xVec3 loc;
        xVec3 size;
    };

    struct motive
    {
        union
        {
            F32* value;
            xColor* value_color;
        };
        union
        {
            F32 delta;
            F32 on_time;
        };
        union
        {
            F32 max_offset;
            F32 total_time;
            F32 begin_scale;
        };
        union
        {
            F32 accel;
            F32 fade_time;
            F32 final_scale;
        };
        union
        {
            void* context;
            xColor on_color;
            F32 begin_alpha;
        };
        union
        {
            F32 period;
            xColor off_color;
            F32 end_alpha;
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
        };
        U8 (*fp_update)(widget&, motive&, F32);
        U8 (*fp_finish)(widget&, motive&);
        union
        {
            F32 start_delta;
            F32 orig_height;
        };
        U8 hold;
        U8 inverse;
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

        static void init_base(xBase& data, const xBaseAsset& asset, ulong32 chunk_size);

        void enable();
        void disable();
        void destruct();
        void updater(F32 dt);
        void show(move_direction dir, bool clearmotives, bool bSkipShowMotive);
        void hide(move_direction d, bool clearmotives, bool bSkipHideMotive);
        void clear_motives();
        bool active();
        bool visible() const;

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
} // namespace xhud

#endif