#ifndef XCAM_H
#define XCAM_H

#include "xMath3.h"
#include "xPad.h"

struct xCamGroup;
struct xCamBlend;
struct xScene;

enum xCamCoordType
{
    XCAM_COORD_INVALID = -1,
    XCAM_COORD_CART = 0,
    XCAM_COORD_CYLINDER,
    XCAM_COORD_SPHERE,
    XCAM_COORD_MAX
};

enum xCamOrientType
{
    XCAM_ORIENT_INVALID = -1,
    XCAM_ORIENT_QUAT = 0,
    XCAM_ORIENT_EULER,
    XCAM_ORIENT_MAX
};

struct xCamCoordCylinder
{
    xVec3 origin;
    F32 dist;
    F32 height;
    F32 theta;
};

struct xCamCoordSphere
{
    xVec3 origin;
    F32 dist;
    xQuat dir;
};

struct xCamOrientEuler
{
    F32 yaw;
    F32 pitch;
    F32 roll;
};

struct xCamConfigCommon
{
    U8 priority;
    U8 pad1;
    U8 pad2;
    U8 pad3;
    F32 blend_time;
};

struct xCamConfigFollow
{
    struct zone_data
    {
        xVec3 offset;
        xVec3 face;
    };

    zone_data zone_rest;
    zone_data zone_above;
    zone_data zone_below;
    F32 speed_zone_offset;
    F32 speed_zone_face;
    F32 speed_move_orbit;
};

class xCam
{
public:
    xMat4x3 mat;
    F32 fov;
    S32 flags;
    U32 owner;
    xCamGroup* group;
    xPad::analog_data analog;
    F32 motion_factor;
    xCamCoordType coord_type;
    xCamOrientType orient_type;
    union
    {
        xVec3 cart;
        xCamCoordCylinder cylinder;
        xCamCoordSphere sphere;
    } coord;
    union
    {
        xQuat quat;
        xCamOrientEuler euler;
    } orient;
    xCamConfigCommon cfg_common;

    xCam()
    {
    }

    static void scene_enter();
    static void scene_exit();

    virtual void create();
    virtual void destroy();
    virtual void start();
    virtual void stop();
    virtual void update(xScene& scene, F32 dt) = 0;
    virtual xCam* get_next();
    virtual xCamConfigFollow* config_follow();

    void add_tweaks(const char*);

    S32 group_index;
    S32 group_flags;
    xCamBlend* blender;
    U8 unk0xBC[4];
};

class xCamBlend : public xCam
{
public:
    xCam* src;
    xCam* dst;
    F32 time;
};

class xCamGroup
{
public:
    xMat4x3 mat;
    xVec3 vel;
    F32 fov;
    F32 fov_default;
    S32 flags;
    xCam* primary;
    xPad::analog_data analog;
    xCam* owned[32];
    S32 size;
    S32 primary_index;
    S32 child_flags;
    S32 child_flags_mask;
    xCamBlend* blend_cam[4];

    void create();
    void destroy();
    void reset();
    void update(xScene& scene, F32 dt);
    void add(xCam& cam, bool force_cut);
    void remove(xCam& cam, bool force_cut);
};

class xCamScreen
{
public:
    RwCamera* icam;
    F32 fov;

    void create(S32 w, S32 h);
    void destroy();
    void set_scene(xScene& scene);
    void prepare_render();
    void begin_render(bool clear);
    void end_render();
    void set_world_matrix(const xMat4x3& mat);
    void set_fov(F32 fov);
};

#endif