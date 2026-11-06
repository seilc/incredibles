#ifndef XCAM_H
#define XCAM_H

#include "xMath2.h"
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

    bool started() const { return flags & 0x1; }

    static void scene_enter();
    static void scene_exit();

    void refresh_mat();
    void debug_init() {}

    static void coord_to_local(xVec3& c, const xVec3& loc);
    static void coord_to_world(xVec3& loc, const xVec3& c);
    static void coord_to_world(xVec3& loc, const xCamCoordCylinder& c);
    static void coord_to_world(xVec3& loc, const xCamCoordSphere& c);
    static void orient_to_local(xQuat& o, const xMat3x3& mat);
    static void orient_to_local(xCamOrientEuler& o, const xMat3x3& mat); // UNUSED
    static void orient_to_world(xMat3x3& mat, const xQuat& o);
    static void orient_to_world(xMat3x3& mat, const xCamOrientEuler& o);
    static xVec3& convert(xVec3& o, const xCamCoordCylinder& i);
    static xVec3& convert(xVec3& o, const xCamCoordSphere& i);
    static xVec3& convert(xVec3& o, const xCamCoordPolar& i);
    static xCamCoordCylinder& convert(xCamCoordCylinder& o, const xCamCoordSphere& i);
    static xCamCoordSphere& convert(xCamCoordSphere& o, const xCamCoordCylinder& i);
    static xQuat& convert(xQuat& o, const xCamOrientEuler& i);
    static xCamOrientEuler& convert(xCamOrientEuler& o, const xQuat& i);
    static void blend(F32& o, F32 a, F32 b, F32 s);
    static void blend_radian(F32& o, F32 a, F32 b, F32 s);
    static void blend(xVec3& o, const xVec3& a, const xVec3& b, F32 s);
    static void blend(xCamCoordCylinder& o, const xCamCoordCylinder& a, const xCamCoordCylinder& b, F32 s);
    static void blend(xCamCoordSphere& o, const xCamCoordSphere& a, const xCamCoordSphere& b, F32 s);
    static void blend(xQuat& o, const xQuat& a, const xQuat& b, F32 s);
    static void blend(xCamOrientEuler& o, const xCamOrientEuler& a, const xCamOrientEuler& b, F32 s);
    static void blend3(F32& o, F32 i0, F32 i1, F32 i2, F32 s);
    static void blend3(xVec3& o, const xVec3& i0, const xVec3& i1, const xVec3& i2, F32 s);
    static void blend3(xCamConfigFollow::zone_data& o, const xCamConfigFollow::zone_data& i0,
                       const xCamConfigFollow::zone_data& i1, const xCamConfigFollow::zone_data& i2, F32 s);
    static void spring_interp(F32& s, F32& v, F32 s0, F32 s1, F32 dt, F32 speed, F32 e);
    static void spring_interp(xVec3& s, F32& v, const xVec3& s0, const xVec3& s1, F32 dt, F32 speed, F32 e);
    static void spring_interp(xQuat& s, F32& v, const xQuat& s0, const xQuat& s1, F32 dt, F32 speed, F32 e);
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
};

inline void xCam::coord_to_local(xVec3& c, const xVec3& loc)
{
    c = loc;
}

inline void xCam::coord_to_world(xVec3& loc, const xVec3& c)
{
    loc = c;
}

inline void xCam::coord_to_world(xVec3& loc, const xCamCoordCylinder& c)
{
    convert(loc, c);
}

inline void xCam::coord_to_world(xVec3& loc, const xCamCoordSphere& c)
{
    convert(loc, c);
}

inline void xCam::orient_to_local(xQuat& o, const xMat3x3& mat)
{
    xQuatFromMat(&o, &mat);
}

// UNUSED: ported from Rat GC
inline void xCam::orient_to_local(xCamOrientEuler& o, const xMat3x3& mat)
{
    xMat3x3GetEuler(&mat, (xVec3*)&o);
}

inline void xCam::orient_to_world(xMat3x3& mat, const xQuat& o)
{
    xQuatToMat(&o, &mat);
}

inline void xCam::orient_to_world(xMat3x3& mat, const xCamOrientEuler& o)
{
    xMat3x3Euler(&mat, o.yaw, o.pitch, o.roll);
}

inline xVec3& xCam::convert(xVec3& o, const xCamCoordCylinder& i)
{
    o.x = i.origin.x + i.dist * isin(i.theta);
    o.y = i.origin.y + i.height;
    o.z = i.origin.z + i.dist * icos(i.theta);
    return o;
}

inline xVec3& xCam::convert(xVec3& o, const xCamCoordSphere& i)
{
    xVec3 dir;
    xQuatToLookVec(&i.dir, &dir);
    xVec3AddScaleMacro(&o, &i.origin, &dir, i.dist);
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

inline xCamCoordCylinder& xCam::convert(xCamCoordCylinder& o, const xCamCoordSphere& i)
{
    o.origin = i.origin;
    xVec3 offset;
    xQuatToLookVec(&i.dir, &offset);
    offset *= i.dist;
    o.dist = xsqrt(offset.x * offset.x + offset.z * offset.z);
    o.height = offset.y;
    o.theta = xatan2(offset.x, offset.z);
    return o;
}

inline xCamCoordSphere& xCam::convert(xCamCoordSphere& o, const xCamCoordCylinder& i)
{
    o.origin = i.origin;
    xMat3x3 mat;
    xVec3 offset = { i.dist * -isin(i.theta), -i.height, i.dist * -icos(i.theta) };
    o.dist = xMat3x3LookVec(&mat, &offset);
    xQuatFromMat(&o.dir, &mat);
    return o;
}

inline xQuat& xCam::convert(xQuat& o, const xCamOrientEuler& i)
{
    xMat3x3 mat;
    xMat3x3Euler(&mat, (const xVec3*)&i);
    xQuatFromMat(&o, &mat);
    return o;
}

inline xCamOrientEuler& xCam::convert(xCamOrientEuler& o, const xQuat& i)
{
    xMat3x3 mat;
    xQuatToMat(&i, &mat);
    xMat3x3GetEuler(&mat, (xVec3*)&o);
    return o;
}

inline void xCam::blend(F32& o, F32 a, F32 b, F32 s)
{
    o = a * (1.0f - s) + b * s;
}

inline void xCam::blend_radian(F32& o, F32 a, F32 b, F32 s)
{
    F32 diff = xrmod(PI + (b - a)) - PI;
    o = xrmod(a + diff * s);
}

inline void xCam::blend(xVec3& o, const xVec3& a, const xVec3& b, F32 s)
{
    blend(o.x, a.x, b.x, s);
    blend(o.y, a.y, b.y, s);
    blend(o.z, a.z, b.z, s);
}

inline void xCam::blend(xCamCoordCylinder& o, const xCamCoordCylinder& a, const xCamCoordCylinder& b, F32 s)
{
    blend(o.origin, a.origin, b.origin, s);
    blend(o.dist, a.dist, b.dist, s);
    blend(o.height, a.height, b.height, s);
    blend_radian(o.theta, a.theta, b.theta, s);
}

inline void xCam::blend(xCamCoordSphere& o, const xCamCoordSphere& a, const xCamCoordSphere& b, F32 s)
{
    blend(o.origin, a.origin, b.origin, s);
    blend(o.dist, a.dist, b.dist, s);
    xQuatSlerp(&o.dir, &a.dir, &b.dir, s);
}

inline void xCam::blend(xQuat& o, const xQuat& a, const xQuat& b, F32 s)
{
    xQuatSlerp(&o, &a, &b, s);
}

inline void xCam::blend(xCamOrientEuler& o, const xCamOrientEuler& a, const xCamOrientEuler& b, F32 s)
{
    blend_radian(o.yaw, a.yaw, b.yaw, s);
    blend_radian(o.pitch, a.pitch, b.pitch, s);
    blend_radian(o.roll, a.roll, b.roll, s);
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

inline void xCam::spring_interp(xVec3& s, F32& v, const xVec3& s0, const xVec3& s1, F32 dt, F32 speed, F32 e)
{
    F32 dx = s0.x - s1.x, dy = s0.y - s1.y, dz = s0.z - s1.z;
    F32 sdist = xsqrt(dx * dx + dy * dy + dz * dz);
    if (sdist < 0.0001f) { s = s1; v = 0.0f; return; }
    F32 dist = sdist;
    xDampSpring(dist, v, dt, speed, e);
    if (v > 0.0f) v = 0.0f;
    if (dist <= 0.0f) { s = s1; return; }

    F32 u = dist / sdist;
    s.x = u * dx + s1.x;
    s.y = u * dy + s1.y;
    s.z = u * dz + s1.z;
}

inline void xCam::spring_interp(xQuat& s, F32& v, const xQuat& s0, const xQuat& s1, F32 dt, F32 speed, F32 e)
{
    F32 sa = xabs(xrmod(xQuatGetAngleDiff(&s0, &s1) + PI) - PI);
    if (sa < 0.0001f) { s = s1; v = 0.0f; return; }
    F32 a = sa;
    xDampSpring(a, v, dt, speed, e);
    if (v > 0.0f) v = 0.0f;
    if (a <= 0.0f) { s = s1; return; }
    xQuatSlerp(&s, &s1, &s0, a / sa);
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

    virtual void create();
    virtual void destroy();
    virtual void start();
    virtual void stop();
    virtual void update(xScene& scene, F32 dt);
    virtual xCam* get_next();

    void attach(xCam& src, xCam& dst);
    void reverse();
    void blend_coord(F32 s);
    void blend_orient(F32 s);

    xVec3& convert(const xCam& cam, xVec3& c) const;
    xCamCoordCylinder& convert(const xCam& cam, xCamCoordCylinder& c) const;
    xCamCoordSphere& convert(const xCam& cam, xCamCoordSphere& c) const;
    xQuat& convert(const xCam& cam, xQuat& c) const;
    xCamOrientEuler& convert(const xCam& cam, xCamOrientEuler& c) const;
};

struct xCamDebug : xCam
{
    struct config : xCamConfigFollow
    {
        xVec2 stick_vel;
        xVec3 move_vel;
        F32 slow_turn;
        F32 slow_move;
    };

    virtual void create(); // UNUSED
    virtual void start(); // UNUSED
    virtual void update(xScene&, F32 dt); // UNUSED

    config cfg;
    xPad::analog_data* analog_move;
    xPad::analog_data* analog_turn;
    xVec2 dpad;
    F32 slow_input;
};

inline xVec3& xCamBlend::convert(const xCam& cam, xVec3& c) const
{
    switch (cam.coord_type) {
    case XCAM_COORD_CART:
        return (xVec3&)cam.coord.cart;
    case XCAM_COORD_CYLINDER:
        return xCam::convert(c, cam.coord.cylinder);
    case XCAM_COORD_SPHERE:
        return xCam::convert(c, cam.coord.sphere);
    default:
        return c;
    }
}

inline xCamCoordCylinder& xCamBlend::convert(const xCam& cam, xCamCoordCylinder& c) const
{
    switch (cam.coord_type) {
    case XCAM_COORD_CYLINDER:
        return (xCamCoordCylinder&)cam.coord.cylinder;
    case XCAM_COORD_SPHERE:
        return xCam::convert(c, cam.coord.sphere);
    default:
        return c;
    }
}

inline xCamCoordSphere& xCamBlend::convert(const xCam& cam, xCamCoordSphere& c) const
{
    switch (cam.coord_type) {
    case XCAM_COORD_CYLINDER:
        return xCam::convert(c, cam.coord.cylinder);
    case XCAM_COORD_SPHERE:
        return (xCamCoordSphere&)cam.coord.sphere;
    default:
        return c;
    }
}

inline xQuat& xCamBlend::convert(const xCam& cam, xQuat& c) const
{
    switch (cam.orient_type) {
    case XCAM_ORIENT_QUAT:
        return (xQuat&)cam.orient.quat;
    case XCAM_ORIENT_EULER:
        return xCam::convert(c, cam.orient.euler);
    default:
        return c;
    }
}

inline xCamOrientEuler& xCamBlend::convert(const xCam& cam, xCamOrientEuler& c) const
{
    switch (cam.orient_type) {
    case XCAM_ORIENT_QUAT:
        return xCam::convert(c, cam.orient.quat);
    case XCAM_ORIENT_EULER:
        return (xCamOrientEuler&)cam.orient.euler;
    default:
        return c;
    }
}

static const S32 MAX_OWNED_CAMS = 32;

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
    xCam* owned[MAX_OWNED_CAMS];
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
    xCamBlend* grab_blend_cam();
    S32 get_next() const;
    S32 get_blend(S32 to_index);
    void set_primary(S32 index);
    void flush_dead();
    void notify_released(xCam& cam);
    void set_child_flags(S32 add, S32 remove);
    void get_view_window(xVec2& vw); // UNUSED
    xCamBlend* create_blend();

    S32 insert(xCam& cam)
    {
        cam.group = this;
        cam.group_index = size;
        owned[size] = &cam;
        size++;
        return size - 1;
    }

    void erase(xCam& cam)
    {
        erase(cam.group_index);
    }

    void erase(S32 index)
    {
        size--;
        xCam& c = *owned[index];
        c.group = NULL;
        c.group_index = -1;
        owned[index] = owned[size];
        owned[index]->group_index = index;
    }
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

    static void create_frustum_planes(xVec4* frust_planes, const xMat4x3& ltm, const xVec2& view_offset, const xVec2& view_window, F32 near_plane, F32 far_plane); // UNUSED
};

#endif