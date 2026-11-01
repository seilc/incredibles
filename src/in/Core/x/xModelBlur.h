#ifndef XMODELBLUR_H
#define XMODELBLUR_H

#include "types.h"

struct xVec3;
struct xFRect;
struct xModelInstance;

class xModelBlur
{
public:
    enum { MAX_ACTIVE = 6 };

    struct config_data
    {
        S32 flags;
        F32 radius;
        F32 color_red;
        F32 color_green;
        F32 color_blue;
        F32 color_alpha;
        F32 card_dist;
        F32 zbias;
        S32 max_card_renders;
    };

    static void scene_enter();
    static void scene_exit();
    static void reset_all();
    static void update_all(F32 dt);
    static void render_all();
    static config_data& get_default_config();

    void create();
    bool activate(F32 lifetime, F32 alpha, const config_data& cfg, xModelInstance* model);
    void deactivate();
    void move(const xVec3& center);
    void set_config(const config_data& cfg);
    bool visible() const;
    void set_lifetime(F32 value, F32 time);
    F32 get_lifetime_goal() const;
    void set_alpha(F32 value, F32 time);
    F32 get_alpha_goal() const;
    bool update(F32 dt);
    void render();

private:
    struct activity_data;

    void render_cards(const xFRect& src_rect, F32 xoff0, F32 yoff0, F32 xoff1, F32 yoff1);

    static activity_data* alloc_activity();
    static void free_activity(activity_data* activity);

    static activity_data* activity_buffer;
    static activity_data* activities[MAX_ACTIVE];
    static S32 activities_used;

    activity_data* activity;
};

#endif
