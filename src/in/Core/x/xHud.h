#ifndef XHUD_H
#define XHUD_H

#include "xBase.h"
#include "xDynAsset.h"
#include "xModel.h"
#include "xColor.h"

#include <stddef.h>

namespace xhud {

struct render_context;
struct font_context;
struct motive;
struct motive_node;
struct asset;
class widget;

struct holder;

class block_allocator
{
public:
    void create(S32 block_size, S32 max_blocks);
    void* alloc();
    void free(void* mem);
    void reset();

private:
    S32 block_size;
    S32 max_blocks;
    holder* head;
    void* buffer;
};

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

typedef bool(*motive_update_callback)(widget&, motive&, F32);
typedef bool(*motive_finish_callback)(widget&, motive&);

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
    motive_update_callback fp_update;
    motive_finish_callback fp_finish;
    union
    {
        F32 start_delta;
        F32 orig_height;
        F32 period_blink_time;
    };
    bool hold;
    bool inverse;

    motive(F32* value, F32 delta, F32 max_offset, F32 accel, motive_update_callback fp_update, void* context,
           F32 _period, bool _hold, bool _inverse, motive_finish_callback finish)
        : value(value)
        , delta(delta)
        , max_offset(max_offset)
        , accel(accel)
        , context(context)
        , period(_period)
        , elapsed(0.0f)
        , offset(0.0f)
        , fp_update(fp_update)
        , fp_finish(finish)
        , start_delta(delta)
        , hold(_hold)
        , inverse(_inverse)
    {
    }

    motive(const motive& c)
        : value(c.value)
        , delta(c.delta)
        , max_offset(c.max_offset)
        , accel(c.accel)
        , context(c.context)
        , period(c.period)
        , elapsed(c.elapsed)
        , offset(c.offset)
        , fp_update(c.fp_update)
        , fp_finish(c.fp_finish)
        , start_delta(c.start_delta)
        , hold(c.hold)
        , inverse(c.inverse)
    {
    }

    bool update(widget& w, F32 dt)
    {
        return fp_update(w, *this, dt);
    }

    void finish(widget& w)
    {
        bool finish = true;
        if (fp_finish) {
            finish = fp_finish(w, *this);
        }
        if (finish) {
            if (value) {
                *value += max_offset - offset;
            }
            offset = max_offset;
        }
    }

    bool active() { return elapsed > 0.0f; }

    bool update_time(F32 dt)
    {
        if (!period) {
            return true;
        }
        elapsed += dt;
        if (elapsed > period) {
            elapsed = 0.0f;
            return false;
        }
        return true;
    }
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
    const asset* a;
    enum
    {
        ACT_NONE,
        ACT_SHOW,
        ACT_HIDE,
        MAX_ACT
    } activity;

    static block_allocator motive_allocator;

    widget(const asset& a);

    // RESTORED: from Rat MAP
    ~widget() {}
    // END RESTORED

    static void init_base(xBase& data, const xBaseAsset& asset, size_t chunk_size);
    static void scene_enter();
    static void disable_all(bool destroy_widgets);
    static void setup_all();
    static void update_all(F32 dt);
    static void render_all();
    static void cb_dispatch(xBase* from, xBase* to, U32 event, const F32* argf, xBase* argw, U32 argwi);

    void enable();
    void disable();
    void set_timer(F32 value);
    F32 get_timer() const;
    void destruct();
    void presetup();
    void updater(F32 dt);
    void dispatcher(xBase*, U32 event, const F32*, xBase*, U32);
    void show(move_direction dir, bool clearmotives, bool bSkipShowMotive);
    void hide(move_direction d, bool clearmotives, bool bSkipHideMotive);
    void add_motive(const motive& m);
    void clear_motives();
    void clear_motives(motive_update_callback fp_update, void* context);
    bool active();
    bool enabled() const { return flag.enabled; }
    bool visible() const { return flag.visible && enabled(); }

    virtual void destroy() {}
    virtual U32 type() const;
    virtual bool is(U32 id) const;
    virtual void init() {}
    virtual void setup() { presetup(); }
    virtual void update(F32 dt) { updater(dt); }
    virtual void render() {}
    virtual void dispatch(xBase* from, U32 event, const F32* argf, xBase* argw, U32 argwi) { dispatcher(from, event, argf, argw, argwi); }

    void add_tweaks() {}

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
bool linear_motive_update(widget& w, motive& m, F32 dt);
bool accelerate_motive_update(widget&, motive& m, F32 dt);
void top_hide_widget(widget& w, motive&);
void bottom_hide_widget(widget& w, motive&);
void left_hide_widget(widget& w, motive&);
void right_hide_widget(widget& w, motive&);
bool delay_motive_update(widget& w, motive& m, F32 dt);

inline F32 widget::get_timer() const { return flag.timer; }
inline void widget::set_timer(F32 value) { flag.timer = value; }
inline void widget::enable() { flag.enabled = true; }
inline void widget::disable() { flag.enabled = false; }

}

#endif
