#include <algorithm>
#include <metin2/core/crc32.h>
#include <metin2/core/stl.h>
#include <metin2/core/timer.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/files/eter_mark_manager.h>
#include <metin2/log/log.h>

#include <string>
#include "grp_image.h"
#include "resource_manager.h"

namespace metin2::inline graphics {

    const uint32_t c_Deleting_Wait_Time = 30000;           // delay before deletion (30 s)
    const int c_DeletingCountPerFrame = 30;                // resources checked per frame
    const uint32_t c_Reference_Decrease_Wait_Time = 30000; // delay before releasing a preloaded resource (30 s)

    file_loader_thread resource_manager::loading_thread_;

    void resource_manager::load_static_cache(const char *file_name) {
        resource *res = resource_pointer(file_name);
        if (!res) {
            metin2::log::info("Cannot cache resource {}", file_name);
            return;
        }

        uint32_t cache_key = get_crc32(file_name, strlen(file_name));
        auto f = cache_map_.find(cache_key);
        if (cache_map_.end() != f)
            return;

        res->add_reference();
        cache_map_.insert(resource_pointer_map::value_type(cache_key, res));
    }

    void resource_manager::process_background_loading() {
        auto itor = request_map_.begin();

        while (itor != request_map_.end()) {
            uint32_t file_crc = itor->first;
            std::string &file_name = itor->second;

            if (is_resource_pointer_data(file_crc) ||
                (waiting_map_.end() != waiting_map_.find(file_crc))) {
                itor = request_map_.erase(itor);
                continue;
            }

            loading_thread_.request(file_name);
            waiting_map_.insert(resource_request_map::value_type(file_crc, file_name));
            itor = request_map_.erase(itor);
            // Requesting one file per frame makes loading slow.
        }

        uint32_t dw_current_time = el_timer_get_m_sec();

        file_loader_thread::data *data;
        while (loading_thread_.fetch(&data)) {
            resource *resource = resource_pointer(data->file_name.c_str());

            if (resource) {
                if (resource->is_empty()) {
                    resource->on_load(data->size, data->buf);
                    resource->add_reference_only();

                    // Released after c_Reference_Decrease_Wait_Time.
                    res_ref_decrease_waiting_map_.insert(resource_ref_decrease_waiting_map::value_type(dw_current_time, resource));
                }
            }

            waiting_map_.erase(get_crc32(data->file_name.c_str(), data->file_name.size()));

            delete[] ((char *)data->buf);
            delete data;
        }

        // Drop the extra reference of preloaded resources once they have waited long enough.
        uint32_t l_current_time = el_timer_get_m_sec();

        auto itor_ref = res_ref_decrease_waiting_map_.begin();

        while (itor_ref != res_ref_decrease_waiting_map_.end()) {
            const uint32_t creating_time = itor_ref->first;

            if (l_current_time - creating_time > c_Reference_Decrease_Wait_Time) {
                resource *resource = itor_ref->second;

                resource->release();
                itor_ref = res_ref_decrease_waiting_map_.erase(itor_ref);
            } else
                ++itor_ref;
        }
    }

    void resource_manager::destroy_cache_map() {
        resource_pointer_map::iterator i;
        for (i = cache_map_.begin(); i != cache_map_.end(); ++i) {
            resource *resource = i->second;
            resource->release();
        }

        cache_map_.clear();
    }

    void resource_manager::destroy_deleting_resource_map() {
        metin2::log::debug("Destroying {} resources marked for deletion", resource_deleting_map_value_.size());
        for (auto i = resource_deleting_map_value_.begin(); i != resource_deleting_map_value_.end(); ++i)
            (i->first)->clear();

        resource_deleting_map_value_.clear();
    }

    void resource_manager::destroy_resource_map() {
        metin2::log::debug("Destroying {} resources", res_map_.size());

        resource_pointer_map::iterator i;
        for (i = res_map_.begin(); i != res_map_.end(); ++i) {
            resource *resource = i->second;
            resource->clear();
        }

        stl_wipe_second(res_map_);
    }

    void resource_manager::destroy_deleting_list() {
        resource::set_delete_immediately(true);

        destroy_cache_map();
        destroy_deleting_resource_map();
    }

    void resource_manager::destroy() {
        assert(resource_deleting_map_value_.empty() && "CResourceManager::Destroy - YOU MUST CALL DestroyDeletingList");
        destroy_resource_map();
    }

    void resource_manager::register_resource_new_function_pointer(const char *file_ext, resource *(*new_func)(const char *file_name)) {
        res_new_func_map_[file_ext] = new_func;
    }

    auto resource_manager::insert_resource_pointer(uint32_t file_crc_value, resource *resource_value) -> resource * {
        auto itor = res_map_.find(file_crc_value);

        if (res_map_.end() != itor) {
            metin2::log::error("Resource {} is already registered", resource_value->file_name());
            assert(!"CResource::InsertResourcePointer: Resource already resistered");
            delete resource_value;
            return itor->second;
        }

        res_map_.insert(resource_pointer_map::value_type(file_crc_value, resource_value));
        return resource_value;
    }

    auto convert_path_name(const char *path_name, char *ret_path_name, int ret_len) -> int {
        const char *pc;
        int len = 0;

        for (pc = path_name; *pc && len < ret_len; ++pc, ++len) {
            if (*pc == '/')
                *(ret_path_name++) = '\\';
            else
                *(ret_path_name++) = (char)ascii_tolower(*pc);
        }

        *ret_path_name = '\0';
        return len;
    }

    auto resource_manager::type_resource_pointer(const char *file_name_value, int type_value) -> resource * {
        if (!file_name_value || !*file_name_value) {
            assert(file_name_value != nullptr && *file_name_value != '\0');
            return nullptr;
        }

        const char *file_value;
        uint32_t file_crc_value = file_crc_impl(file_name_value, &file_value);
        resource *resource_value = find_resource_pointer(file_crc_value);

        if (resource_value) // the resource already exists: return it.
            return resource_value;

        resource *(*new_func)(const char *) = nullptr;

        if (type_value != -1) {
            auto f = res_new_func_by_type_map_.find(type_value);

            if (res_new_func_by_type_map_.end() != f)
                new_func = f->second;
        } else {
            const char *file_ext = strrchr(file_value, '.');

            if (file_ext) {
                auto f = res_new_func_map_.find(std::string(file_ext + 1).substr(0, 8));

                if (res_new_func_map_.end() != f)
                    new_func = f->second;
            }
        }

        if (!new_func) {
            metin2::log::error("Resource {} has an unsupported file type", file_value);
            return nullptr;
        }

        resource_value = insert_resource_pointer(file_crc_value, new_func(file_value));
        return resource_value;
    }

    auto resource_manager::resource_pointer(const char *file_name_value) -> resource * {
        if (!file_name_value || !*file_name_value) {
            metin2::log::error("Resource requested with an empty file name");
            return nullptr;
        }

        const char *file_value;
        uint32_t file_crc_value = file_crc_impl(file_name_value, &file_value);
        resource *resource_value = find_resource_pointer(file_crc_value);

        if (resource_value) // the resource already exists: return it.
            return resource_value;

        const char *file_ext = strrchr(file_value, '.');

#ifdef _DEBUG
        if (!IsFileExist(file_name)) {
            if (pcFileExt == NULL || (strcasecmp(pcFileExt, ".fnt") != 0)) {
                metin2::log::error("Resource {} not found", file_name);
            }
        }
#endif

        resource *(*new_func)(const char *) = nullptr;

        if (file_ext) {
            auto f = res_new_func_map_.find(std::string(file_ext + 1).substr(0, 8));

            if (res_new_func_map_.end() != f)
                new_func = f->second;
        }

        if (!new_func) {
            metin2::log::error("Resource {} has an unsupported file type", file_value);
            return nullptr;
        }

        resource_value = insert_resource_pointer(file_crc_value, new_func(file_value));
        return resource_value;
    }

    auto resource_manager::find_resource_pointer(uint32_t file_crc) -> resource * {
        auto itor = res_map_.find(file_crc);

        if (res_map_.end() == itor)
            return nullptr;

        return itor->second;
    }

    auto resource_manager::is_resource_pointer_data(uint32_t file_crc) -> bool {
        auto itor = res_map_.find(file_crc);

        if (res_map_.end() == itor)
            return false;

        return (itor->second)->is_data();
    }

    auto resource_manager::file_crc_impl(const char *file_name, const char **ppsz_lower_file_name) -> uint32_t {
        // The returned name stays valid until the next call on this thread.
        // Keyed by the canonical name, so that the old and the new name of a file give one resource.
        thread_local std::string full_path_file_name;
        full_path_file_name = eter_file_manager::canonical_name(file_name);
        std::ranges::replace(full_path_file_name, '/', '\\');

        if (ppsz_lower_file_name)
            *ppsz_lower_file_name = full_path_file_name.c_str();

        return get_crc32(full_path_file_name.data(), full_path_file_name.size());
    }

    struct dump_data {
        const char *filename;
        float kb;
        uint32_t cost;
    };

    auto dump_kb_compare(const dump_data &lhs, const dump_data &rhs) -> bool {
        return (lhs.kb > rhs.kb) ? true : false;
    }

    auto dump_cost_compare(const dump_data &lhs, const dump_data &rhs) -> bool {
        return (lhs.cost > rhs.cost) ? true : false;
    }

    struct dump_print_fn {
        FILE *fp;
        static float total_kb;

        void operator()(dump_data &data) {
            total_kb += data.kb;
            fprintf(fp, "%6.1f %s\n", data.kb, data.filename);
        }
    };

    float dump_print_fn::total_kb;

    struct dump_cost_print_fn {
        FILE *fp;

        void operator()(dump_data &data) {
            fprintf(fp, "%-4d %s\n", data.cost, data.filename);
        }
    };

    auto resource_manager::is_file_exist(const char *file_name) -> bool {
        if (eter_mark_manager::is_mark_path(file_name))
            return get_mark_manager().is_exist(file_name);
        return get_file_manager().is_exist(file_name);
    }

    auto resource_manager::file(mapped_file &file, const char *file_name, const void **ppv_data) -> bool {
        if (eter_mark_manager::is_mark_path(file_name))
            return get_mark_manager().get(file, file_name, ppv_data);
        return get_file_manager().get(file, file_name, ppv_data);
    }

    void resource_manager::update() {
        uint32_t current_time = el_timer_get_m_sec();
        resource *resource;
        int count = 0;

        auto itor = resource_deleting_map_value_.begin();

        while (itor != resource_deleting_map_value_.end()) {
            resource = itor->first;

            if (current_time >= itor->second) {
                if (resource->can_destroy()) {
                    resource->clear();
                }

                itor = resource_deleting_map_value_.erase(itor);

                if (++count >= c_DeletingCountPerFrame)
                    break;
            } else
                ++itor;
        }

        process_background_loading();
    }

    void resource_manager::reserve_deleting_resource(resource *resource) {
        uint32_t current_time = el_timer_get_m_sec();
        resource_deleting_map_value_.insert(resource_deleting_map::value_type(resource, current_time + c_Deleting_Wait_Time));
    }

    resource_manager::resource_manager() = default;

    resource_manager::~resource_manager() {
        destroy();
    }
}
