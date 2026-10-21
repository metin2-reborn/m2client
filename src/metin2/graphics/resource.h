#pragma once

#include <cstdint>
#include <metin2/core/reference_object.h>
#include <string>

namespace metin2::inline graphics {

    class resource : public reference_object {
      public:
        using type_type = uint32_t;

        enum state {
            STATE_EMPTY,
            STATE_ERROR,
            STATE_EXIST,
            STATE_LOAD,
            STATE_FREE
        };

      public:
        void clear();

        static auto string_to_type(const char *type) -> type_type;
        static auto type() -> type_type;

        void load();
        void reload();

        virtual auto create_device_objects() -> bool;
        virtual void destroy_device_objects();

      public:
        resource(const char *file_name);
        ~resource() override;

        static void set_delete_immediately(bool is_set = false);

        auto is_data() const -> bool;
        auto is_empty() const -> bool;
        auto is_type(type_type type) -> bool;

        auto load_cost_milli_second() -> uint32_t { return load_cost_milii_second_; }
        auto file_name() const -> const char * { return file_name_.c_str(); }
        auto file_name_string() const -> const std::string & { return file_name_; }

        virtual auto on_load(int size, const void *buf) -> bool = 0;

      protected:
        void set_file_name(const char *file_name);

        virtual void on_clear() = 0;
        virtual auto on_is_empty() const -> bool = 0;
        virtual auto on_is_type(type_type type) -> bool = 0;

        void on_construct() override;
        void on_self_destruct() override;

      protected:
        std::string file_name_;
        uint32_t load_cost_milii_second_;
        state me_state_;

      protected:
        static bool delete_immediately_;
    };
}
