#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "model_instance.h"
#include "thing.h"

namespace metin2::inline models {

    // Pre-allocates and releases the pool of vertex buffers shared by deformable model instances.
    void granny_create_shared_deform_buffer();
    void granny_destroy_shared_deform_buffer();

    class lod_controller : public graphic_base {
      public:
        static void set_min_lod_mode(bool is_enable);

      public:
        struct set_local_time_fn {
            float local_time;
            void operator()(lod_controller *controller) {
                controller->set_local_time(local_time);
            }
        };

        struct update_time_fn {
            float elapsed_time;

            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->update_time(elapsed_time);
            }
        };

        struct update_lod_level_fn {
            float distance_from_center;
            float distance_from_camera;

            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->update_lod_level(distance_from_center, distance_from_camera);
            }
        };

        struct render_with_one_texture_fn {
            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->render_with_one_texture();
            }
        };

        struct blend_render_with_one_texture_fn {
            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->blend_render_with_one_texture();
            }
        };

        struct render_to_shadow_map_fn {
            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->render_to_shadow_map();
            }
        };

        struct render_shadow_fn {
            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->render_shadow();
            }
        };

        struct deform_fn {
            const math::matrix *mc_world_matrix;

            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->deform(mc_world_matrix);
            }
        };
        struct deform_all_fn {
            const math::matrix *mc_world_matrix;

            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->deform_all(mc_world_matrix);
            }
        };

        struct create_device_objects_fn {
            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->create_device_objects();
            }
        };

        struct destroy_device_objects_fn {
            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->destroy_device_objects();
            }
        };

        struct bound_box_fn {
            math::vector3 *vt_min;
            math::vector3 *vt_max;

            bound_box_fn(math::vector3 *vt_min_value, math::vector3 *vt_max_value) {
                vt_min = vt_min_value;
                vt_max = vt_max_value;
            }

            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->get_bound_box(vt_min, vt_max);
            }
        };

        struct reset_local_time_fn {
            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->reset_local_time();
            }
        };

        struct set_motion_pointer_fn {
            const motion *motion_value;
            float speed_ratio;
            float blend_time;
            int loop_count;

            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->set_motion_pointer(motion_value, blend_time, loop_count, speed_ratio);
            }
        };

        struct change_motion_pointer_fn {
            const motion *motion_value;
            float speed_ratio;
            int loop_count;

            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->change_motion_pointer(motion_value, loop_count, speed_ratio);
            }
        };

        struct end_stop_motion_pointer_fn {
            const motion *motion_value;

            void operator()(lod_controller *controller) {
                if (controller->is_model_instance())
                    controller->set_motion_at_end();
            }
        };

        lod_controller();
        ~lod_controller() override;

        void clear();

        void create_device_objects();
        void destroy_device_objects();

        void add_model(graphic_thing *thing, int src_model, lod_controller *skel_lod_controller = nullptr);
        void attach_model_instance(lod_controller *src_lod_controller, const char *bone_name);
        void detach_model_instance(lod_controller *src_lod_controller);
        void set_lod_limits(float near_lod, float far_lod);
        void set_lod_level(uint8_t lod_level);
        auto lod_level() -> uint8_t { return lod_level_; }
        void set_material_image_pointer(const char *image_name, graphic_image *image);
        void set_material_data(const char *image_name, const material_data &material_data);
        void set_specular_info(const char *mtrl_name, bool enable, float power);

        void render_with_one_texture();
        void blend_render_with_one_texture();

        void update_lod_level(float distance_from_center, float distance_from_camera);
        void update_time(float elapsed_time);

        void deform(const math::matrix *world_matrix);
        void deform_no_skin(const math::matrix *world_matrix);
        void deform_all(const math::matrix *world_matrix);

        void render_to_shadow_map();
        void render_shadow();

        void get_bound_box(math::vector3 *vt_min, math::vector3 *vt_max);
        auto intersect(const math::matrix *matrix, float *u, float *v, float *t) -> bool;

        void set_local_time(float local_time);
        void reset_local_time();

        void set_motion_pointer(const motion *motion, float blend_time, int loop_count, float speed_ratio);
        void change_motion_pointer(const motion *motion, int loop_count, float speed_ratio);
        void set_motion_at_end();

        auto is_model_instance() -> bool;
        auto get_model_instance() -> model_instance *;
        auto have_blend_thing() -> bool { return nullptr != get_model_instance() ? get_model_instance()->have_blend_thing() : false; } // A null model instance here crashed the client.

      protected:
        void set_current_model_instance(model_instance *pgrn_model_instance);
        void refresh_attached_model_instance();

        void reserve_shared_deformable_vertex_buffer(uint32_t deformable_vertex_count);

      protected:
        float lod_distance_;
        uint32_t lod_ani_fps_;

        struct attaching_model_data {
            lod_controller *lod_controller_value;
            std::string bone_name;
        };

        std::vector<attaching_model_data> attached_model_data_vector_;
        lod_controller *attached_parent_model_;

        uint8_t lod_level_;
        model_instance *current_model_instance_;

        std::deque<model_instance *> que_model_inst_;

        graphic_vertex_buffer *shared_deformable_vertex_buffer_;
    };
}
