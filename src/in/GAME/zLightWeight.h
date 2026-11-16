#ifndef ZLIGHTWEIGHT_H
#define ZLIGHTWEIGHT_H

#include "types.h"

namespace z
{
    class lightweight_system_base
    {
    public:
        virtual void scene_setup() = 0;
        virtual void reset() = 0;
        virtual void update(F32 dt) = 0;
        virtual void render() const = 0;
        virtual void post_render_2d() const = 0;
        virtual void post_render_3d() const = 0;
        virtual void exit() = 0;
    };

    namespace lightweight_manager
    {
        extern S32 system_count;
        extern lightweight_system_base* system[10];

        void setup();
        void exit();
        void reset();
        void update(F32 dt);
        void render();
        void post_render_2d();
        void post_render_3d();
        void scene_setup();
        void register_lightweight(lightweight_system_base* new_system);
    } // namespace lightweight_manager

    template <class T, S32 N>
    class lightweight
    {
        bool used;
        U32 id;

    public:
        lightweight(bool used)
        {
            this->used = used;
        }

        void set_used(bool value)
        {
            used = value;
        }

        static void add(const T& item);

        virtual void render() const {}
        virtual void post_render_2d() const {}
        virtual void post_render_3d() const {}
        virtual void update(F32) {}
        virtual void destroy() {}
        virtual void reset() {}
    };
} // namespace z

#endif