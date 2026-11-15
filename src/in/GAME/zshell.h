#ifndef ZSHELL_H
#define ZSHELL_H

#include "xVec3.h"
#include "zLightWeight.h"

struct RpAtomic;
struct xEnt;
struct xMat4x3;
struct xModelInstance;
struct zShrapnelAsset;

namespace z
{
    RpAtomic* find_model(const char* model_name, U32& modelID);
    xModelInstance* get_model(const char* model_name);
    void free_model(xModelInstance* model);

    struct shell_type
    {
        const char* shell_model_name;
        zShrapnelAsset* shrapnel;
        xVec3 gravity_direction;
        F32 blast_radius;
        S32 damage;
        F32 away_knockback;
        F32 up_knockback;
        F32 max_dist;

        static shell_type* create(const char* model_name);
    };

    class shell : public lightweight<shell, 15>
    {
    public:
        xEnt* owner;
        F32 distance;
        F32 gravity_speed;
        xVec3 position;
        xVec3 velocity;
        xModelInstance* model;
        shell_type* type;

        static void setup();
        void destroy();
        void update(F32 dt);
        void set_mat(xMat4x3* mat) const;
        void render() const;
        shell(const xVec3& position, const xVec3& velocity, xEnt* owner, F32 gravity_speed, shell_type* type);
    };
} // namespace z

#endif
