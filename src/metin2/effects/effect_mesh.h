#pragma once

#include <metin2/math/math.h>

#include <metin2/graphics/grp_image_instance.h>
#include <metin2/graphics/grp_screen.h>
#include <metin2/graphics/resource.h>

#include "effect_element_base.h"
#include "type.h"

namespace metin2::inline effects {

    class effect_mesh : public resource {
      public:
        struct effect_frame_data {
            uint8_t changed_frame;
            float visibility;
            uint32_t vertex_count;
            uint32_t texture_vertex_count;
            uint32_t index_count;
            std::vector<pt_vertex> pdt_vertex_vector;
        };

        struct effect_mesh_data {
            char object_name[32];
            char diffuse_map_file_name[128];

            std::vector<effect_frame_data> effect_frame_data_vector;
            std::vector<graphic_image *> image_vector;

            static auto new_value() -> effect_mesh_data *;
            static void delete_value(effect_mesh_data *data);

            static void destroy_system();

            static dynamic_pool<effect_mesh_data> pool_value;
        };

      public:
        using ref = ref_ptr<effect_mesh>;

      public:
        static auto type() -> type_type;

      public:
        effect_mesh(const char *file_name);
        ~effect_mesh() override;

        auto frame_count() -> uint32_t;
        auto mesh_count() -> uint32_t;
        auto mesh_data_pointer(uint32_t mesh_index) -> effect_mesh_data *;

        auto texture_vector_pointer(uint32_t mesh_index) -> std::vector<graphic_image *> *;

      protected:
        auto on_load(int size, const void *buf) -> bool override;

        void on_clear() override;
        auto on_is_empty() const -> bool override;
        auto on_is_type(type_type type) -> bool override;

        auto load_data_ver001(int size, const uint8_t *buf) -> bool;
        auto load_data_ver002(int size, const uint8_t *buf) -> bool;

      protected:
        int geom_count_;
        int frame_count_;
        std::vector<effect_mesh_data *> effect_mesh_data_vector_;

        bool is_data_;
    };

    class effect_mesh_script : public effect_element_base {
      public:
        struct mesh_data {
            uint8_t billboard_type;

            bool blending_enable;
            uint8_t blending_src_type;
            uint8_t blending_dest_type;

            uint8_t color_operation_type;
            math::color color_factor;

            bool texture_animation_loop_enable;
            float texture_animation_frame_delay;

            uint32_t texture_animation_start_frame;

            time_event_table_float time_event_alpha;

            mesh_data() {
                time_event_alpha.clear();
            }
        };
        using mesh_data_vector = std::vector<mesh_data>;

      public:
        effect_mesh_script();
        ~effect_mesh_script() override;

        auto mesh_file_name() -> const char *;

        auto check_mesh_index(uint32_t mesh_index) -> bool;
        auto mesh_data_pointer(uint32_t mesh_index, mesh_data **mesh_data_value) -> bool;

        auto billboard_type(uint32_t mesh_index) -> int;
        auto is_blending_enable(uint32_t mesh_index) -> bool;
        auto blending_src_type(uint32_t mesh_index) -> uint8_t;
        auto blending_dest_type(uint32_t mesh_index) -> uint8_t;
        auto color_operation_type(uint32_t mesh_index, uint8_t *type) -> bool;
        auto color_factor(uint32_t mesh_index, math::color *color) -> bool;
        auto time_table_alpha_pointer(uint32_t mesh_index, time_event_table_float **time_event_alpha) -> bool;

        auto is_mesh_animation_loop() -> bool;
        auto mesh_animation_loop_count() -> int;
        auto mesh_animation_frame_delay() -> float;

      protected:
        void on_clear() override;
        auto on_is_data() -> bool override;
        auto on_load_script(asset_table &table) -> bool override;

      protected:
        bool is_mesh_animation_loop_;
        int mesh_animation_loop_count_;
        float mesh_animation_frame_delay_;
        mesh_data_vector mesh_data_vector_value_;

        std::string mesh_file_name_;

      public:
        static void destroy_system();

        static auto new_value() -> effect_mesh_script *;
        static void delete_value(effect_mesh_script *data);

        static dynamic_pool<effect_mesh_script> pool_value;
    };
}
