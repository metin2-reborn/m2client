#pragma once

#include <cstdint>
#include <metin2/core/ref.h>
#include <metin2/geometry/collision_data.h>
#include <metin2/math/math.h>
#include <vector>
#include "resource.h"

namespace metin2::inline graphics {

    struct height_data {
        char name_value[32 + 1];
        std::vector<math::vector3> vertex_vector;
    };

    using height_data_vector = std::vector<height_data>;

    class attribute_data : public resource {
      public:
        using ref = ref_ptr<attribute_data>;

      public:
        static auto type() -> type_type;

      public:
        attribute_data(const char *file_name);
        ~attribute_data() override;

        auto get_collision_data_vector() const -> const static_collision_data_vector &;

        auto add_collision_data(const static_collision_data &collision_data) -> size_t;

        auto height_data_count() const -> uint32_t;
        auto height_data_pointer(uint32_t index, const height_data **height_data) const -> bool;

        auto maximize_radius() -> float;

      protected:
        auto on_load(int size, const void *buf) -> bool override;
        void on_clear() override;
        auto on_is_empty() const -> bool override;
        auto on_is_type(type_type type) -> bool override;
        void on_self_destruct() override;

      protected:
        float maximize_radius_;

        static_collision_data_vector static_collision_data_vector_value_;
        height_data_vector height_data_vector_value_;
    };
}
