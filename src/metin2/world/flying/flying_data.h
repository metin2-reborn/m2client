#pragma once
#include <cstdint>
#include <metin2/core/pool.h>
#include <metin2/math/math.h>
#include <string>

namespace metin2::inline world::inline flying {

    class flying_data {
      public:
        enum {
            FLY_ATTACH_NONE,
            FLY_ATTACH_EFFECT,
            FLY_ATTACH_OBJECT, // not implemented
        };

        enum {
            FLY_ATTACH_TYPE_NONE,
            FLY_ATTACH_TYPE_LINE,
            FLY_ATTACH_TYPE_MULTI_LINE,
            FLY_ATTACH_TYPE_SINE,
            FLY_ATTACH_TYPE_EXP,
        };

        struct flying_attach_data {
            int type_value;
            int fly_type;
            std::string filename;

            bool has_tail;
            uint32_t tail_color;
            float tail_length;
            float tail_size;
            bool rect_shape;

            float roll;
            // Used by MULTI_LINE only; LINE needs none of these.
            float distance;
            float period;
            float amplitude;
        };

        friend class flying_instance;

        friend class CSceneFly;
        friend class CFlyTabPage;

        flying_data();
        virtual ~flying_data();

        void destroy();

        auto load_script_file(const char *filename) -> bool;

        void set_bomb_effect(const char *effect_name_value);

        auto attach_data_reference(int index) -> flying_attach_data &;
        auto attach_data_count() -> int { return attach_data_vector_.size(); }

      protected:
        void initialize_impl();

        std::string filename_;

        bool spreading_;
        bool maintain_parallel_;

        float init_vel_;
        float cone_angle_;
        float roll_angle_;

        math::vector3 ang_vel_;

        float gravity_;

        float bomb_range_;
        std::string bomb_effect_name_;
        uint32_t bomb_effect_id_;

        bool is_homing_;
        float homing_max_angle_;
        float homing_start_time_;

        bool hit_on_background_;
        bool hit_on_another_monster_;
        int pierce_count_;
        float collision_sphere_radius_;

        float range_;
        math::vector3 accel_;

        std::vector<flying_attach_data> attach_data_vector_;

      public:
        static auto new_value() -> flying_data * { return pool_value.alloc(); }
        static void delete_value(flying_data *data) {
            data->destroy();
            pool_value.free(data);
        }

        static void destroy_system() { pool_value.destroy(); }

        static dynamic_pool<flying_data> pool_value;
    };
}
