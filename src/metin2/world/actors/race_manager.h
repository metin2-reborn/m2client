#pragma once

#include "race_data.h"

namespace metin2::inline world::inline actors {

    class race_manager {
      public:
        using race_data_map = std::map<uint32_t, race_data *>;
        using race_data_iterator = race_data_map::iterator;

      public:
        race_manager();
        ~race_manager();

        void create();
        void destroy();

        void register_race_name(uint32_t race_index, const char *name);
        void register_race_src_name(const char *name, const char *src_name);

        void set_path_name(const char *path_name);
        auto full_path_file_name(const char *file_name) -> const char *;

        void create_race(uint32_t race_index);
        void select_race(uint32_t race_index);
        auto selected_race_data_pointer() -> race_data *;

        auto race_data_pointer(uint32_t race_index, race_data **race_data) -> bool;

      protected:
        auto load_race_data_impl(uint32_t race_index) -> race_data *;
        auto load_race_motion_list(race_data &race_data, const char *path_name, const char *motion_list_file_name) -> bool;

        void initialize_impl();
        void destroy_race_data_map();

      protected:
        race_data_map race_data_map_value_;

        std::map<std::string, std::string> map_race_name_src_name_;
        std::map<uint32_t, std::string> map_race_key_race_name_;

      private:
        std::string path_name_;
        race_data *selected_race_data_;
    };

    inline auto get_race_manager() -> race_manager & {
        static race_manager instance;
        return instance;
    }
}
