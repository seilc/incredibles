#include "xCam.h"

#include "iCamera.h"
#include "xDebugTweak.h"
#include "xMemMgr.h"
#include "xScene.h"
#include "xutil.h"

namespace {
    namespace tweak {
        void add_tweaks(const char* prefix)
        {
        }
    }
}

void xCam::start()
{
    flags |= 0x1;
}

void xCam::stop()
{
    flags &= ~0x1;
}

void xCam::scene_enter()
{
    tweak::add_tweaks("Camera|\001Globals|");
}

void xCam::scene_exit()
{
    xTWEAKREMOVE("Camera");
}

void xCam::refresh_mat()
{
    switch (coord_type) {
    case XCAM_COORD_CART:
        coord_to_world(mat.pos, coord.cart);
        break;
    case XCAM_COORD_CYLINDER:
        coord_to_world(mat.pos, coord.cylinder);
        break;
    case XCAM_COORD_SPHERE:
        coord_to_world(mat.pos, coord.sphere);
        break;
    }

    switch (orient_type) {
    case XCAM_ORIENT_QUAT:
        orient_to_world(mat, orient.quat);
        break;
    case XCAM_ORIENT_EULER:
        orient_to_world(mat, orient.euler);
        break;
    }
}

void xCam::create()
{
    flags = 0;
    coord_type = XCAM_COORD_INVALID;
    orient_type = XCAM_ORIENT_INVALID;
    group = NULL;
    debug_init();
}

void xCamGroup::create()
{
    size = 0;
    for (xCamBlend** it = blend_cam, **end = it + ARRAY_LENGTH(blend_cam); it != end; it++)
        *it = create_blend();
}

void xCamGroup::destroy()
{
    for (xCamBlend** it = blend_cam, **end = it + ARRAY_LENGTH(blend_cam); it != end; it++)
        (*it)->destroy();
}

void xCamGroup::reset()
{
    for (xCam** it = owned, **end = it + size; it != end; it++) {
        xCam& c = **it;
        c.group = NULL;
        if (c.started())
            c.stop();
    }

    size = 0;
    primary = NULL;
    flags = 0;
    fov = fov_default;
    set_child_flags(0, 0);
}

void xCamGroup::update(xScene& scene, F32 dt)
{
    xVec3 prev_pos = mat.pos;

    S32 next_index = get_next();
    next_index = get_blend(next_index);
    set_primary(next_index);

    if (primary) {
        primary->flags &= child_flags_mask;
        primary->flags |= child_flags;
        primary->analog = analog;
        primary->update(scene, dt);

        mat = primary->mat;
        fov = primary->fov;

        if (primary->flags & 0x4) {
            xCam* next = primary->get_next();
            if (next != primary) {
                primary->stop();
                erase(*primary);
                if (!next) {
                    primary = NULL;
                } else {
                    if (next->group_index == -1)
                        insert(*next);
                    primary = next;
                    primary->group = this;
                    primary->group_flags &= ~0x10;
                }
            }
        }

        vel = (mat.pos - prev_pos) / dt;

        flush_dead();
    }
}

void xCamGroup::add(xCam& cam, bool force_cut)
{
    if (cam.group == this) {
        cam.group_flags &= ~0xC;
        if (force_cut)
            cam.group_flags |= 0x8;
    } else if (!cam.group && size < MAX_OWNED_CAMS) {
        cam.group_flags = 0;
        if (force_cut)
            cam.group_flags |= 0x8;
        insert(cam);
    }
}

void xCamGroup::remove(xCam& cam, bool force_cut)
{
    if (cam.group == this) {
        if (&cam == primary || (cam.group_flags & 0x10)) {
            cam.group_flags &= ~0x8;
            cam.group_flags |= 0x4;
            if (force_cut)
                cam.group_flags |= 0x8;
        } else {
            erase(cam);
        }
    }
}

xCamBlend* xCamGroup::grab_blend_cam()
{
    for (xCamBlend** it = blend_cam, **end = it + ARRAY_LENGTH(blend_cam); it != end; it++) {
        if (!(*it)->group)
            return *it;
    }
    return NULL;
}

S32 xCamGroup::get_next() const
{
    S32 next, min_priority;

    if (!primary || (primary->group_flags & 0x4)) {
        min_priority = 0;
        next = -1;
    } else {
        min_priority = primary->cfg_common.priority + 1;
        next = primary->group_index;
    }

    for (S32 i = 0; i < size; i++) {
        if (!(owned[i]->group_flags & 0x4)) {
            S32 priority = owned[i]->cfg_common.priority;
            if (priority >= min_priority) {
                next = i;
                min_priority = priority + 1;
            }
        }
    }

    return next;
}

S32 xCamGroup::get_blend(S32 to_index)
{
    if (!primary || to_index == -1 || to_index == primary->group_index)
        return to_index;

    xCam* to = owned[to_index];
    xCam* from = primary->group_flags & 0x4 ? primary : to;
    F32 blend_time = from->group_flags & 0x8 ? 0.0f : from->cfg_common.blend_time;
    if (blend_time <= 0.0f)
        return to_index;

    if (to->group_flags & 0x10) {
        xCamBlend* parent = to->blender;
        while (parent) {
            if (parent->dst != to)
                parent->reverse();
            to = parent;
            parent = parent->blender;
        }
        return primary->group_index;
    }

    if (size >= MAX_OWNED_CAMS)
        return to_index;

    xCamBlend* blend = grab_blend_cam();
    if (!blend)
        return to_index;

    primary->blender = to->blender = blend;
    blend->attach(*primary, *to);
    blend->cfg_common.blend_time = blend_time;
    primary->group_flags |= 0x10;
    to->group_flags |= 0x10;
    blend->group_flags = 0x1;
    return insert(*blend);
}

void xCamGroup::set_primary(S32 index)
{
    if (primary && index == primary->group_index)
        return;

    if (primary && primary->started() && !(primary->group_flags & 0x10)) {
        primary->stop();
        mat = primary->mat;
    }

    if (index == -1) {
        primary = NULL;
        return;
    }

    primary = owned[index];
    primary->group = this;
    primary->mat = mat;
    primary->fov = fov_default;
    primary->start();
}

void xCamGroup::flush_dead()
{
    for (S32 i = 0; i < size; i++) {
        xCam& c = *owned[i];
        if (&c != primary && !(c.group_flags & 0x10) && (c.group_flags & 0x5)) {
            if (c.started())
                c.stop();
            erase(i);
            i--;
        }
    }
}

void xCamGroup::notify_released(xCam& cam)
{
    cam.group_flags &= ~0x10;
    cam.blender = NULL;
}

void xCamGroup::set_child_flags(S32 add, S32 remove)
{
    child_flags_mask = ~remove;
    child_flags = add & child_flags_mask;
}

// RESTORED: speculative
void xCamGroup::get_view_window(xVec2& vw)
{
    F32 t = itan(0.5f * fov);
    vw.x = t;
    vw.y = 0.75f * t;
}
// END RESTORED

xCamBlend* xCamGroup::create_blend()
{
    xCamBlend* cam = new (eMemStaticType_0, 0) xCamBlend();
    if (!cam) return NULL;
    cam->create();
    cam->owner = 'XCBL';
    cam->cfg_common.priority = 0xff;
    return cam;
}

void xCamScreen::create(S32 w, S32 h)
{
    icam = iCameraCreate(w, h, 1);
}

void xCamScreen::destroy()
{
    iCameraDestroy(icam);
}

void xCamScreen::set_scene(xScene& scene)
{
    iCameraAssignEnv(icam, scene.env->geom);
}

void xCamScreen::prepare_render()
{
    UpdateGlobalFrustumPlanes(icam);
}

void xCamScreen::begin_render(bool clear)
{
    iCameraBegin(icam, clear);
    iCameraUpdateFog(icam, 0);
}

void xCamScreen::end_render()
{
    iCameraEnd(icam);
}

void xCamScreen::set_world_matrix(const xMat4x3& mat)
{
    iCameraUpdatePos(icam, (xMat4x3*)&mat);
}

void xCamScreen::set_fov(F32 fov)
{
    this->fov = fov;
    iCameraSetFOV(icam, (180.0f/PI) * fov);
}

// RESTORED: from Incredibles PS2
struct plane_type
{
    xVec3 norm;
    F32 dist;
};

void xCamScreen::create_frustum_planes(xVec4* frust_planes, const xMat4x3& ltm, const xVec2& view_offset, const xVec2& view_window, F32 near_plane, F32 far_plane)
{
    plane_type* planes = (plane_type*)frust_planes;

    xVec3 temp;
    xVec3 cop = ltm.right * -view_offset.x + ltm.up * view_offset.y;
    F32 scale = 2.0f;

    xVec3 right = ltm.right * view_window.x;
    xVec3 up = ltm.up * view_window.y;

    xVec3 corners[8];
    S32 i;
    corners[0] = ltm.at + right + up;
    corners[1] = corners[0] - right * scale;
    corners[2] = corners[1] - up * scale;
    corners[3] = corners[2] + right * scale;

    for (i = 0; i < 4; i++) {
        xVec3* target = &corners[i];
        temp = *target - cop;
        *target = cop + ltm.pos + temp * near_plane;
        corners[i + 4] = cop + ltm.pos + temp * far_plane;
    }

    planes[0].norm = ltm.at;
    planes[0].dist = planes[0].norm.dot(corners[4]);
    planes[1].norm = -planes[0].norm;
    planes[1].dist = planes[1].norm.dot(corners[0]);

    temp = corners[1] - corners[5];
    planes[2].norm.cross(temp, corners[6] - corners[5]);
    planes[2].norm.normalize();
    planes[2].dist = planes[2].norm.dot(corners[1]);

    planes[3].norm.cross(corners[4] - corners[5], temp);
    planes[3].norm.normalize();
    planes[3].dist = planes[3].norm.dot(corners[1]);

    temp = corners[3] - corners[7];
    planes[4].norm.cross(temp, corners[4] - corners[7]);
    planes[4].norm.normalize();
    planes[4].dist = planes[4].norm.dot(corners[3]);

    planes[5].norm.cross(corners[6] - corners[7], temp);
    planes[5].norm.normalize();
    planes[5].dist = planes[5].norm.dot(corners[3]);
}
// END RESTORED

void xCamBlend::create()
{
    xCam::create();
    flags |= 0x4;
    coord_type = XCAM_COORD_INVALID;
    orient_type = XCAM_ORIENT_INVALID;
}

void xCamBlend::destroy()
{
}

void xCamBlend::start()
{
    static const xCamCoordType coord_table[3][3] = {
        { XCAM_COORD_CART, XCAM_COORD_CART, XCAM_COORD_CART },
        { XCAM_COORD_CART, XCAM_COORD_CYLINDER, XCAM_COORD_SPHERE },
        { XCAM_COORD_CART, XCAM_COORD_SPHERE, XCAM_COORD_SPHERE }
    };

    static const xCamOrientType orient_table[2][2] = {
        { XCAM_ORIENT_QUAT, XCAM_ORIENT_QUAT },
        { XCAM_ORIENT_QUAT, XCAM_ORIENT_EULER }
    };

    if (!src->started()) {
        src->mat = mat;
        src->fov = fov;
        src->start();
    }

    if (!dst->started()) {
        dst->mat = mat;
        dst->fov = fov;
        dst->start();
    }

    cfg_common.priority = MAX(src->cfg_common.priority, dst->cfg_common.priority);

    coord_type = coord_table[src->coord_type][dst->coord_type];
    orient_type = orient_table[src->orient_type][dst->orient_type];

    time = 0.0f;
    xCam::start();
}

void xCamBlend::stop()
{
    src->stop();
    if (time < cfg_common.blend_time)
        dst->stop();

    group->notify_released(*src);
    group->notify_released(*dst);

    xCam::stop();
}

void xCamBlend::update(xScene& scene, F32 dt)
{
    time += dt;

    F32 t = time / cfg_common.blend_time; if (t > 1.0f) t = 1.0f;
    t = xSCurve(t);

    src->analog = dst->analog = analog;
    src->update(scene, dt);
    dst->update(scene, dt);

    blend_coord(t);
    blend_orient(t);
    blend(fov, src->fov, dst->fov, t);

    refresh_mat();
}

xCam* xCamBlend::get_next()
{
    return time >= cfg_common.blend_time ? dst : this;
}

void xCamBlend::attach(xCam& src, xCam& dst)
{
    this->src = &src;
    this->dst = &dst;
}

void xCamBlend::reverse()
{
    xCam* temp = src;
    src = dst;
    dst = temp;
    time = cfg_common.blend_time - time;
}

void xCamBlend::blend_coord(F32 s)
{
    switch (coord_type) {
    case XCAM_COORD_CART: {
        xVec3 c1, c2;
        blend(coord.cart, convert(*src, c1), convert(*dst, c2), s);
        break;
    }
    case XCAM_COORD_CYLINDER: {
        xCamCoordCylinder c1, c2;
        blend(coord.cylinder, convert(*src, c1), convert(*dst, c2), s);
        break;
    }
    case XCAM_COORD_SPHERE: {
        xCamCoordSphere c1, c2;
        blend(coord.sphere, convert(*src, c1), convert(*dst, c2), s);
        break;
    }
    }
}

void xCamBlend::blend_orient(F32 s)
{
    switch (orient_type) {
    case XCAM_ORIENT_QUAT: {
        xQuat c1, c2;
        blend(orient.quat, convert(*src, c1), convert(*dst, c2), s);
        break;
    }
    case XCAM_ORIENT_EULER: {
        xCamOrientEuler c1, c2;
        blend(orient.euler, convert(*src, c1), convert(*dst, c2), s);
        break;
    }
    }
}

// RESTORED: from Rat GC
void xCamDebug::create()
{
    xCam::create();
    coord_type = XCAM_COORD_CART;
    orient_type = XCAM_ORIENT_EULER;
}

void xCamDebug::start()
{
    xCam::start();
    coord_to_local(coord.cart, mat.pos);
    orient_to_local(orient.euler, mat);
    dpad = 0.0f;
    slow_input = 0.0f;
    orient.euler.roll = 0.0f;
}

void xCamDebug::update(xScene&, F32 dt)
{
    F32 turn_mult = 1.0f + (cfg.slow_turn - 1.0f) * slow_input;
    F32 move_mult = 1.0f + (cfg.slow_move - 1.0f) * slow_input;

    F32 dyaw = dt * (turn_mult * (cfg.stick_vel.x * analog_turn->offset.x));
    orient.euler.yaw += dyaw;
    orient.euler.yaw = xrmod(orient.euler.yaw);

    F32 dpitch = dt * (turn_mult * (cfg.stick_vel.y * analog_turn->offset.y));
    orient.euler.pitch += dpitch;
    orient.euler.pitch = range_limit(orient.euler.pitch, -89.0f * (PI/180.0f), 89.0f * (PI/180.0f));

    orient_to_world(mat, orient.euler);

    xVec3 dright = mat.right;
    dright *= dt * (move_mult * (cfg.move_vel.x * analog_move->offset.x));
    coord.cart += dright;

    F32 dy = dt * (move_mult * (-dpad.y * cfg.move_vel.y));
    coord.cart.y += dy;

    xVec3 dat = mat.at;
    dat *= dt * (move_mult * (cfg.move_vel.z * analog_move->offset.y));
    coord.cart += dat;

    coord_to_world(mat.pos, coord.cart);
}
// END RESTORED
