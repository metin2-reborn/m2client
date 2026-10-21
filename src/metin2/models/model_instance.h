#pragma once

#include <memory>

#include <metin2/geometry/grp_collision_object.h>
#include <metin2/graphics/grp_image.h>

#include "model.h"
#include "motion.h"

namespace metin2::inline models {

    class model_instance : public graphic_collision_object {
      public:
        enum {
            ANIFPS_MIN = 30,
            ANIFPS_MAX = 120,
        };

      public:
        static void destroy_system();

        static auto new_value() -> model_instance *;
        static void delete_value(model_instance *inst);

        static dynamic_pool<model_instance> pool_value;

      public:
        struct create_device_objects_fn {
            void operator()(model_instance *model_instance) { model_instance->create_device_objects(); }
        };

        struct destroy_device_objects_fn {
            void operator()(model_instance *model_instance) { model_instance->destroy_device_objects(); }
        };

      public:
        model_instance();
        ~model_instance() override;

        auto is_empty() -> bool;
        void clear();

        auto create_device_objects() -> bool;
        void destroy_device_objects();

        void update(uint32_t ani_fps);
        void update_local_time(float elapsed_time);
        void update_transform(math::matrix *matrix, float seconds_elapsed);

        void deform_no_skin(const math::matrix *world_matrix);
        void deform(const math::matrix *world_matrix);

        // FIXME: this split only works because the hardware limits us to two textures; eventually the
        // model textures and effect textures should be set up automatically.
        // Separate functions instead of a branch inside one for speed.
        // Buildings always use one texture, characters sometimes two.
        void render_with_one_texture();
        void blend_render_with_one_texture();
        void render_without_texture();

        void set_material_image_pointer(const char *image_name, graphic_image *image);
        void set_material_data(const char *image_name, const material_data &material_data);
        void set_specular_info(const char *mtrl_name, bool enable, float power);

        void set_main_model_pointer(model *model, graphic_vertex_buffer *shared_defromable_vertex_buffer);
        void set_linked_model_pointer(model *model, graphic_vertex_buffer *shared_defromable_vertex_buffer, model_instance **skeleton_inst);

        void set_motion_pointer(const motion *motion, float blend_time = 0.0f, int loop_count = 0, float speed_ratio = 1.0f);
        void change_motion_pointer(const motion *motion, int loop_count = 0, float speed_ratio = 1.0f);
        void set_motion_at_end();
        auto is_motion_playing() -> bool;

        void copy_motion(model_instance *model_instance, bool is_free_source_control = false);

        void set_local_time(float local_time);
        auto reset_local_time() -> int;
        auto local_time() -> float;

        auto vertex_count() -> uint32_t;

        auto bone_matrix_pointer(int bone) const -> const float *;
        auto composite_bone_matrix_pointer(int bone) const -> const float *;
        auto bone_index_by_name(const char *bone_name, int *bone_index) const -> bool;
        void set_parent_model_instance(const model_instance *parent_model_instance, const char *bone_name);
        void set_parent_model_instance(const model_instance *parent_model_instance, int bone);

        auto intersect(const math::matrix *matrix, float *pu, float *pv, float *pt) -> bool;
        void make_bound_box(bound_box *bound_box, const float *mat, const float *obb_min, const float *obb_max, math::vector3 *vt_min, math::vector3 *vt_max);
        void get_bound_box(math::vector3 *vt_min, math::vector3 *vt_max);

      protected:
        void initialize_impl();

        void destroy_model_instance();
        void destroy_mesh_matrices();
        void destroy_dynamic_vertex_buffer();

        void create_model_instance();
        void create_mesh_matrices();
        void create_dynamic_vertex_buffer();

        void destroy_world_pose();
        void create_world_pose(model_instance *src_model_inst);

        auto create_mesh_binding_vector(model_instance *dst_model_inst) -> bool;
        void destroy_mesh_binding_vector();

        auto mesh_bone_indices(unsigned int mesh_binding) const -> int *;

        void set_shared_deformable_vertex_buffer(graphic_vertex_buffer *shared_deformable_vertex_buffer);

        auto deformable_vertex_buffer_ptr() -> gpu::vertex_buffer *;
        auto deformable_vertex_buffer_ref() -> graphic_vertex_buffer &;

        auto world_pose_ptr() const -> granny_world_pose *;

        void update_world_pose();
        void update_world_matrices(const math::matrix *world_matrix);
        void deform_pnt_vertices(void *dest);

        void render_mesh_node_list_with_one_texture(mesh::type mesh_type, material::type mtrl_type);
        void render_mesh_node_list_with_two_texture(mesh::type mesh_type, material::type mtrl_type);
        void render_mesh_node_list_without_texture(mesh::type mesh_type, material::type mtrl_type);

      protected:
        model *model_value_;

        granny_model_instance *pgrn_model_instance_;

        granny_control *pgrn_ctrl_;
        granny_animation *pgrn_ani_;

        std::unique_ptr<math::matrix[]> mesh_matrices_;

        const model_instance *mc_parent_instance_;
        int parent_bone_index_;

        float local_time_;

        uint32_t old_update_frame_;

        material_palette mtrl_pal_;

        granny_world_pose *pgrn_world_pose_real_; // Owns the allocation.
        std::vector<granny_mesh_binding *> vct_pgrn_mesh_binding_;

        graphic_vertex_buffer *shared_deformable_vertex_buffer_;
        graphic_vertex_buffer local_deformable_vertex_buffer_;

        model_instance **skeleton_inst_;

      public:
        auto have_blend_thing() -> bool { return model_value_->have_blend_thing(); }
    };
}
