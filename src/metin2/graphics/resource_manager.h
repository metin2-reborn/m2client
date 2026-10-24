#pragma once

#include <metin2/core/mapped_file.h>
#include "file_loader_thread.h"
#include "resource.h"

#include <map>
#include <set>
#include <string>

namespace metin2::inline graphics {

    class resource_manager {
      public:
        resource_manager();
        ~resource_manager();

        void load_static_cache(const char *file_name);

        void destroy_deleting_list();
        void destroy();

        auto insert_resource_pointer(uint32_t file_crc_value, resource *resource_value) -> resource *;
        auto find_resource_pointer(uint32_t file_crc) -> resource *;
        auto resource_pointer(const char *file_name) -> resource *;
        auto type_resource_pointer(const char *file_name, int type = -1) -> resource *;

        auto is_resource_pointer_data(uint32_t file_crc) -> bool;

        void register_resource_new_function_pointer(const char *file_ext, resource *(*res_new_func)(const char *file_name));

        auto is_file_exist(const char *file_name) -> bool;

        // Reads a resource file: guild marks ("mark/...") from the user's profile, everything else
        // through the file manager.
        static auto file(mapped_file &file, const char *file_name, const void **ppv_data) -> bool;

        void update();
        void reserve_deleting_resource(resource *resource);

      public:
        void process_background_loading();

      protected:
        void destroy_deleting_resource_map();
        void destroy_resource_map();
        void destroy_cache_map();

        auto file_crc_impl(const char *file_name, const char **lower_file = nullptr) -> uint32_t;

      protected:
        using resource_pointer_map = std::map<uint32_t, resource *>;
        using resource_new_function_pointer_map = std::map<std::string, resource *(*)(const char *)>;
        using resource_new_function_by_type_pointer_map = std::map<int, resource *(*)(const char *)>;
        using resource_deleting_map = std::map<resource *, uint32_t>;
        using resource_request_map = std::map<uint32_t, std::string>;
        using resource_ref_decrease_waiting_map = std::map<uint32_t, resource *>;

      protected:
        resource_pointer_map cache_map_;
        resource_pointer_map res_map_;
        resource_new_function_pointer_map res_new_func_map_;
        resource_new_function_by_type_pointer_map res_new_func_by_type_map_;
        resource_deleting_map resource_deleting_map_value_;
        resource_request_map request_map_; // files requested from the loader thread
        resource_request_map waiting_map_;
        resource_ref_decrease_waiting_map res_ref_decrease_waiting_map_;

        static file_loader_thread loading_thread_;
    };

    inline auto get_resource_manager() -> resource_manager & {
        static resource_manager instance;
        return instance;
    }

}
