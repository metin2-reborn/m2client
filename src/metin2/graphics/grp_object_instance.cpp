#include "grp_object_instance.h"
#include <metin2/core/timer.h>

namespace metin2::inline graphics {

    void graphic_object_instance::on_initialize() {
        memset(portal_id_, 0, sizeof(portal_id_));
    }

    void graphic_object_instance::clear() {
        if (culling_handle_value_) {
            get_culling_manager().unregister(culling_handle_value_);
            culling_handle_value_ = nullptr;
        }

        clear_height_instance();

        is_visible_ = true;

        position_value_.x = position_value_.y = position_value_.z = 0.0f;
        scale_.x = scale_.y = scale_.z = 0.0f;
        yaw_ = pitch_ = roll_ = 0.0f;
        world_matrix_ = math::matrix_identity();

        memset(portal_id_, 0, sizeof(portal_id_));

        on_clear();
    }

    auto graphic_object_instance::render() -> bool {
        if (!is_show())
            return false;

        on_render();
        return true;
    }

    void graphic_object_instance::blend_render() {
        if (!is_show())
            return;

        on_blend_render();
    }

    void graphic_object_instance::render_to_shadow_map() {
        if (!is_show())
            return;

        on_render_to_shadow_map();
    }

    void graphic_object_instance::render_shadow() {
        if (!is_show())
            return;

        on_render_shadow();
    }

    void graphic_object_instance::render_pc_blocker() {
        if (!is_show())
            return;

        on_render_pc_blocker();
    }

    void graphic_object_instance::update() {
        on_update();

        update_bounding_sphere();
    }

    void graphic_object_instance::deform() {
        if (!is_show())
            return;

        on_deform();
    }

    void graphic_object_instance::transform() {
        world_matrix_ = m_rotation_;

        world_matrix_._41 += position_value_.x;
        world_matrix_._42 += position_value_.y;
        world_matrix_._43 += position_value_.z;
    }

    auto graphic_object_instance::get_position() const -> const math::vector3 & {
        return position_value_;
    }

    auto graphic_object_instance::rotation() -> float {
        return get_roll();
    }

    auto graphic_object_instance::yaw() -> float {
        return yaw_;
    }

    auto graphic_object_instance::get_pitch() -> float {
        return pitch_;
    }

    auto graphic_object_instance::get_roll() -> float {
        return roll_;
    }

    auto graphic_object_instance::get_transform() -> math::matrix & {
        return world_matrix_;
    }

    void graphic_object_instance::set_rotation_quaternion(const math::quaternion &q) {
        m_rotation_ = math::matrix_rotation_quaternion(q);
    }

    void graphic_object_instance::set_rotation(float rotation) {
        yaw_ = 0;
        pitch_ = 0;
        roll_ = rotation;

        m_rotation_ = math::matrix_rotation_z(math::to_radian(rotation));
    }

    void graphic_object_instance::set_rotation(float yaw, float pitch, float roll) {
        yaw_ = yaw;
        pitch_ = pitch;
        roll_ = roll;

        m_rotation_ = math::matrix_rotation_yaw_pitch_roll(math::to_radian(yaw), math::to_radian(pitch), math::to_radian(roll));
    }

    void graphic_object_instance::set_position(float x, float y, float z) {
        position_value_.x = x;
        position_value_.y = y;
        position_value_.z = z;
    }

    void graphic_object_instance::set_position(const math::vector3 &newposition) {
        position_value_ = newposition;
    }

    void graphic_object_instance::set_scale(float x, float y, float z) {
        scale_.x = x;
        scale_.y = y;
        scale_.z = z;
    }

    void graphic_object_instance::show() {
        is_visible_ = true;
    }

    void graphic_object_instance::hide() {
        is_visible_ = false;
    }
    auto graphic_object_instance::is_show() -> bool {
        return is_visible_;
    }

    graphic_object_instance::graphic_object_instance() {
        culling_handle_value_ = nullptr;
        initialize();
    }

    void graphic_object_instance::initialize() {
        if (culling_handle_value_)
            get_culling_manager().unregister(culling_handle_value_);
        culling_handle_value_ = nullptr;

        height_attribute_instance_ = nullptr;

        is_visible_ = true;

        block_camera_ = false;

        position_value_.x = position_value_.y = position_value_.z = 0.0f;
        scale_.x = scale_.y = scale_.z = 0.0f;
        yaw_ = pitch_ = roll_ = 0.0f;

        world_matrix_ = math::matrix_identity();
        m_rotation_ = math::matrix_identity();

        on_initialize();
    }

    graphic_object_instance::~graphic_object_instance() {
        initialize();
    }

    void graphic_object_instance::update_bounding_sphere() {
        if (culling_handle_value_) {
            vector3d center;
            float radius;
            bounding_sphere(center, radius);
            if (radius != culling_handle_value_->radius())
                culling_handle_value_->new_pos_radius(center, radius);
            else
                culling_handle_value_->new_pos(center);
        }
    }

    void graphic_object_instance::register_bounding_sphere() {
        if (culling_handle_value_)
            get_culling_manager().unregister(culling_handle_value_);

        culling_handle_value_ = get_culling_manager().register_value(this);
    }

    void graphic_object_instance::add_collision(const static_collision_data *pscd, const math::matrix *mat) {
        static_collision_instance_vector_.push_back(base_collision_instance::build_collision_instance(pscd, mat));
    }

    void graphic_object_instance::clear_collision() {
        collision_instance_vector::iterator it;
        for (it = static_collision_instance_vector_.begin(); it != static_collision_instance_vector_.end(); ++it) {
            (*it)->destroy();
        }
        static_collision_instance_vector_.clear();
    }

    auto graphic_object_instance::collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {
        collision_instance_vector::const_iterator it;
        for (it = static_collision_instance_vector_.begin(); it != static_collision_instance_vector_.end(); ++it) {
            if ((*it)->collision_dynamic_sphere(s))
                return true;
        }
        return false;
    }

    auto graphic_object_instance::movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {
        collision_instance_vector::const_iterator it;
        for (it = static_collision_instance_vector_.begin(); it != static_collision_instance_vector_.end(); ++it) {
            if ((*it)->movement_collision_dynamic_sphere(s))
                return true;
        }
        return false;
    }

    auto graphic_object_instance::collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 {
        collision_instance_vector::const_iterator it;
        for (it = static_collision_instance_vector_.begin(); it != static_collision_instance_vector_.end(); ++it) {
            if ((*it)->movement_collision_dynamic_sphere(s))
                return (*it)->collision_movement_adjust(s);
        }

        return math::vector3(0.0f, 0.0f, 0.0f);
    }

    void graphic_object_instance::update_collision_data(const static_collision_data_vector *pscd_vector) {
        clear_collision();
        on_update_collision_data(pscd_vector);
    }

    auto graphic_object_instance::collision_instance_count() -> uint32_t {
        return static_collision_instance_vector_.size();
    }

    auto graphic_object_instance::collision_instance_data(uint32_t index) -> base_collision_instance * {
        if (index > static_collision_instance_vector_.size()) {
            return nullptr;
        }
        return static_collision_instance_vector_[index];
    }

    void graphic_object_instance::set_height_instance(attribute_instance *attribute_instance) {
        height_attribute_instance_ = attribute_instance;
    }

    void graphic_object_instance::clear_height_instance() {
        height_attribute_instance_ = nullptr;
    }

    void graphic_object_instance::update_height_instance(attribute_instance *attribute_instance) {
        clear_height_instance();
        on_update_heigh_instance(attribute_instance);
    }

    auto graphic_object_instance::is_object_height() -> bool {
        if (height_attribute_instance_)
            return true;

        return false;
    }

    auto graphic_object_instance::object_height(float x, float y, float *height) -> bool {
        if (!height_attribute_instance_)
            return false;

        return on_get_object_height(x, y, height);
    }

    void graphic_object_instance::set_portal(uint32_t index, int id) {
        if (index >= PORTAL_ID_MAX_NUM) {
            assert(index < PORTAL_ID_MAX_NUM);
            return;
        }

        portal_id_[index] = id;
    }

    auto graphic_object_instance::portal(uint32_t index) -> int {
        if (index >= PORTAL_ID_MAX_NUM) {
            assert(index < PORTAL_ID_MAX_NUM);
            return 0;
        }

        return portal_id_[index];
    }
}
