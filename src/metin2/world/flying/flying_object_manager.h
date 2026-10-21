#pragma once

#include "fly_target.h"

#include <list>
#include <map>
#include <set>

namespace metin2::inline world::inline maps {
    class map_manager;
}
namespace metin2::inline world::inline actors {
    class actor_instance;
}

namespace metin2::inline world::inline flying {

    class flying_instance;
    class flying_data;

    class flying_manager {
      public:
        enum index_fly_type {
            INDEX_FLY_TYPE_NORMAL,
            INDEX_FLY_TYPE_FIRE_CRACKER,
            INDEX_FLY_TYPE_AUTO_FIRE,
        };

      public:
        flying_manager();
        ~flying_manager();

        void destroy();

        void delete_all_instances();

        auto register_flying_data(const char *filename) -> bool;
        auto register_flying_data(const char *filename, uint32_t &r_ret_crc) -> bool;

        auto create_flying_instance_fly_target(const uint32_t id, const math::vector3 &start_position, const fly_target &cr_fly_target, bool can_attack) -> flying_instance *;

        void update();
        void render();

        void set_map_manager_ptr(map_manager *map_manager) { map_manager_value_ = map_manager; }
        auto map_manager_ptr() -> map_manager * { return map_manager_value_; }

      public: // driven by server packets
        auto register_indexed_fly_data(uint32_t index, uint8_t type, const char *file_name) -> bool;
        void create_indexed_fly(uint32_t index, actor_instance *start_actor, actor_instance *end_actor);

      private:
        void destroy_flying_instance_list();
        void destroy_flying_data_map();

        using flying_data_map = std::map<uint32_t, flying_data *>;
        using flying_instance_list = std::list<flying_instance *>;

        struct index_fly_data {
            uint8_t type_value;
            uint32_t crc;
        };
        using index_fly_data_map = std::map<uint32_t, index_fly_data>;

        flying_data_map map_fly_data_;
        flying_instance_list lst_fly_inst_;
        index_fly_data_map map_index_fly_data_;

        map_manager *map_manager_value_;

        uint32_t id_counter_;
    };

    inline auto get_flying_manager() -> flying_manager & {
        static flying_manager instance;
        return instance;
    }
}
