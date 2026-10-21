#pragma once

#include "effect_instance.h"

namespace metin2::inline effects {

    class effect_manager : public screen {
      public:
        using effect_data_map = std::map<uint32_t, effect_data *>;
        using effect_instance_map = std::map<uint32_t, effect_instance *>;

      public:
        effect_manager();
        ~effect_manager() override;

        void destroy();

        void update_sound();
        void update();
        void render();

        void get_info(std::string *info);

        auto is_alive_effect(uint32_t instance_index) -> bool;

        auto register_effect(const char *file_name, bool is_exist_delete = false, bool is_need_cache = false) -> bool;
        auto register_effect2(const char *file_name, uint32_t *ret_crc, bool is_need_cache = false) -> bool;

        void delete_all_instances();

        auto create_effect(uint32_t id, const math::vector3 &position, const math::vector3 &rotation) -> int;
        auto create_effect(const char *file_name, const math::vector3 &position, const math::vector3 &rotation) -> int;

        void create_effect_instance(uint32_t instance_index, uint32_t id);
        auto select_effect_instance(uint32_t instance_index) -> bool;
        auto destroy_effect_instance(uint32_t instance_index) -> bool;
        void deactive_effect_instance(uint32_t instance_index);

        void set_effect_textures(uint32_t id, std::vector<std::string> textures);
        void set_effect_instance_global_matrix(const math::matrix &global);

        void show_effect();
        void hide_effect();

        auto empty_index() -> int;
        auto get_effect_data(uint32_t id, effect_data **effect) -> bool;

        // For effects placed directly in an area: the instance is handed to the caller and not kept in
        // the manager's instance map.
        void create_unsafe_effect_instance(uint32_t effect_data_id, effect_instance **effect_instance);
        auto destroy_unsafe_effect_instance(effect_instance *effect_instance) -> bool;

      protected:
        void initialize_impl();

        void destroy_effect_instance_map();
        void destroy_effect_cache_map();
        void destroy_effect_data_map();

      protected:
        effect_data_map eft_data_map_;
        effect_instance_map eft_inst_map_;
        effect_instance_map eft_cache_map_;

        effect_instance *selected_effect_instance_;
    };

    inline auto get_effect_manager() -> effect_manager & {
        static effect_manager instance;
        return instance;
    }
}
