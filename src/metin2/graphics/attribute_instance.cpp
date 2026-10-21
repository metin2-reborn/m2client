#include "attribute_instance.h"
#include <metin2/core/utils.h>
#include <metin2/geometry/grp_math.h>

namespace metin2::inline graphics {

    dynamic_pool<attribute_instance> attribute_instance::pool_value;

    auto attribute_instance::picking(const math::vector3 &v, const math::vector3 &dir, float &out_x, float &out_y) -> bool {
        if (is_empty())
            return false;

        bool picked = false;
        float nx = 0;
        float ny = 0;

        for (uint32_t i = 0; i < height_data_vector_value_.size(); ++i)
            for (uint32_t j = 0; j < height_data_vector_value_[i].size(); j += 3) {
                const math::vector3 &cv0 = height_data_vector_value_[i][j];
                const math::vector3 &cv2 = height_data_vector_value_[i][j + 1];
                const math::vector3 &cv1 = height_data_vector_value_[i][j + 2];

                math::vector3 n;
                const auto vvv = (cv1 - cv0);
                const auto vvv2 = (cv2 - cv0);
                const auto vvv3 = (cv2 - cv1);
                n = math::cross(vvv, vvv2);
                math::vector3 x;
                float t;
                const auto offset = (v - cv0);
                t = -math::dot(offset, n) / math::dot(dir, n);

                x = v + t * dir;
                const auto vvv4 = (x - cv0);
                const auto vvv5 = (x - cv1);
                const auto vvv6 = (x - cv2);
                math::vector3 temp;
                temp = math::cross(vvv, vvv4);
                if (math::dot(temp, n) < 0) continue;
                temp = math::cross(vvv3, vvv5);
                if (math::dot(temp, n) < 0) continue;
                const auto edge = (cv0 - cv2);
                temp = math::cross(edge, vvv6);
                if (math::dot(temp, n) < 0) continue;

                if (picked) {
                    if ((v.x - x.x) * (v.x - x.x) + (v.y - x.y) * (v.y - x.y) < (v.x - nx) * (v.x - nx) + (v.y - ny) * (v.y - ny)) {
                        nx = x.x;
                        ny = x.y;
                    }
                } else {
                    nx = x.x;
                    ny = x.y;
                }
                picked = true;
            }
        if (picked) {
            out_x = nx;
            out_y = ny;
        }
        return picked;
    }

    auto attribute_instance::height(float fx, float fy, float *height) -> bool {
        if (is_empty())
            return false;

        fy *= -1.0f;

        if (!is_in_height(fx, fy))
            return false;

        bool flag = false;

        for (uint32_t i = 0; i < height_data_vector_value_.size(); ++i)
            for (uint32_t j = 0; j < height_data_vector_value_[i].size(); j += 3) {
                const math::vector3 &vertex0 = height_data_vector_value_[i][j];
                const math::vector3 &vertex1 = height_data_vector_value_[i][j + 1];
                const math::vector3 &vertex2 = height_data_vector_value_[i][j + 2];

                if (
                    (fx < vertex0.x && fx < vertex1.x && fx < vertex2.x) ||
                    (fx > vertex0.x && fx > vertex1.x && fx > vertex2.x) ||
                    (fy < vertex0.y && fy < vertex1.y && fy < vertex2.y) ||
                    (fy > vertex0.y && fy > vertex1.y && fy > vertex2.y))
                    continue;

                if (is_in_triangle2d(vertex0.x, vertex0.y,
                        vertex1.x, vertex1.y,
                        vertex2.x, vertex2.y, fx, fy)) {
                    math::vector3 line1 = vertex1 - vertex0;
                    math::vector3 line2 = vertex2 - vertex0;
                    math::vector3 cross;

                    cross = math::cross(line1, line2);
                    cross = math::normalize(cross);

                    if (0.0f != cross.z) {
                        float fd = (cross.x * vertex0.x + cross.y * vertex0.y + cross.z * vertex0.z);
                        float fm = (cross.x * fx + cross.y * fy);
                        *height = f_max((fd - fm) / cross.z, *height);

                        flag = true;
                    }
                }
            }

        return flag;
    }

    auto attribute_instance::object_pointer() const -> attribute_data * {
        return ro_attribute_data_.pointer();
    }
    auto attribute_instance::is_in_height(float fx, float fy) -> bool {
        float fdx = global_._41 - fx;
        float fdy = global_._42 - fy;
        if (sqrtf(fdx * fdx + fdy * fdy) > height_radius_)
            return false;

        return true;
    }

    void attribute_instance::set_object_pointer(attribute_data *attribute_data) {
        clear();
        ro_attribute_data_.set_pointer(attribute_data);
    }

    void attribute_instance::refresh_object(const math::matrix &global) {
        assert(!ro_attribute_data_.is_null());

        global_ = global;

        height_radius_ = ro_attribute_data_->maximize_radius();

        uint32_t height_data_count = ro_attribute_data_->height_data_count();
        height_data_vector_value_.clear();
        height_data_vector_value_.resize(height_data_count);
        for (uint32_t i = 0; i < height_data_count; ++i) {
            const height_data *height_data;
            if (!ro_attribute_data_->height_data_pointer(i, &height_data))
                continue;

            uint32_t vertex_count = height_data->vertex_vector.size();
            height_data_vector_value_[i].clear();
            height_data_vector_value_[i].resize(vertex_count);
            for (uint32_t j = 0; j < vertex_count; ++j) {
                height_data_vector_value_[i][j] = math::transform_coord(height_data->vertex_vector[j], global_);
            }
        }
    }

    void attribute_instance::create_system(uint32_t capacity) {
        pool_value.create(capacity);
    }

    void attribute_instance::destroy_system() {
        pool_value.destroy();
    }

    auto attribute_instance::new_value() -> attribute_instance * {
        return pool_value.alloc();
    }

    void attribute_instance::delete_value(attribute_instance *inst) {
        pool_value.free(inst);
    }

    auto attribute_instance::is_empty() const -> bool {
        if (!height_data_vector_value_.empty())
            return false;

        return true;
    }

    void attribute_instance::clear() {
        height_radius_ = 0.0f;
        collision_radius_ = 0.0f;
        global_ = math::matrix_identity();

        height_data_vector_value_.clear();

        ro_attribute_data_.set_pointer(nullptr);
    }

    attribute_instance::attribute_instance() = default;
    attribute_instance::~attribute_instance() = default;
}
