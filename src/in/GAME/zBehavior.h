#ifndef ZBEHAVIOR_H
#define ZBEHAVIOR_H

#include "xEnt.h"
#include "zCombat.h"
#include "xJSP.h"

enum GrabType
{
    eGrabTypeMrIStanding,
    eGrabTypeMrIGroundToss,
    eGrabTypeElastigirl,
    eGrabTypeCount
};

struct SphereCollisionResults
{
    xVec3 tested_dir;
    F32 dist;
    S32 collisionFlags;
    xVec3 contact;
    xVec3 normal;
    F32 colliding_sphere_raidus;
    U32 oid;
    void* optr;
    xModelInstance* mptr;
    xJSPHeader* jsp;
    xClumpCollBSPTriangle* bspTriangle;
    xCollideSphereHitType hit_triangle_type;
    xPlane tri_plane;
    S32 sphere_index;
};

// clang-format off
class behavior
{
private:
    xEnt* owner;

public:
    behavior() {}

    virtual bool in_state(const char* state, F32 look_ahead, xModelInstance* model) const;
    virtual const char* getName();
    virtual void set_owner(xEnt* new_owner) { owner = new_owner; }
    virtual bool system_event(xBase* from, xBase* to, U32 to_event, const F32* to_param,
                              xBase* to_param_widget, U32 to_param_widget_id) { return false; }
    virtual bool damage(zCombatDamageInfo& damageInfo) { return false; }
    virtual void enter_state(const behavior* prev_behavior) {}
    virtual void add_states(xAnimTable* table) {}
    virtual void add_transitions(xAnimTable* table) {}
    virtual void exit_state() {}
    virtual void kill() {}
    virtual void update(F32 dt) {}
    virtual void setup() {}
    virtual void exit() {}
    virtual void init() {}
    virtual void scene_setup() {}
    virtual void reset() {}
    virtual void render() {}
    virtual void render_extra() {}
    virtual bool exclusive() { return true; }
    virtual bool runnable(F32 dt) = 0;
    virtual void before_anim_matrices(xAnimPlay* play, xQuat* quat, xVec3* tran,
                                      S32 boneCount) {}
    virtual bool grabbable(GrabType grabType) { return false; }
    virtual bool punchable() { return true; }
    virtual bool collision_response(const xVec3& current_dir,
                                    const SphereCollisionResults& scene_collide,
                                    xVec3& response_dpos, bool collide_on) { return true; }
    virtual void register_collision(const SphereCollisionResults& scene_collide) {}
    virtual F32 delay() { return 0.0f; }
	virtual void set_state(const char* new_state, F32 blend, xModelInstance* model);
    virtual void set_state(const char* new_state, F32 blend, F32 dest_time, xModelInstance* model);

    U32 type;
};
// clang-format on

struct behavior_node
{
    bool prev_ran;
    S8 priority;
    behavior_node* next;
    behavior_node* iteration_next;
    behavior* data;
    F32 delay;
};

class behavior_manager
{
private:
    behavior_node* behavior_list;
    behavior_node* removed_list;
    behavior* current_behavior;
    bool manual_update;
    bool killed;

public:
    behavior_manager()
    {
        behavior_list = NULL;
        current_behavior = NULL;
        manual_update = false;
    }

    virtual void select_current(F32 dt);
    virtual void update(F32 dt);
    virtual void init();
    virtual void setup();
    virtual void exit();
    virtual void scene_setup();
    virtual void add_states(xAnimTable* table);
    virtual void add_transitions(xAnimTable* table);
    virtual void reset();
    virtual void render();
    virtual void render_extra();
    virtual void before_anim_matrices(xAnimPlay* play, xQuat* quat, xVec3* tran, S32 boneCount);
    virtual bool system_event(xBase* from, xBase* to, U32 to_event, const F32* to_param,
                              xBase* to_param_widget, U32 to_param_widget_id);
    virtual void damage(zCombatDamageInfo& damageInfo);
    virtual void kill();
    virtual bool collision_response(const xVec3& current_dir,
                                    const SphereCollisionResults& scene_collide,
                                    xVec3& response_dpos);
    virtual void register_collision(const SphereCollisionResults& scene_collide);

    void set_iteration_pointers();
    void add_behavior(behavior* new_behavior, S8 priority);
    void set_behavior(behavior* new_behavior);
    behavior* get_behavior(U32 type);
    void reset_behavior(behavior* new_behavior);
    void remove_behavior(behavior* which);
    void exit_states();

    behavior* get_current_behavior()
    {
        return current_behavior;
    }
};

U32 ErrorCB(xAnimTransition*, xAnimSingle*, void*);

#endif