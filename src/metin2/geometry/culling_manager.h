#pragma once

#include <metin2/graphics/grp_screen.h>

#include "sphere_pack.h"

#include <memory>

namespace metin2::inline graphics {
    class graphic_object_instance;
}

namespace metin2::inline geometry {

    template <class T>
    struct range_tester : public sphere_pack_callback {
        T *f;
        float dist;
        range_tester(T *fn, float distance = -1)
            : f(fn), dist(distance) {}

        ~range_tester() override = default;

        void ray_trace_callback(const vector3d &p1,
            const vector3d &dir,
            float distance,
            const vector3d &sect,
            sphere_pack *sphere) override {
            if (dist <= 0.0f || dist >= distance)
                (*f)((graphic_object_instance *)sphere->user_data());
        };

        void visibility_callback(const frustum &f_value, sphere_pack *sphere, view_state state) override {};

        void range_test_callback(const vector3d &p, float distance, sphere_pack *sphere, view_state state) override {
            if (state != VS_OUTSIDE)
                (*f)((graphic_object_instance *)sphere->user_data());
        }

        void point_test2d_callback(const vector3d &p, sphere_pack *sphere, view_state state) override {
            if (state != VS_OUTSIDE) {
                (*f)((graphic_object_instance *)sphere->user_data());
            }
        }
    };

    class culling_manager : public sphere_pack_callback, private screen {
      public:
        using culling_handle = sphere_pack *;
        using range_list = std::vector<graphic_object_instance *>;

        culling_manager();
        ~culling_manager() override;

        void ray_trace_callback(const vector3d &p1,
            const vector3d &dir,
            float distance,
            const vector3d &sect,
            sphere_pack *sphere) override;

        void visibility_callback(const frustum &f, sphere_pack *sphere, view_state state) override;

        void range_test_callback(const vector3d &p, float distance, sphere_pack *sphere, view_state state) override;

        void reset();
        void update();
        void process();

        void range_test(const vector3d &p, float radius, sphere_pack_callback *callback) {
            factory_->range_test(p, radius, callback);
        }

        void point_test2d(const vector3d &p, sphere_pack_callback *callback) {
            factory_->point_test2d(p, callback);
        }

        template <class T>
        void for_in_range2d(const vector3d &p, T *func) {
            range_tester<T> r(func);
            factory_->point_test2d(p, &r);
        }

        template <class T>
        void for_in_range(const vector3d &p, float radius, T *func) {
            range_tester<T> r(func);
            factory_->range_test(p, radius, &r);
        }

        template <class T>
        void for_in_ray(const vector3d &p1, const vector3d &dir, T *func) {
            range_tester<T> r(func);
            factory_->ray_trace(p1, dir, &r);
        }

        auto register_value(graphic_object_instance *ob) -> culling_handle;
        void unregister(culling_handle h);

        auto begin() -> range_list::iterator { return list_.begin(); }
        auto end() -> range_list::iterator { return list_.end(); }

      protected:
        range_list list_;

        float ray_far_distance_;

        std::unique_ptr<sphere_pack_factory> factory_;
    };

    inline auto get_culling_manager() -> culling_manager & {
        static culling_manager instance;
        return instance;
    }
}
