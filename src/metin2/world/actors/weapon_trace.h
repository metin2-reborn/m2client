#pragma once

#include <metin2/models/thing_instance.h>

namespace metin2::inline world::inline actors {

    class weapon_trace {
      public:
        static void destroy_system();
        static void delete_value(weapon_trace *wt_del);
        static auto new_value() -> weapon_trace *;

      public:
        weapon_trace();
        virtual ~weapon_trace();

        void clear();

        void turn_on();
        void turn_off();

        void use_alpha();
        void use_texture();

        void set_texture(const char *file_name);
        auto set_weapon_instance(graphic_thing_instance *instance, uint32_t model_index, const char *bone_name) -> bool;
        void set_position(float fx, float fy, float fz);
        void set_rotation(float rotation);

        void update(float reach_scale);
        void render();

        void initialize();

      protected:
        auto build_vertex() -> bool;

      protected:
        float last_update_;

        using time_point = std::pair<float, math::vector3>;
        using time_point_list = std::deque<time_point>;
        time_point_list short_time_point_list_;
        time_point_list long_time_point_list_;

        std::vector<pdt_vertex> pdt_vertex_vector_;

        float life_time_;
        float sampling_time_;

        graphic_thing_instance *instance_;
        uint32_t model_instance_index_;

        graphic_image_instance image_instance_;

        float fx_;
        float fy_;
        float fz_;
        float rotation_;
        float length_;

        bool is_playing_;
        bool use_texture_;

        int bone_index_;

      protected:
        static dynamic_pool<weapon_trace> pool_value_;
    };
}
