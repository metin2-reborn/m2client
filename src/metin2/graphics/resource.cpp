#include <metin2/core/crc32.h>
#include <metin2/core/timer.h>
#include <metin2/log/log.h>

#include "resource.h"
#include "resource_manager.h"

#include <cstring>

namespace metin2::inline graphics {

    bool resource::delete_immediately_ = false;

    resource::resource(const char *file_name) : me_state_(STATE_EMPTY) {
        set_file_name(file_name);
    }

    resource::~resource() = default;

    void resource::set_delete_immediately(bool is_set) {
        delete_immediately_ = is_set;
    }

    void resource::on_construct() {
        load();
    }

    void resource::on_self_destruct() {
        if (delete_immediately_)
            clear();
        else
            get_resource_manager().reserve_deleting_resource(this);
    }

    void resource::load() {
        if (me_state_ != STATE_EMPTY)
            return;

        const char *file_name_value = file_name();

        uint32_t start_value = el_timer_get_m_sec();
        mapped_file file_value;
        const void *file_data;

        if (resource_manager::file(file_value, file_name_value, &file_data)) {
            load_cost_milii_second_ = el_timer_get_m_sec() - start_value;

            if (on_load(file_value.size(), file_data)) {
                me_state_ = STATE_EXIST;
            } else {
                metin2::log::debug("Resource {} cannot be loaded", file_name_value);
                me_state_ = STATE_ERROR;
                return;
            }
        } else {
            if (on_load(0, nullptr))
                me_state_ = STATE_EXIST;
            else {
                metin2::log::debug("Resource {} not found", file_name_value);
                me_state_ = STATE_ERROR;
            }
        }
    }

    void resource::reload() {
        clear();
        metin2::log::debug("Reloading resource {}", file_name());

        mapped_file file;
        const void *file_data;

        if (resource_manager::file(file, file_name(), &file_data)) {
            if (on_load(file.size(), file_data)) {
                me_state_ = STATE_EXIST;
            } else {
                me_state_ = STATE_ERROR;
                return;
            }
        } else {
            if (on_load(0, nullptr))
                me_state_ = STATE_EXIST;
            else {
                me_state_ = STATE_ERROR;
            }
        }
    }

    auto resource::string_to_type(const char *type) -> resource::type_type {
        return get_crc32(type, strlen(type));
    }

    void resource::set_file_name(const char *file_name) {
        file_name_ = file_name;
    }

    void resource::clear() {
        on_clear();
        me_state_ = STATE_EMPTY;
    }

    auto resource::is_type(type_type type) -> bool {
        return on_is_type(type);
    }

    auto resource::type() -> resource::type_type {
        static type_type s_type = string_to_type("CResource");
        return s_type;
    }

    auto resource::on_is_type(type_type type) -> bool {
        if (resource::type() == type)
            return true;

        return false;
    }

    auto resource::is_data() const -> bool {
        return me_state_ != STATE_EMPTY;
    }

    auto resource::is_empty() const -> bool {
        return on_is_empty();
    }

    auto resource::create_device_objects() -> bool {
        return true;
    }

    void resource::destroy_device_objects() {
    }
}
