#include "collision_data.h"
#include <metin2/core/pool.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/grp_screen.h>
#include <metin2/graphics/state_manager.h>
#include "grp_math.h"
#include "line_intersect_utils.h"

namespace metin2::inline geometry {
    const float gc_reduce_move = 0.5f;

    dynamic_pool<sphere_collision_instance> sci;
    dynamic_pool<cylinder_collision_instance> cci;
    dynamic_pool<plane_collision_instance> pci;
    dynamic_pool<aabb_collision_instance> aci;
    dynamic_pool<obb_collision_instance> oci;

    void destroy_collision_instance_system() {
        sci.destroy();
        cci.destroy();
        pci.destroy();
        aci.destroy();
        oci.destroy();
    }

    auto base_collision_instance::build_collision_instance(const static_collision_data *collision_data, const math::matrix *mat) -> base_collision_instance * {
        switch (collision_data->type_value) {
        case COLLISION_TYPE_PLANE:
            {
                plane_collision_instance *ppci = pci.alloc();
                math::matrix rotation;
                math::matrix translation_local;
                rotation = math::matrix_rotation_quaternion(collision_data->quat_rotation);
                translation_local = math::matrix_translation(collision_data->position_value.x, collision_data->position_value.y, collision_data->position_value.z);
                math::matrix transform = rotation * translation_local * *mat;

                plane_data &plane_data = ppci->attribute();
                plane_data.position_value = math::transform_coord(collision_data->position_value, *mat);
                float half_width = collision_data->dimensions[0] / 2.0f;
                float half_length = collision_data->dimensions[1] / 2.0f;

                plane_data.quad_position[0].x = -half_width;
                plane_data.quad_position[0].y = -half_length;
                plane_data.quad_position[0].z = 0.0f;
                plane_data.quad_position[1].x = +half_width;
                plane_data.quad_position[1].y = -half_length;
                plane_data.quad_position[1].z = 0.0f;
                plane_data.quad_position[2].x = -half_width;
                plane_data.quad_position[2].y = +half_length;
                plane_data.quad_position[2].z = 0.0f;
                plane_data.quad_position[3].x = +half_width;
                plane_data.quad_position[3].y = +half_length;
                plane_data.quad_position[3].z = 0.0f;
                for (uint32_t i = 0; i < 4; ++i)
                    plane_data.quad_position[i] = math::transform_coord(plane_data.quad_position[i], transform);
                math::vector3 line0 = plane_data.quad_position[1] - plane_data.quad_position[0];
                math::vector3 line1 = plane_data.quad_position[2] - plane_data.quad_position[0];
                math::vector3 line2 = plane_data.quad_position[1] - plane_data.quad_position[3];
                math::vector3 line3 = plane_data.quad_position[2] - plane_data.quad_position[3];
                line0 = math::normalize(line0);
                line1 = math::normalize(line1);
                line2 = math::normalize(line2);
                line3 = math::normalize(line3);
                plane_data.normal_value = math::cross(line0, line1);
                plane_data.normal_value = math::normalize(plane_data.normal_value);

                plane_data.inside_vector[0] = math::cross(plane_data.normal_value, line0);
                plane_data.inside_vector[1] = math::cross(line1, plane_data.normal_value);
                plane_data.inside_vector[2] = math::cross(line2, plane_data.normal_value);
                plane_data.inside_vector[3] = math::cross(plane_data.normal_value, line3);

                return ppci;
            }
            break;
        case COLLISION_TYPE_BOX:
            assert(false && "COLLISION_TYPE_BOX not implemented");
            break;
        case COLLISION_TYPE_AABB:
            {
                aabb_collision_instance *paci = aci.alloc();

                math::matrix translation_local;
                translation_local = math::matrix_translation(collision_data->position_value.x, collision_data->position_value.y, collision_data->position_value.z);

                math::vector3 pos;
                pos.x = translation_local._41;
                pos.y = translation_local._42;
                pos.z = translation_local._43;

                aabb_data &aabb_data = paci->attribute();
                aabb_data.min.x = pos.x - collision_data->dimensions[0];
                aabb_data.min.y = pos.y - collision_data->dimensions[1];
                aabb_data.min.z = pos.z - collision_data->dimensions[2];
                aabb_data.max.x = pos.x + collision_data->dimensions[0];
                aabb_data.max.y = pos.y + collision_data->dimensions[1];
                aabb_data.max.z = pos.z + collision_data->dimensions[2];

                math::matrix transform = *mat;
                aabb_data.min = math::transform_coord(aabb_data.min, transform);
                aabb_data.max = math::transform_coord(aabb_data.max, transform);

                return paci;
            }
            break;
        case COLLISION_TYPE_OBB:
            {
                obb_collision_instance *poci = oci.alloc();

                math::matrix translation_local;
                translation_local = math::matrix_translation(collision_data->position_value.x, collision_data->position_value.y, collision_data->position_value.z);
                math::matrix rotation;
                rotation = math::matrix_rotation_quaternion(collision_data->quat_rotation);

                math::matrix translation_world;
                translation_world = math::matrix_identity();
                translation_world._41 = mat->_41;
                translation_world._42 = mat->_42;
                translation_world._43 = mat->_43;
                translation_world._44 = mat->_44;

                math::vector3 min_value, max_value;
                min_value.x = collision_data->position_value.x - collision_data->dimensions[0];
                min_value.y = collision_data->position_value.y - collision_data->dimensions[1];
                min_value.z = collision_data->position_value.z - collision_data->dimensions[2];
                max_value.x = collision_data->position_value.x + collision_data->dimensions[0];
                max_value.y = collision_data->position_value.y + collision_data->dimensions[1];
                max_value.z = collision_data->position_value.z + collision_data->dimensions[2];

                min_value = math::transform_coord(min_value, *mat);
                max_value = math::transform_coord(max_value, *mat);
                math::vector3 position = (min_value + max_value) * 0.5f;

                obb_data &obb_data = poci->attribute();
                obb_data.min.x = position.x - collision_data->dimensions[0];
                obb_data.min.y = position.y - collision_data->dimensions[1];
                obb_data.min.z = position.z - collision_data->dimensions[2];
                obb_data.max.x = position.x + collision_data->dimensions[0];
                obb_data.max.y = position.y + collision_data->dimensions[1];
                obb_data.max.z = position.z + collision_data->dimensions[2];

                obb_data.rot = math::matrix_identity();
                obb_data.rot = *mat;
                obb_data.rot._41 = 0;
                obb_data.rot._42 = 0;
                obb_data.rot._43 = 0;
                obb_data.rot._44 = 1;

                return poci;
            }
            break;
        case COLLISION_TYPE_SPHERE:
            {
                sphere_collision_instance *psci = sci.alloc();

                math::matrix translation_local;
                translation_local = math::matrix_translation(collision_data->position_value.x, collision_data->position_value.y, collision_data->position_value.z);
                translation_local = translation_local * *mat;

                sphere_data &sphere_data = psci->attribute();
                sphere_data.position_value.x = translation_local._41;
                sphere_data.position_value.y = translation_local._42;
                sphere_data.position_value.z = translation_local._43;
                sphere_data.radius = collision_data->dimensions[0];

                return psci;
            }
            break;
        case COLLISION_TYPE_CYLINDER:
            {
                cylinder_collision_instance *pcci = cci.alloc();

                math::matrix translation_local;
                translation_local = math::matrix_translation(collision_data->position_value.x, collision_data->position_value.y, collision_data->position_value.z);
                translation_local = translation_local * *mat;

                cylinder_data &cylinder_data = pcci->attribute();
                cylinder_data.radius = collision_data->dimensions[0];
                cylinder_data.height = collision_data->dimensions[1];
                cylinder_data.position_value.x = translation_local._41;
                cylinder_data.position_value.y = translation_local._42;
                cylinder_data.position_value.z = translation_local._43;

                return pcci;
            }
            break;
        }
        assert(false && "NOT_REACHED");
        return nullptr;
    }

    void base_collision_instance::destroy() {
        on_destroy();
    }

    auto sphere_collision_instance::attribute() -> sphere_data & {
        return attribute_;
    }

    auto sphere_collision_instance::attribute() const -> const sphere_data & {
        return attribute_;
    }

    void sphere_collision_instance::render(uint32_t fill_mode) {
        static screen s;
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, 0xffffffff);
        s.render_sphere(nullptr, attribute_.position_value.x, attribute_.position_value.y, attribute_.position_value.z, attribute_.radius, fill_mode);
    }

    void sphere_collision_instance::on_destroy() {
        sci.free(this);
    }

    auto sphere_collision_instance::on_movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {
        if (square_distance_between_linesegment_and_point(s.last_position, s.position_value, attribute_.position_value) < (attribute_.radius + s.radius) * (attribute_.radius + s.radius)) {
            // Only a movement towards the center collides, so a sphere already inside can leave.
            if (get_vector3_distance(s.position_value, attribute_.position_value) <
                get_vector3_distance(s.last_position, attribute_.position_value))
                return true;
        }

        return false;
    }

    auto sphere_collision_instance::on_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {

        if (square_distance_between_linesegment_and_point(s.last_position, s.position_value, attribute_.position_value) < (attribute_.radius + s.radius) * (attribute_.radius + s.radius)) {
            return true;
        }

        return false;
    }

    auto sphere_collision_instance::on_get_collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 {
        const auto offset = (s.position_value - attribute_.position_value);
        if (math::length_sq(offset) >= (s.radius + attribute_.radius) * (attribute_.radius + s.radius))
            return math::vector3(0.0f, 0.0f, 0.0f);
        math::vector3 c;
        const auto move = (s.position_value - s.last_position);
        const auto up = math::vector3(0.0f, 0.0f, 1.0f);
        c = math::cross(move, up);

        float sum = -math::dot(c, offset);
        float mul = (s.radius + attribute_.radius) * (s.radius + attribute_.radius) - math::length_sq(offset);

        if (sum * sum - 4 * mul <= 0)
            return math::vector3(0.0f, 0.0f, 0.0f);
        float sq = sqrt(sum * sum - 4 * mul);
        float t1 = -sum - sq, t2 = -sum + sq;
        t1 *= 0.5f;
        t2 *= 0.5f;

        if (fabs(t1) <= fabs(t2)) {
            return (gc_reduce_move * t1) * c;
        } else
            return (gc_reduce_move * t2) * c;
    }

    auto plane_collision_instance::attribute() -> plane_data & {
        return attribute_;
    }

    auto plane_collision_instance::attribute() const -> const plane_data & {
        return attribute_;
    }

    auto plane_collision_instance::on_movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {
        math::vector3 sphere_position = s.position_value - attribute_.position_value;
        math::vector3 sphere_last_position = s.last_position - attribute_.position_value;

        float position1 = math::dot(attribute_.normal_value, sphere_position);
        float position2 = math::dot(attribute_.normal_value, sphere_last_position);

        if ((position1 > 0.0f && position2 < 0.0f) || (position1 < 0.0f && position2 > 0.0f) || ((position1) <= s.radius && position1 >= -s.radius)) {
            math::vector3 quad_position1 = s.position_value - attribute_.quad_position[0];
            math::vector3 quad_position2 = s.position_value - attribute_.quad_position[3];

            if (math::dot(quad_position1, attribute_.inside_vector[0]) > -s.radius)
                if (math::dot(quad_position1, attribute_.inside_vector[1]) > -s.radius)
                    if (math::dot(quad_position2, attribute_.inside_vector[2]) > -s.radius)
                        if (math::dot(quad_position2, attribute_.inside_vector[3]) > -s.radius) {
                            // Only a movement towards the center collides, so a sphere already inside can leave.
                            const auto offset_2 = (s.position_value - attribute_.position_value);
                            const auto offset_3 = (s.last_position - attribute_.position_value);
                            if (fabs(math::dot(offset_2, attribute_.normal_value)) <
                                fabs(math::dot(offset_3, attribute_.normal_value)))
                                return true;
                        }
        }

        return false;
    }

    auto plane_collision_instance::on_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {

        math::vector3 sphere_position = s.position_value - attribute_.position_value;
        math::vector3 sphere_last_position = s.last_position - attribute_.position_value;

        float position1 = math::dot(attribute_.normal_value, sphere_position);
        float position2 = math::dot(attribute_.normal_value, sphere_last_position);

        if ((position1 > 0.0f && position2 < 0.0f) || (position1 < 0.0f && position2 > 0.0f) || ((position1) <= s.radius && position1 >= -s.radius)) {
            math::vector3 quad_position1 = s.position_value - attribute_.quad_position[0];
            math::vector3 quad_position2 = s.position_value - attribute_.quad_position[3];

            if (math::dot(quad_position1, attribute_.inside_vector[0]) > -s.radius)
                if (math::dot(quad_position1, attribute_.inside_vector[1]) > -s.radius)
                    if (math::dot(quad_position2, attribute_.inside_vector[2]) > -s.radius)
                        if (math::dot(quad_position2, attribute_.inside_vector[3]) > -s.radius) {
                            return true;
                        }
        }

        return false;
    }

    auto plane_collision_instance::on_get_collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 {
        math::vector3 advance = s.position_value - s.last_position;

        float d = math::dot(attribute_.normal_value, advance);
        if (d >= -0.0001 && d <= 0.0001)
            return math::vector3(0.0f, 0.0f, 0.0f);
        const auto vv = (s.position_value - attribute_.position_value);
        float t = -math::dot(attribute_.normal_value, vv) / d;

        if (math::dot(attribute_.normal_value, advance) >= 0) {
            return t * advance - s.radius * attribute_.normal_value;
        } else {
            return t * advance + s.radius * attribute_.normal_value;
        }
    }

    void plane_collision_instance::render(uint32_t /*fill_mode*/) {
        static screen s;
        s.render_bar3d(attribute_.quad_position);
    }

    void plane_collision_instance::on_destroy() {
        pci.free(this);
    }

    auto cylinder_collision_instance::attribute() -> cylinder_data & {
        return attribute_;
    }

    auto cylinder_collision_instance::attribute() const -> const cylinder_data & {
        return attribute_;
    }

    auto cylinder_collision_instance::collide_cylinder_vs_dynamic_sphere(const cylinder_data &c_rattribute, const dynamic_sphere_instance &s) const -> bool {
        if (s.position_value.z + s.radius < c_rattribute.position_value.z)
            return false;

        if (s.position_value.z - s.radius > c_rattribute.position_value.z + c_rattribute.height)
            return false;

        math::vector3 oa, ob;
        intersect_line_segments(c_rattribute.position_value, math::vector3(c_rattribute.position_value.x, c_rattribute.position_value.y, c_rattribute.position_value.z + c_rattribute.height), s.last_position, s.position_value, oa, ob);
        const auto vv = (oa - ob);
        return (math::length_sq(vv) <= (c_rattribute.radius + s.radius) * (c_rattribute.radius + s.radius));
    }

    auto cylinder_collision_instance::on_movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {
        if (collide_cylinder_vs_dynamic_sphere(attribute_, s)) {
            // Only a movement towards the center collides, so a sphere already inside can leave.
            if (get_vector3_distance(s.position_value, attribute_.position_value) <
                get_vector3_distance(s.last_position, attribute_.position_value))
                return true;
        }

        // A large movement is stepped along the path one radius at a time and every step is tested.
        math::vector3 v3_distance = s.position_value - s.last_position;
        float f_distance = math::length(v3_distance);
        if (s.radius <= 0.0001f)
            return false;
        if (f_distance >= s.radius * 2.0f) {
            cylinder_data cylinder;
            cylinder = attribute_;
            cylinder.position_value = s.last_position;

            int i_step = f_distance / s.radius * 2.0f;
            math::vector3 v3_step = v3_distance / float(i_step);

            for (int i = 0; i < i_step; ++i) {
                cylinder.position_value += v3_step;
                if (collide_cylinder_vs_dynamic_sphere(cylinder, s))
                    return true;
            }
        }

        return false;
    }

    auto cylinder_collision_instance::on_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {

        return (collide_cylinder_vs_dynamic_sphere(attribute_, s));
    }

    auto cylinder_collision_instance::on_get_collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 {
        math::vector3 position = attribute_.position_value;
        position.z = s.position_value.z;
        const auto vv = (s.position_value - position);
        if (math::length_sq(vv) >= (s.radius + attribute_.radius) * (attribute_.radius + s.radius))
            return math::vector3(0.0f, 0.0f, 0.0f);
        math::vector3 c;
        math::vector3 advance = s.position_value - s.last_position;
        advance.z = 0;
        const auto vssa = math::vector3(0.0f, 0.0f, 1.0f);
        c = math::cross(advance, vssa);

        const auto svsvs = (s.position_value - position);
        float sum = -math::dot(c, svsvs);
        float mul = (s.radius + attribute_.radius) * (s.radius + attribute_.radius) - math::length_sq(svsvs);

        if (sum * sum - 4 * mul <= 0)
            return math::vector3(0.0f, 0.0f, 0.0f);
        float sq = sqrt(sum * sum - 4 * mul);
        float t1 = -sum - sq, t2 = -sum + sq;
        t1 *= 0.5f;
        t2 *= 0.5f;

        if (fabs(t1) <= fabs(t2)) {
            return (gc_reduce_move * t1) * c;
        } else
            return (gc_reduce_move * t2) * c;
    }

    void cylinder_collision_instance::render(uint32_t fill_mode) {
        static screen s;
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, 0xffffffff);
        s.render_cylinder(nullptr, attribute_.position_value.x, attribute_.position_value.y, attribute_.position_value.z + attribute_.height / 2, attribute_.radius, attribute_.height, fill_mode);
    }

    void cylinder_collision_instance::on_destroy() {
        cci.free(this);
    }

    auto aabb_collision_instance::attribute() -> aabb_data & {
        return attribute_;
    }

    auto aabb_collision_instance::attribute() const -> const aabb_data & {

        return attribute_;
    }

    auto aabb_collision_instance::on_movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {
        math::vector3 v;

        memcpy(&v, &s.position_value, sizeof(math::vector3));

        if (v.x < attribute_.min.x) v.x = attribute_.min.x;
        if (v.x > attribute_.max.x) v.x = attribute_.max.x;
        if (v.y < attribute_.min.y) v.x = attribute_.min.y;
        if (v.y > attribute_.max.y) v.x = attribute_.max.y;
        if (v.z < attribute_.min.z) v.z = attribute_.min.z;
        if (v.z > attribute_.max.z) v.z = attribute_.max.z;

        if (get_vector3_distance(v, s.position_value) <= s.radius * s.radius) {

            return true;
        }

        memcpy(&v, &s.last_position, sizeof(math::vector3));

        if (v.x < attribute_.min.x) v.x = attribute_.min.x;
        if (v.x > attribute_.max.x) v.x = attribute_.max.x;
        if (v.y < attribute_.min.y) v.x = attribute_.min.y;
        if (v.y > attribute_.max.y) v.x = attribute_.max.y;
        if (v.z < attribute_.min.z) v.z = attribute_.min.z;
        if (v.z > attribute_.max.z) v.z = attribute_.max.z;

        if (get_vector3_distance(v, s.last_position) <= s.radius * s.radius) {

            return true;
        }

        return false;
    }

    auto aabb_collision_instance::on_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {
        math::vector3 v;
        memcpy(&v, &s.position_value, sizeof(math::vector3));

        if (v.x < attribute_.min.x) v.x = attribute_.min.x;
        if (v.x > attribute_.max.x) v.x = attribute_.max.x;
        if (v.y < attribute_.min.y) v.x = attribute_.min.y;
        if (v.y > attribute_.max.y) v.x = attribute_.max.y;
        if (v.z < attribute_.min.z) v.z = attribute_.min.z;
        if (v.z > attribute_.max.z) v.z = attribute_.max.z;

        if (v.x > attribute_.min.x && v.x < attribute_.max.x &&
            v.y > attribute_.min.y && v.y < attribute_.max.y &&
            v.z > attribute_.min.z && v.z < attribute_.max.z) {
            return true;
        }

        if (get_vector3_distance(v, s.position_value) <= s.radius * s.radius) {
            return true;
        }

        memcpy(&v, &s.last_position, sizeof(math::vector3));

        if (v.x < attribute_.min.x) v.x = attribute_.min.x;
        if (v.x > attribute_.max.x) v.x = attribute_.max.x;
        if (v.y < attribute_.min.y) v.x = attribute_.min.y;
        if (v.y > attribute_.max.y) v.x = attribute_.max.y;
        if (v.z < attribute_.min.z) v.z = attribute_.min.z;
        if (v.z > attribute_.max.z) v.z = attribute_.max.z;

        if (v.x > attribute_.min.x && v.x < attribute_.max.x &&
            v.y > attribute_.min.y && v.y < attribute_.max.y &&
            v.z > attribute_.min.z && v.z < attribute_.max.z) {
            return true;
        }

        if (get_vector3_distance(v, s.last_position) <= s.radius * s.radius) {
            return true;
        }

        return false;
    }

    auto aabb_collision_instance::on_get_collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 {

        math::vector3 temp;
        if (s.position_value.x + s.radius <= attribute_.min.x) {
            temp.x = attribute_.min.x;
        } else if (s.position_value.x - s.radius >= attribute_.max.x) {
            temp.x = attribute_.max.x;
        } else if (s.position_value.x + s.radius >= attribute_.min.x && s.position_value.x + s.radius <= attribute_.max.x) {
            temp.x = s.position_value.x + s.radius;
        } else {
            temp.x = s.position_value.x - s.radius;
        }

        if (s.position_value.y + s.radius <= attribute_.min.y) {
            temp.y = attribute_.min.y;
        } else if (s.position_value.y - s.radius >= attribute_.max.y) {
            temp.y = attribute_.max.y;
        } else if (s.position_value.y + s.radius >= attribute_.min.y && s.position_value.y + s.radius <= attribute_.max.y) {
            temp.y = s.position_value.y + s.radius;
        } else {
            temp.y = s.position_value.y - s.radius;
        }

        if (s.position_value.z + s.radius <= attribute_.min.z) {
            temp.z = attribute_.min.z;
        } else if (s.position_value.z - s.radius >= attribute_.max.z) {
            temp.z = attribute_.max.z;
        } else if (s.position_value.z + s.radius >= attribute_.min.z && s.position_value.z + s.radius <= attribute_.max.z) {
            temp.z = s.position_value.z + s.radius;
        } else {
            temp.z = s.position_value.z - s.radius;
        }

        const auto vv = (temp - s.position_value);
        if (math::length_sq(vv) < s.radius * s.radius)
            return math::vector3(.0f, .0f, .0f);

        return math::vector3(.0f, .0f, .0f);
    }

    void aabb_collision_instance::render(uint32_t fill_mode) {
        static screen s;
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, 0xffffffff);
        s.render_cube(attribute_.min.x, attribute_.min.y, attribute_.min.z, attribute_.max.x, attribute_.max.y, attribute_.max.z);
        return;
    }

    void aabb_collision_instance::on_destroy() {
        aci.free(this);
    }

    auto obb_collision_instance::attribute() -> obb_data & {
        return attribute_;
    }

    auto obb_collision_instance::attribute() const -> const obb_data & {

        return attribute_;
    }

    auto obb_collision_instance::on_movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {
        math::vector3 center = 0.5f * (attribute_.min + attribute_.max);
        math::vector3 sphere = s.position_value - center;
        sphere = math::transform_coord(sphere, attribute_.rot);
        sphere = sphere + center;

        math::vector3 point = sphere;
        if (point.x < attribute_.min.x) {
            point.x = attribute_.min.x;
        }
        if (point.x > attribute_.max.x) {
            point.x = attribute_.max.x;
        }
        if (point.y < attribute_.min.y) {
            point.y = attribute_.min.y;
        }
        if (point.y > attribute_.max.y) {
            point.y = attribute_.max.y;
        }
        if (point.z < attribute_.min.z) {
            point.z = attribute_.min.z;
        }
        if (point.z > attribute_.max.z) {
            point.z = attribute_.max.z;
        }

        if (get_vector3_distance(point, sphere) <= s.radius * s.radius) {
            return true;
        }

        sphere = s.last_position - center;
        sphere = math::transform_coord(sphere, attribute_.rot);
        sphere = sphere + center;

        point = sphere;
        if (point.x < attribute_.min.x) {
            point.x = attribute_.min.x;
        }
        if (point.x > attribute_.max.x) {
            point.x = attribute_.max.x;
        }
        if (point.y < attribute_.min.y) {
            point.y = attribute_.min.y;
        }
        if (point.y > attribute_.max.y) {
            point.y = attribute_.max.y;
        }
        if (point.z < attribute_.min.z) {
            point.z = attribute_.min.z;
        }
        if (point.z > attribute_.max.z) {
            point.z = attribute_.max.z;
        }

        if (get_vector3_distance(point, sphere) <= s.radius * s.radius) {
            return true;
        }

        return false;
    }

    auto obb_collision_instance::on_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {

        math::vector3 center = 0.5f * (attribute_.min + attribute_.max);
        math::vector3 sphere = s.position_value - center;
        sphere = math::transform_coord(sphere, attribute_.rot);
        sphere = sphere + center;

        math::vector3 point = sphere;
        if (point.x < attribute_.min.x) {
            point.x = attribute_.min.x;
        }
        if (point.x > attribute_.max.x) {
            point.x = attribute_.max.x;
        }
        if (point.y < attribute_.min.y) {
            point.y = attribute_.min.y;
        }
        if (point.y > attribute_.max.y) {
            point.y = attribute_.max.y;
        }
        if (point.z < attribute_.min.z) {
            point.z = attribute_.min.z;
        }
        if (point.z > attribute_.max.z) {
            point.z = attribute_.max.z;
        }

        if (get_vector3_distance(point, sphere) <= s.radius * s.radius) {
            return true;
        }

        sphere = s.last_position - center;
        sphere = math::transform_coord(sphere, attribute_.rot);
        sphere = sphere + center;

        point = sphere;
        if (point.x < attribute_.min.x) {
            point.x = attribute_.min.x;
        }
        if (point.x > attribute_.max.x) {
            point.x = attribute_.max.x;
        }
        if (point.y < attribute_.min.y) {
            point.y = attribute_.min.y;
        }
        if (point.y > attribute_.max.y) {
            point.y = attribute_.max.y;
        }
        if (point.z < attribute_.min.z) {
            point.z = attribute_.min.z;
        }
        if (point.z > attribute_.max.z) {
            point.z = attribute_.max.z;
        }

        if (get_vector3_distance(point, sphere) <= s.radius * s.radius) {
            return true;
        }

        return false;
    }

    auto obb_collision_instance::on_get_collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 {

        return math::vector3(.0f, .0f, .0f);
    }

    void obb_collision_instance::render(uint32_t fill_mode) {
        static screen s;
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, 0xffffffff);
        s.render_cube(attribute_.min.x, attribute_.min.y, attribute_.min.z, attribute_.max.x, attribute_.max.y, attribute_.max.z, attribute_.rot);
        return;
    }

    void obb_collision_instance::on_destroy() {
        oci.free(this);
    }
}
