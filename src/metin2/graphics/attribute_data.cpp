#include "attribute_data.h"
#include <metin2/core/utils.h>

namespace metin2::inline graphics {

    const char c_szAttributeDataFileHeader[] = "AttributeData";
    const int c_iAttributeDataFileHeaderLength = 13;

    auto attribute_data::get_collision_data_vector() const -> const static_collision_data_vector & {
        return static_collision_data_vector_value_;
    }

    auto attribute_data::height_data_count() const -> uint32_t {
        return height_data_vector_value_.size();
    }

    auto attribute_data::height_data_pointer(uint32_t index, const height_data **height_data) const -> bool {
        if (index >= height_data_count())
            return false;

        *height_data = &height_data_vector_value_[index];

        return true;
    }

    auto attribute_data::maximize_radius() -> float {
        return maximize_radius_;
    }

    auto attribute_data::add_collision_data(const static_collision_data &data) -> size_t {
        static_collision_data_vector_value_.push_back(data);
        return static_collision_data_vector_value_.size();
    }

    auto attribute_data::on_load(int /*iSize*/, const void *c_pv_buf) -> bool {
        if (!c_pv_buf) {
            // A missing file is not an error: collision data is then generated from the graphic model, so keep the resource.
            return true;
        }

        const auto *c_pb_buf = static_cast<const uint8_t *>(c_pv_buf);

        char header[c_iAttributeDataFileHeaderLength + 1];
        memcpy(header, c_pb_buf, c_iAttributeDataFileHeaderLength + 1);
        c_pb_buf += c_iAttributeDataFileHeaderLength + 1;
        if (strcmp(header, c_szAttributeDataFileHeader))
            return false;

        uint32_t collision_data_count;
        uint32_t height_data_count;
        memcpy(&collision_data_count, c_pb_buf, sizeof(uint32_t));
        c_pb_buf += sizeof(uint32_t);
        memcpy(&height_data_count, c_pb_buf, sizeof(uint32_t));
        c_pb_buf += sizeof(uint32_t);

        static_collision_data_vector_value_.clear();
        static_collision_data_vector_value_.resize(collision_data_count);
        height_data_vector_value_.clear();
        height_data_vector_value_.resize(height_data_count);

        for (uint32_t i = 0; i < collision_data_count; ++i) {
            static_collision_data &collision_data = static_collision_data_vector_value_[i];
            memcpy(&collision_data.type_value, c_pb_buf, sizeof(uint32_t));
            c_pb_buf += sizeof(uint32_t);
            memcpy(collision_data.name_value, c_pb_buf, 32);
            c_pb_buf += 32;
            memcpy(&collision_data.position_value, c_pb_buf, sizeof(math::vector3));
            c_pb_buf += sizeof(math::vector3);

            switch (collision_data.type_value) {
            case COLLISION_TYPE_PLANE:
                memcpy(collision_data.dimensions, c_pb_buf, 2 * sizeof(float));
                c_pb_buf += 2 * sizeof(float);
                break;
            case COLLISION_TYPE_BOX:
                memcpy(collision_data.dimensions, c_pb_buf, 3 * sizeof(float));
                c_pb_buf += 3 * sizeof(float);
                break;
            case COLLISION_TYPE_SPHERE:
                memcpy(collision_data.dimensions, c_pb_buf, sizeof(float));
                c_pb_buf += sizeof(float);
                break;
            case COLLISION_TYPE_CYLINDER:
                memcpy(collision_data.dimensions, c_pb_buf, 2 * sizeof(float));
                c_pb_buf += 2 * sizeof(float);
                break;
            case COLLISION_TYPE_AABB:
                memcpy(collision_data.dimensions, c_pb_buf, 3 * sizeof(float));
                c_pb_buf += 3 * sizeof(float);
                break;
            case COLLISION_TYPE_OBB:
                memcpy(collision_data.dimensions, c_pb_buf, 3 * sizeof(float));
                c_pb_buf += 3 * sizeof(float);
                break;
            }

            memcpy(collision_data.quat_rotation, c_pb_buf, sizeof(math::quaternion));
            c_pb_buf += sizeof(math::quaternion);
        }

        for (uint32_t j = 0; j < height_data_count; ++j) {
            height_data &height_data = height_data_vector_value_[j];
            memcpy(height_data.name_value, c_pb_buf, 32);
            c_pb_buf += 32;

            uint32_t primitive_count;
            memcpy(&primitive_count, c_pb_buf, sizeof(uint32_t));
            c_pb_buf += sizeof(uint32_t);

            height_data.vertex_vector.clear();
            height_data.vertex_vector.resize(primitive_count);
            memcpy(&height_data.vertex_vector[0], c_pb_buf, primitive_count * sizeof(math::vector3));
            c_pb_buf += primitive_count * sizeof(math::vector3);

            for (uint32_t k = 0; k < height_data.vertex_vector.size(); ++k) {
                maximize_radius_ = f_max(maximize_radius_, fabs(height_data.vertex_vector[k].x) + 50.0f);
                maximize_radius_ = f_max(maximize_radius_, fabs(height_data.vertex_vector[k].y) + 50.0f);
                maximize_radius_ = f_max(maximize_radius_, fabs(height_data.vertex_vector[k].z) + 50.0f);
            }
        }

        return true;
    }

    void attribute_data::on_clear() {
        static_collision_data_vector_value_.clear();
        height_data_vector_value_.clear();
    }

    auto attribute_data::on_is_empty() const -> bool {
        if (!static_collision_data_vector_value_.empty())
            return false;
        if (!height_data_vector_value_.empty())
            return false;

        return true;
    }

    auto attribute_data::on_is_type(type_type type) -> bool {
        if (attribute_data::type() == type)
            return true;

        return resource::on_is_type(type);
    }

    auto attribute_data::type() -> attribute_data::type_type {
        static type_type s_type = string_to_type("CAttributeData");
        return s_type;
    }

    void attribute_data::on_self_destruct() {
        clear();
    }

    attribute_data::attribute_data(const char *file_name) : resource(file_name) {
        maximize_radius_ = 0.0f;
    }

    attribute_data::~attribute_data() = default;
}
