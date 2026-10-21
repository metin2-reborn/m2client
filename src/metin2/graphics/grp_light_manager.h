#pragma once

#include <metin2/core/pool.h>
#include <metin2/core/util.h>
#include "grp_base.h"

#include <deque>
#include <metin2/gpu/types.h>

namespace metin2::inline graphics {

    using light_id = uint32_t;

    enum light_type {
        LIGHT_TYPE_STATIC,  // Continuously turning on light
        LIGHT_TYPE_DYNAMIC, // Immediately turning off light
    };

    class light_base {
      public:
        light_base() = default;
        virtual ~light_base() = default;

      protected:
        static float cur_time_;
    };

    class light : public graphic_base, public light_base {
      public:
        light();
        ~light() override;

        void initialize();
        void clear();

        void update();

        void set_parameter(light_id id, const gpu::light &light);

        void set_distance(float distance);
        auto distance() const -> float { return distance_; }

        auto get_light_id() -> light_id { return light_id_value_; }

        auto is_edited() -> bool { return is_edited_; }
        void set_device_light(bool active);

        void set_diffuse_color(float fr, float fg, float fb, float fa = 1.0f);
        void set_ambient_color(float fr, float fg, float fb, float fa = 1.0f);
        void set_range(float range);
        void set_position(float fx, float fy, float fz);

        auto get_position() const -> const math::vector3 &;

      private:
        light_id light_id_value_; // Light ID. equal to D3D light index

        gpu::light light_data_;
        bool is_edited_;
        float distance_;

        transitor_color diffuse_color_transitor_;
        transitor_color ambient_color_transitor_;
        transitor_float range_transitor_;
    };

    class light_manager : public graphic_base, public light_base {
      public:
        enum {
            LIGHT_LIMIT_DEFAULT = 3,
        };

        using light_id_deque = std::deque<light_id>;
        using light_map = std::map<light_id, light *>;
        using light_sort_vector = std::vector<light *>;

      public:
        light_manager();
        ~light_manager() override;

        void destroy();

        void initialize();

        void update();

        void register_light(light_type light_type, light_id *pout_light_id, gpu::light &light_data);
        auto get_light(light_id light_id) -> light *;
        void delete_light(light_id light_id);

        void set_center_position(const math::vector3 &position);
        void set_skip_index(uint32_t skip_index);

      protected:
        light_id_deque non_using_light_id_deque_;

        light_map light_map_value_;
        light_sort_vector light_sort_vector_value_;

        math::vector3 center_position_;
        uint32_t limit_light_count_;
        uint32_t skip_index_;

      protected:
        auto new_light_id() -> light_id;
        void release_light_id(light_id light_id);

        dynamic_pool<light> light_pool_;
    };

    inline auto get_light_manager() -> light_manager & {
        static light_manager instance;
        return instance;
    }
}
