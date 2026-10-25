#ifndef XLIGHTVOLUME_H
#define XLIGHTVOLUME_H

#include "xColor.h"
#include "xModel.h"
#include "xDebug.h"

#include <rpworld.h>

class xLightVolume
{
public:
    static void scene_enter();
    static void scene_exit();
    static void reset_all();

    void create();
    bool activate();
    void deactivate();

    virtual void render() = 0;
    virtual void render_immediate_pass(xColor, bool, bool)
    {
        xASSERT(72, false && "Must implement for immediate-mode.");
    }

    static void render_all();
    void render_atomic(RpAtomic*, xColor, bool, U32);

private:
    struct activity_type;

    activity_type* activity;

    static activity_type* activities;
    static S32 activities_used;
};

class xLightVolumeModel : public xLightVolume
{
public:
    void create();
    virtual void render();
    void render_single(xModelInstance* model, U32 stencil_pass);

    xModelInstance* model;
    xColor color;
    bool inside;
    bool single;
};

#endif
