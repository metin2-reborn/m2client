#pragma once

#include <metin2/core/pool.h>
#include <vector>
#include "attribute_data.h"

namespace metin2::inline graphics {

    class attribute_instance {
      public:
        attribute_instance();
        virtual ~attribute_instance();

        void clear();
        auto is_empty() const -> bool;

        void set_object_pointer(attribute_data *attribute_data);
        void refresh_object(const math::matrix &global);
        auto object_pointer() const -> attribute_data *;

        auto picking(const math::vector3 &v, const math::vector3 &dir, float &out_x, float &out_y) -> bool;

        auto is_in_height(float fx, float fy) -> bool;
        auto height(float fx, float fy, float *height) -> bool;

      protected:
        void set_global_matrix(const math::matrix &global);

      protected:
        float collision_radius_;
        float height_radius_;

        math::matrix global_;

        std::vector<std::vector<math::vector3>> height_data_vector_value_;

        attribute_data::ref ro_attribute_data_;

      public:
        static void create_system(uint32_t capacity);
        static void destroy_system();

        static auto new_value() -> attribute_instance *;
        static void delete_value(attribute_instance *inst);

        static dynamic_pool<attribute_instance> pool_value;
    };
}
