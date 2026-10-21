#pragma once

#include <memory>

#include "model.h"
#include "motion.h"

namespace metin2::inline models {

    class graphic_thing : public resource {
      public:
        using ref = ref_ptr<graphic_thing>;

      public:
        static auto type() -> graphic_thing::type_type;

      public:
        graphic_thing(const char *file_name);

        ~graphic_thing() override;

        auto create_device_objects() -> bool override;
        void destroy_device_objects() override;

        auto check_model_index(int model) const -> bool;
        auto model_pointer(int model_value) -> model *;
        auto model_count() const -> int;

        auto check_motion_index(int motion) const -> bool;
        auto motion_pointer(int motion_value) -> motion *;
        auto motion_count() const -> int;

      protected:
        void initialize();

        auto load_models() -> bool;
        auto load_motions() -> bool;

      protected:
        auto on_load(int size, const void *buf) -> bool override;
        void on_clear() override;
        auto on_is_empty() const -> bool override;
        auto on_is_type(type_type type) -> bool override;

      protected:
        granny_file *pgrn_file_;
        granny_file_info *pgrn_file_info_;

        std::unique_ptr<model[]> models_;
        std::unique_ptr<motion[]> motions_;
    };
}
