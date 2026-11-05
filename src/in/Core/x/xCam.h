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

struct xCamCoordPolar
{
    xVec3 origin;
    F32 theta;
    F32 phi;
    F32 dist;
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
    static void coord_to_world(xVec3& loc, const xCamCoordCylinder& c);
    static void coord_to_local(xVec3& c, const xVec3& loc);
    static void orient_to_local(xQuat& o, const xMat3x3& mat);
    static xVec3& convert(xVec3& o, const xCamCoordCylinder& i);
    static xVec3& convert(xVec3& o, const xCamCoordPolar& i);
    static void blend3(F32& o, F32 i0, F32 i1, F32 i2, F32 s);
    static void blend3(xVec3& o, const xVec3& i0, const xVec3& i1, const xVec3& i2, F32 s);
    static void blend3(xCamConfigFollow::zone_data& o, const xCamConfigFollow::zone_data& i0,
                       const xCamConfigFollow::zone_data& i1, const xCamConfigFollow::zone_data& i2, F32 s);
    static void spring_interp(F32& s, F32& v, F32 s0, F32 s1, F32 dt, F32 speed, F32 e);
    template <class T, class U>
    static void spring_interp(T& s, U& v, const T& s0, const T& s1, F32 dt, F32 speed);

    virtual void create();
    virtual void destroy() {}
    virtual void start();
    virtual void stop();
    virtual void update(xScene& scene, F32 dt) = 0;
    virtual xCam* get_next() { return NULL; }
    virtual xCamConfigFollow* config_follow() { return NULL; }

    void add_tweaks(const char*);

    S32 group_index;
    S32 group_flags;
    xCamBlend* blender;
    U8 unk0xBC[4];
};

inline void xCam::coord_to_world(xVec3& loc, const xCamCoordCylinder& c)
{
    convert(loc, c);
}

inline void xCam::coord_to_local(xVec3& c, const xVec3& loc)
{
    c = loc;
}

inline void xCam::orient_to_local(xQuat& o, const xMat3x3& mat)
{
    xQuatFromMat(&o, &mat);
}

inline xVec3& xCam::convert(xVec3& o, const xCamCoordCylinder& i)
{
    o.x = i.origin.x + i.dist * isin(i.theta);
    o.y = i.origin.y + i.height;
    o.z = i.origin.z + i.dist * icos(i.theta);
    return o;
}

inline xVec3& xCam::convert(xVec3& o, const xCamCoordPolar& i)
{
    F32 cp = i.dist * icos(i.phi);
    o.x = i.origin.x + cp * isin(i.theta);
    o.y = i.origin.y + i.dist * isin(i.phi);
    o.z = i.origin.z + cp * icos(i.theta);
    return o;
}

inline void xCam::blend3(F32& o, F32 i0, F32 i1, F32 i2, F32 s)
{
    o = i1 + s * (s >= 0.0f ? i2 - i1 : i1 - i0);
}

inline void xCam::blend3(xVec3& o, const xVec3& i0, const xVec3& i1, const xVec3& i2, F32 s)
{
    blend3(o.x, i0.x, i1.x, i2.x, s);
    blend3(o.y, i0.y, i1.y, i2.y, s);
    blend3(o.z, i0.z, i1.z, i2.z, s);
}

inline void xCam::blend3(xCamConfigFollow::zone_data& o, const xCamConfigFollow::zone_data& i0,
                         const xCamConfigFollow::zone_data& i1, const xCamConfigFollow::zone_data& i2, F32 s)
{
    blend3(o.offset, i0.offset, i1.offset, i2.offset, s);
    blend3(o.face, i0.face, i1.face, i2.face, s);
}

inline void xCam::spring_interp(F32& s, F32& v, F32 s0, F32 s1, F32 dt, F32 speed, F32 e)
{
    F32 ds = s0 - s1;
    xDampSpring(ds, v, dt, speed, e);
    s = s1 + ds;
}

template <class T, class U>
void xCam::spring_interp(T& s, U& v, const T& s0, const T& s1, F32 dt, F32 speed)
{
    F32 e = xDampSpringCoeff(dt, speed);
    spring_interp(s, v, s0, s1, dt, speed, e);
}

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