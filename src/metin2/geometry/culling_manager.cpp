#include "culling_manager.h"
#include <metin2/graphics/grp_object_instance.h>

namespace metin2::inline geometry {

    void culling_manager::ray_trace_callback(const vector3d & /*p1*/,
        const vector3d & /*dir*/,
        float distance,
        const vector3d & /*sect*/,
        sphere_pack *sphere) {
        if (ray_far_distance_ <= 0.0f || ray_far_distance_ >= distance) {
            list_.push_back((graphic_object_instance *)sphere->user_data());
        }
    }

    void culling_manager::visibility_callback(const frustum & /*f*/, sphere_pack *sphere, view_state state) {

        auto *instance = (graphic_object_instance *)sphere->user_data();
        if (state == VS_OUTSIDE) {
            instance->hide();
        } else {
            instance->show();
        }
    }

    void culling_manager::range_test_callback(const vector3d & /*p*/, float /*distance*/, sphere_pack *sphere, view_state state) {
        if (state != VS_OUTSIDE) {
            list_.push_back((graphic_object_instance *)sphere->user_data());
        }
    }

    void culling_manager::reset() {
        factory_->reset();
    }

    void culling_manager::update() {
        // Objects move their own spheres; this only rebalances the tree.
        factory_->process();
    }

    void culling_manager::process() {
        update_view_matrix();
        update_proj_matrix();
        build_view_frustum();
        factory_->frustum_test(get_frustum(), this);
    }

    auto culling_manager::register_value(graphic_object_instance *obj) -> culling_manager::culling_handle {
        assert(obj);
        vector3d center;
        float radius;
        obj->bounding_sphere(center, radius);
        return factory_->add_sphere(center, radius, obj);
    }

    void culling_manager::unregister(culling_handle h) {
        factory_->remove(h);
    }

    culling_manager::culling_manager() {
        factory_ = std::make_unique<sphere_pack_factory>(
            10000, // maximum count
            6400,  // root radius
            1600,  // leaf radius
            400    // extra radius
        );
    }

    culling_manager::~culling_manager() = default;
}
