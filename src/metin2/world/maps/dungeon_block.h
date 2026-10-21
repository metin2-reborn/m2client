#pragma once

#include <metin2/graphics/grp_object_instance.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/models/model_instance.h>
#include <metin2/models/thing.h>

namespace metin2::inline world::inline maps {

    class dungeon_model_instance;

    class dungeon_block : public graphic_object_instance {
      public:
        using model_instance_container = std::vector<dungeon_model_instance *>;
        enum {
            ID = THING_OBJECT
        };
        auto get_type() const -> uint32_t override { return ID; }

      public:
        dungeon_block();
        ~dungeon_block() override;

        void destroy();

        void build_bounding_sphere();
        auto load(const char *file_name) -> bool;

        auto intersect(float *pfu, float *pfv, float *pft) -> bool;
        void get_bound_box(math::vector3 *min_value, math::vector3 *max_value);

        void update();
        void render();

        auto bounding_sphere(math::vector3 &center, float &radius) -> bool override;
        void on_update_collision_data(const static_collision_data_vector *pscd_vector) override;
        void on_update_heigh_instance(attribute_instance *attribute_instance) override;
        auto on_get_object_height(float x, float y, float *height) -> bool override;

        void on_render() override {}
        void on_blend_render() override {}
        void on_render_to_shadow_map() override {}
        void on_render_shadow() override;
        void on_render_pc_blocker() override {}

      protected:
        void initialize_impl();

      protected:
        math::vector3 center_;
        float radius_;

        graphic_thing *thing_;
        model_instance_container model_instance_container_value_;
        graphic_vertex_buffer deformable_vertex_buffer_;
    };
}
