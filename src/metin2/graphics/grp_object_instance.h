#pragma once

#include <metin2/geometry/collision_data.h>
#include <metin2/geometry/culling_manager.h>
#include "attribute_instance.h"
#include "grp_screen.h"

namespace metin2::inline graphics {

    // Object type IDs returned by graphic_object_instance::get_type.
    enum : uint32_t {
        THING_OBJECT = 0xadf21f13,
        TREE_OBJECT = 0x8ac9f7a6,
        ACTOR_OBJECT = 0x29a76c24,
        EFFECT_OBJECT = 0x1cfa97c6,
        DUNGEON_OBJECT = 0x18326035,
    };

    enum {
        PORTAL_ID_MAX_NUM = 8,
    };

    class graphic_object_instance : public graphic_collision_object {
      public:
        graphic_object_instance();
        ~graphic_object_instance() override;
        virtual auto get_type() const -> uint32_t = 0;

      public:
        auto get_position() const -> const math::vector3 &;
        auto rotation() -> float;
        auto yaw() -> float;
        auto get_pitch() -> float;
        auto get_roll() -> float;

        void set_position(float x, float y, float z);
        void set_position(const math::vector3 &newposition);
        void set_scale(float x, float y, float z);
        void set_rotation(float rotation);
        void set_rotation(float yaw, float pitch, float roll);
        void set_rotation_quaternion(const math::quaternion &q);

        void clear();
        void update();
        auto render() -> bool;
        void blend_render();
        void render_to_shadow_map();
        void render_shadow();
        void render_pc_blocker();
        void deform();
        void transform();

        void show();
        void hide();
        auto is_show() -> bool;

        void block_camera(bool block) { block_camera_ = block; }
        auto block_camera() -> bool { return block_camera_; }

        auto tb_box_min() -> math::vector3 & { return tb_box_min_; }
        auto tb_box_max() -> math::vector3 & { return tb_box_max_; }
        auto b_box_min() -> math::vector3 & { return b_box_min_; }
        auto b_box_max() -> math::vector3 & { return b_box_max_; }

        auto get_transform() -> math::matrix &;
        auto world_matrix() -> const math::matrix & { return world_matrix_; }

        void set_portal(uint32_t index, int id);
        auto portal(uint32_t index) -> int;

        void initialize();
        virtual void on_initialize();

      public:
        void update_bounding_sphere();
        void register_bounding_sphere();
        virtual auto bounding_sphere(math::vector3 &center, float &radius) -> bool = 0;

        virtual void on_render() = 0;
        virtual void on_blend_render() = 0;
        virtual void on_render_to_shadow_map() = 0;
        virtual void on_render_shadow() = 0;
        virtual void on_render_pc_blocker() = 0;
        virtual void on_clear() {}
        virtual void on_update() {}
        virtual void on_deform() {}

      protected:
        math::vector3 position_value_;
        math::vector3 scale_;

        float yaw_;
        float pitch_;
        float roll_;

        math::matrix m_rotation_;

        bool is_visible_;
        math::matrix world_matrix_;

        bool block_camera_;

        math::vector4 tb_box_[8];
        math::vector3 tb_box_min_, tb_box_max_;
        math::vector3 b_box_min_, b_box_max_;

        uint8_t portal_id_[PORTAL_ID_MAX_NUM];

        culling_manager::culling_handle culling_handle_value_;

      public:
        void add_collision(const static_collision_data *pscd, const math::matrix *mat);
        void clear_collision();
        auto collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool;
        auto movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool;
        auto collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3;

        void update_collision_data(const static_collision_data_vector *pscd_vector = nullptr);

      protected:
        collision_instance_vector static_collision_instance_vector_;
        virtual void on_update_collision_data(const static_collision_data_vector *pscd_vector) = 0;

      public:
        auto collision_instance_count() -> uint32_t;
        auto collision_instance_data(uint32_t index) -> base_collision_instance *;

      public:
        void set_height_instance(attribute_instance *attribute_instance);
        void clear_height_instance();

        void update_height_instance(attribute_instance *attribute_instance = nullptr);

        auto is_object_height() -> bool;
        auto object_height(float x, float y, float *height) -> bool;

      protected:
        attribute_instance *height_attribute_instance_;
        virtual void on_update_heigh_instance(attribute_instance *attribute_instance) = 0;
        virtual auto on_get_object_height(float x, float y, float *height) -> bool = 0;
    };
}
