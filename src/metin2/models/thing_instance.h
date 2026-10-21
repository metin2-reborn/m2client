#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <vector>

#include <metin2/core/stl.h>
#include <metin2/graphics/grp_object_instance.h>

#include "lod_controller.h"

namespace metin2::inline models {

    const int DONTUSEVALUE = -1;
    class graphic_thing_instance : public graphic_object_instance {
      public:
        struct model_thing_set {
            void clear() {
                stl_wipe(lod_thing_ref_vector);
            }

            std::vector<graphic_thing::ref *> lod_thing_ref_vector;
        };

      public:
        enum {
            ID = THING_OBJECT
        };

        auto get_type() const -> uint32_t override { return ID; }

        graphic_thing_instance();

        ~graphic_thing_instance() override;

        void deform_no_skin();

        void update_lod_level();
        void update_time();
        void deform_all();

        auto less_render_order(graphic_thing_instance *thing_instance) -> bool;

        auto picking(const math::vector3 &v, const math::vector3 &dir, float &out_x, float &out_y) -> bool;

        void on_initialize() override;

        auto create_device_objects() -> bool;
        void destroy_device_objects();

        void reserve_model_instance(int count);
        void reserve_model_thing(int count);

        auto check_model_instance_index(int model_instance) -> bool;
        auto check_model_thing_index(int model_thing) -> bool;
        auto check_motion_thing_index(uint32_t motion_key) -> bool;
        auto motion_thing_pointer(uint32_t key, graphic_thing **motion) -> bool;
        auto is_motion_thing() -> bool;

        void register_model_thing(int i_model_thing, graphic_thing *p_model_thing);
        void register_lod_thing(int i_model_thing, graphic_thing *p_model_thing);
        void register_motion_thing(uint32_t motion_key, graphic_thing *motion_thing);

        auto set_model_instance(int dst_model_instance, int src_model_thing, int src_model, int skel_instance = DONTUSEVALUE) -> bool;
        void set_end_stop_motion();
        void set_motion_at_end();

        void attach_model_instance(int dst_model_instance, const char *bone_name, int src_model_instance);
        void attach_model_instance(int dst_model_instance, const char *bone_name, graphic_thing_instance &rsrc_instance, int src_model_instance);
        void detach_model_instance(int dst_model_instance, graphic_thing_instance &src_instance, int src_model_instance);
        auto find_bone_index(int model_instance, const char *bone_name, int *ret_bone) -> bool;

        void reset_local_time();
        void insert_delay(float delay);

        void set_material_image_pointer(uint32_t part, const char *image_name, graphic_image *image);
        void set_material_data(uint32_t part, const char *image_name, material_data material_data);
        void set_specular_info(uint32_t part, const char *mtrl_name, bool enable, float power);

        void set_local_time_impl(float local_time); // Only used by tools.
        auto local_time() -> float;
        auto second_elapsed() -> float;
        auto average_second_elapsed() -> float;

        auto lod_level(uint32_t model_instance) -> uint8_t;
        auto height() -> float;

        void render_with_one_texture();
        void blend_render_with_one_texture();

        auto lod_controller_pointer(uint32_t model_index) const -> lod_controller *;
        auto lod_controller_pointer(uint32_t model_index) -> lod_controller *;

      public:
        auto base_thing_ptr() -> graphic_thing *;

        auto set_motion(uint32_t motion_key, float blend_time = 0.0f, int loop_count = 0, float speed_ratio = 1.0f) -> bool;
        auto change_motion(uint32_t motion_key, int loop_count = 0, float speed_ratio = 1.0f) -> bool;
        auto intersect(float *pu, float *pv, float *pt) -> bool;
        void get_bound_box(math::vector3 *vt_min, math::vector3 *vt_max);
        auto get_bound_box(uint32_t model_instance_index, math::vector3 *vt_min, math::vector3 *vt_max) -> bool;
        auto bone_matrix(uint32_t model_instance_index, uint32_t bone_index, math::matrix **matrix) -> bool;
        auto composite_bone_matrix(uint32_t model_instance_index, uint32_t bone_index, math::matrix **matrix) -> bool;
        void update_transform(math::matrix *matrix, float seconds_elapsed = 0.0f, int model_instance_index = 0);

      public:
        void build_bounding_sphere();
        void build_bounding_aabb();
        virtual void calculate_b_box();
        auto bounding_sphere(math::vector3 &center, float &radius) -> bool override;
        virtual auto bounding_aabb(math::vector3 &min_value, math::vector3 &max_value) -> bool;

      protected:
        void on_clear() override;
        void on_deform() override;
        void on_update() override;
        void on_render() override;
        void on_blend_render() override;
        void on_render_to_shadow_map() override;
        void on_render_shadow() override;
        void on_render_pc_blocker() override;

      protected:
        bool updated_;
        float local_time_;
        float delay_;
        float second_elapsed_;
        float average_second_elapsed_;
        float radius_;
        math::vector3 center_;
        math::vector3 min_, max_;

        std::vector<lod_controller *> lod_controller_vector_;
        std::vector<model_thing_set> model_thing_set_vector_;
        std::map<uint32_t, graphic_thing::ref *> ro_motion_thing_map_;

      protected:
        void on_update_collision_data(const static_collision_data_vector *pscd_vector) override;
        void on_update_heigh_instance(attribute_instance *attribute_instance) override;
        auto on_get_object_height(float x, float y, float *height) -> bool override;

      public:
        static void create_system(uint32_t capacity);
        static void destroy_system();

        static auto new_value() -> graphic_thing_instance *;
        static void delete_value(graphic_thing_instance *thing_inst);

        static dynamic_pool<graphic_thing_instance> pool_value;

        auto have_blend_thing() -> bool;
    };
}
