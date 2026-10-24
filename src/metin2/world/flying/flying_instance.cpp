#include <metin2/core/random.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/geometry/grp_math.h>

#include <metin2/world/maps/map_manager.h>

#include <metin2/core/crc32.h>
#include <metin2/core/timer.h>
#include "fly_handler.h"
#include "fly_target.h"
#include "fly_trace.h"
#include "flying_data.h"
#include "flying_instance.h"
#include "flying_object_manager.h"

#include <algorithm>

namespace metin2::inline world::inline flying {

    dynamic_pool<flying_instance> flying_instance::pool_value;

    flying_instance::flying_instance() {
        initialize_impl();
    }

    flying_instance::~flying_instance() {
        destroy();
    }

    void flying_instance::initialize_impl() {
        attach_rotation_ = rot_ = math::quaternion(0.0f, 0.0f, 0.0f, 0.0f);
        accel_ = local_velocity_ = velocity_ = position_value_ = math::vector3(0.0f, 0.0f, 0.0f);

        handler_ = nullptr;
        data_value_ = nullptr;
        owner_ = nullptr;

        alive_ = false;
        can_attack_ = false;

        skill_index_ = 0;

        pierce_count_ = 0;

        start_time_ = 0.0f;
        remain_range_ = 0.0f;

        target_hitted_ = false;
        hitted_object_set_.clear();
    }

    void flying_instance::clear() {
        destroy();
    }

    void flying_instance::destroy() {
        fly_target_value_.clear();

        clear_attach_instance();

        initialize_impl();
    }

    void flying_instance::build_attach_instance() {
        for (int i = 0; i < data_value_->attach_data_count(); i++) {
            flying_data::flying_attach_data &rfad = data_value_->attach_data_reference(i);

            switch (rfad.type_value) {
            case flying_data::FLY_ATTACH_OBJECT:
                assert(!"FlyingInstance.cpp:BuildAttachInstance Not implemented FLY_ATTACH_OBJECT");
                break;
            case flying_data::FLY_ATTACH_EFFECT:
                {
                    effect_manager &rem = get_effect_manager();
                    attach_effect_instance aei;

                    uint32_t crc = get_case_crc32(rfad.filename.c_str(), rfad.filename.size());

                    aei.attach_index = i;
                    aei.effect_instance_index = rem.empty_index();

                    aei.fly_trace_value = nullptr;
                    if (rfad.has_tail) {
                        aei.fly_trace_value = fly_trace::new_value();
                        aei.fly_trace_value->create(rfad);
                    }
                    rem.create_effect_instance(aei.effect_instance_index, crc);

                    attach_effect_instance_value_.push_back(aei);
                }
                break;
            }
        }
    }

    void flying_instance::create(flying_data *data, const math::vector3 &start_pos, const fly_target &target, bool can_attack) {
        fly_target_value_ = target;
        can_attack_ = can_attack;

        set_data_pointer_impl(data, start_pos);
        set_target_direction(fly_target_value_);
    }

    void flying_instance::set_target_direction(const fly_target &target) {
        math::vector3 target_pos = target.fly_target_position();

        if (data_value_->maintain_parallel_) {
            target_pos.z += 50.0f;
        }

        math::vector3 target_dir = target_pos - position_value_;

        target_dir = math::normalize(target_dir);
        set_target_normalized_direction(target_dir);
    }

    void flying_instance::set_target_normalized_direction(const math::vector3 &nomalized_direction) {
        math::quaternion q = safe_rotation_normalized_arc(math::vector3(0.0f, -1.0f, 0.0f), nomalized_direction);
        rot_ = rot_ * q;
        vec3_transform_quaternion(&velocity_, &local_velocity_, &rot_);
        vec3_transform_quaternion(&accel_, &data_value_->accel_, &rot_);
    }

    void flying_instance::set_fly_target(const fly_target &cr_target) {
        fly_target_value_ = cr_target;

        set_target_direction(fly_target_value_);
    }

    void flying_instance::adjust_direction_for_homing(const math::vector3 &target_position) {
        math::vector3 v_target_dir(target_position);
        v_target_dir -= position_value_;
        v_target_dir = math::normalize(v_target_dir);
        math::vector3 v_vel;
        v_vel = math::normalize(velocity_);

        const auto vv1 = (v_vel - v_target_dir);
        if (math::length_sq(vv1) < 0.001f)
            return;

        math::quaternion q = safe_rotation_normalized_arc(v_vel, v_target_dir);

        if (data_value_->homing_max_angle_ > 180) {
            vec3_transform_quaternion_safe(&velocity_, &velocity_, &q);
            vec3_transform_quaternion_safe(&accel_, &accel_, &q);
            rot_ = q * rot_;
            return;
        }

        // homing_max_angle_ is the turn limit per frame at 60 fps.
        const float max_angle = std::min(data_value_->homing_max_angle_ * get_timer().elapsed_second() * 60.0f, 180.0f);
        float c = cosf(math::to_radian(max_angle));
        float s = sinf(math::to_radian(max_angle));

        if (q.w <= -1.0f + 0.0001f) {
            q.x = 0;
            q.y = 0;
            q.z = s;
            q.w = c;
        } else if (q.w <= c && q.w <= 1.0f - 0.0001f) {
            float factor = s / sqrtf(1.0f - q.w * q.w);
            q.x *= factor;
            q.y *= factor;
            q.z *= factor;
            q.w = c;
        }
        vec3_transform_quaternion_safe(&velocity_, &velocity_, &q);
        vec3_transform_quaternion_safe(&accel_, &accel_, &q);
        rot_ = rot_ * q;
    }

    void flying_instance::update_attach_instance() {
        math::quaternion q;
        q = math::quaternion_rotation_yaw_pitch_roll(math::to_radian(data_value_->ang_vel_.y) * get_timer().elapsed_second(), math::to_radian(data_value_->ang_vel_.x) * get_timer().elapsed_second(), math::to_radian(data_value_->ang_vel_.z) * get_timer().elapsed_second());

        attach_rotation_ = attach_rotation_ * q;
        q = attach_rotation_ * rot_;

        effect_manager &rem = get_effect_manager();
        attach_effect_instance_vector::iterator it;
        for (it = attach_effect_instance_value_.begin(); it != attach_effect_instance_value_.end(); ++it) {
            flying_data::flying_attach_data &rfad = data_value_->attach_data_reference(it->attach_index);
            assert(rfad.type_value == flying_data::FLY_ATTACH_EFFECT);
            rem.select_effect_instance(it->effect_instance_index);
            math::matrix m;
            switch (rfad.fly_type) {
            case flying_data::FLY_ATTACH_TYPE_LINE:
                m = math::matrix_rotation_quaternion(rot_);
                m._41 = position_value_.x;
                m._42 = position_value_.y;
                m._43 = position_value_.z;
                break;
            case flying_data::FLY_ATTACH_TYPE_MULTI_LINE:
                {
                    math::vector3 p(
                        -sinf(math::to_radian(rfad.roll)) * rfad.distance,
                        0.0f,
                        -cosf(math::to_radian(rfad.roll)) * rfad.distance);
                    vec3_transform_quaternion_safe(&p, &p, &q);
                    p += position_value_;
                    m = math::matrix_rotation_quaternion(q);
                    m._41 = p.x;
                    m._42 = p.y;
                    m._43 = p.z;
                }
                break;
            case flying_data::FLY_ATTACH_TYPE_SINE:
                {
                    float angle = (get_timer().current_second() - start_time_) * 2 * 3.1415926535897931f / rfad.period;
                    math::vector3 p(
                        -sinf(math::to_radian(rfad.roll)) * rfad.amplitude * sinf(angle),
                        0.0f,
                        -cosf(math::to_radian(rfad.roll)) * rfad.amplitude * sinf(angle));
                    vec3_transform_quaternion_safe(&p, &p, &q);
                    p += position_value_;
                    m = math::matrix_rotation_quaternion(q);
                    m._41 = p.x;
                    m._42 = p.y;
                    m._43 = p.z;
                }
                break;
            case flying_data::FLY_ATTACH_TYPE_EXP:
                {
                    float dt = get_timer().current_second() - start_time_;
                    float angle = dt / rfad.period;
                    math::vector3 p(
                        -sinf(math::to_radian(rfad.roll)) * rfad.amplitude * exp(-angle) * angle,
                        0.0f,
                        -cosf(math::to_radian(rfad.roll)) * rfad.amplitude * exp(-angle) * angle);
                    vec3_transform_quaternion_safe(&p, &p, &q);
                    p += position_value_;
                    m = math::matrix_rotation_quaternion(q);
                    m._41 = p.x;
                    m._42 = p.y;
                    m._43 = p.z;
                }
                break;
            }
            rem.set_effect_instance_global_matrix(m);
            if (it->fly_trace_value)
                it->fly_trace_value->update_new_position(math::vector3(m._41, m._42, m._43));
        }
    }
    struct check_background_during_flying_fn {
        dynamic_sphere_instance s;
        bool hit;
        check_background_during_flying_fn(const math::vector3 &v1, const math::vector3 &v2) {
            s.radius = 1.0f;
            s.last_position = v1;
            s.position_value = v2;
            hit = false;
        }
        void operator()(graphic_object_instance *p) {
            if (!p)
                return;

            if (!hit && p->get_type() != ACTOR_OBJECT) {
                if (p->collision_dynamic_sphere(s)) {
                    hit = true;
                }
            }
        }
        auto is_hitted() -> bool {
            return hit;
        }
    };

    struct check_another_monster_during_flying_fn {
        dynamic_sphere_instance s;
        graphic_object_instance *inst;
        const actor_instance_interface *owner;
        check_another_monster_during_flying_fn(const actor_instance_interface *owner_value, const math::vector3 &v1, const math::vector3 &v2)
            : owner(owner_value) {
            s.radius = 10.0f;
            s.last_position = v1;
            s.position_value = v2;
            inst = nullptr;
        }
        void operator()(graphic_object_instance *p) {
            if (!p)
                return;

            if (!inst && p->get_type() == ACTOR_OBJECT) {
                auto *pa = (actor_instance_interface *)p;
                if (pa != owner && pa->test_collision_with_dynamic_sphere(s)) {
                    inst = p;
                }
            }
        }
        auto is_hitted() -> bool {
            return inst != nullptr;
        }
        auto hitted_object() -> graphic_object_instance * {
            return inst;
        }
    };

    auto flying_instance::update() -> bool {
        if (!alive_)
            return false;

        if (data_value_->is_homing_ &&
            data_value_->homing_start_time_ + start_time_ < get_timer().current_second()) {
            if (fly_target_value_.is_object())
                adjust_direction_for_homing(fly_target_value_.fly_target_position());
        }

        math::vector3 last_position = position_value_;

        velocity_ += accel_ * get_timer().elapsed_second();
        velocity_.z += data_value_->gravity_ * get_timer().elapsed_second();
        math::vector3 movement = velocity_ * get_timer().elapsed_second();
        float move_distance = math::length(movement);
        float collision_sphere_radius = std::max(move_distance * 2, data_value_->collision_sphere_radius_);
        remain_range_ -= move_distance;
        position_value_ += movement;

        update_attach_instance();

        if (remain_range_ < 0) {
            if (handler_)
                handler_->on_exploding_out_of_range();

            explode(false);
            return false;
        }

        if (fly_target_value_.is_object()) {
            if (!target_hitted_) {
                if (square_distance_between_linesegment_and_point(position_value_, last_position, fly_target_value_.fly_target_position()) < data_value_->bomb_range_ * data_value_->bomb_range_) {
                    target_hitted_ = true;

                    if (can_attack_) {
                        fly_targetable_object *victim = fly_target_value_.get_fly_target();
                        if (victim) {
                            victim->on_shoot_damage();
                        }
                    }

                    if (handler_) {
                        handler_->on_exploding_at_target(skill_index_);
                    }

                    if (pierce_count_) {
                        pierce_count_--;
                        bomb();
                    } else {
                        explode();
                        return false;
                    }

                    return true;
                }
            }
        } else if (fly_target_value_.is_position()) {
            if (square_distance_between_linesegment_and_point(position_value_, last_position, fly_target_value_.fly_target_position()) < data_value_->bomb_range_ * data_value_->bomb_range_) {
                explode();
                return false;
            }
        }

        vector3d start, dir;
        start.set(last_position.x, last_position.y, last_position.z);
        dir.set(movement.x, movement.y, movement.z);

        culling_manager &culling_mgr = get_culling_manager();

        if (data_value_->hit_on_another_monster_) {
            check_another_monster_during_flying_fn check_another_monster_during_flying(owner_, last_position, position_value_);
            culling_mgr.for_in_range(start, collision_sphere_radius, &check_another_monster_during_flying);
            if (check_another_monster_during_flying.is_hitted()) {
                auto *hitted_instance = (actor_instance_interface *)check_another_monster_during_flying.hitted_object();
                if (hitted_object_set_.end() == hitted_object_set_.find(hitted_instance)) {
                    hitted_object_set_.insert(hitted_instance);

                    if (handler_) {
                        handler_->on_exploding_at_another_target(skill_index_, hitted_instance->virtual_id());
                    }

                    if (pierce_count_) {
                        pierce_count_--;
                        bomb();
                    } else {
                        explode();
                        return false;
                    }

                    return true;
                }
            }
        }

        if (data_value_->hit_on_background_) {
            if (get_flying_manager().map_manager_ptr()) {
                float ground_height = get_flying_manager().map_manager_ptr()->terrain_height(position_value_.x, -position_value_.y);
                if (ground_height > position_value_.z) {
                    if (handler_)
                        handler_->on_exploding_at_background();

                    explode();
                    return false;
                }
            }

            check_background_during_flying_fn check_background_during_flying(last_position, position_value_);
            culling_mgr.for_in_range(start, collision_sphere_radius, &check_background_during_flying);

            if (check_background_during_flying.is_hitted()) {
                if (handler_)
                    handler_->on_exploding_at_background();

                explode();
                return false;
            }
        }

        return true;
    }

    void flying_instance::clear_attach_instance() {
        effect_manager &eft_mgr = get_effect_manager();

        attach_effect_instance_vector::iterator i;
        for (i = attach_effect_instance_value_.begin(); i != attach_effect_instance_value_.end(); ++i) {
            eft_mgr.destroy_effect_instance(i->effect_instance_index);

            if (i->fly_trace_value)
                fly_trace::delete_value(i->fly_trace_value);

            i->attach_index = 0;
            i->effect_instance_index = 0;
            i->fly_trace_value = nullptr;
        }
        attach_effect_instance_value_.clear();
    }

    void flying_instance::explode(bool bomb_value) {
        if (!alive_)
            return;

        alive_ = false;

        if (bomb_value)
            bomb();
    }

    void flying_instance::bomb() {
        effect_manager &eft_mgr = get_effect_manager();
        if (!data_value_->bomb_effect_id_)
            return;

        uint32_t empty_index = eft_mgr.empty_index();
        eft_mgr.create_effect_instance(empty_index, data_value_->bomb_effect_id_);

        math::matrix m;
        m = math::matrix_identity();
        m._41 = position_value_.x;
        m._42 = position_value_.y;
        m._43 = position_value_.z;
        eft_mgr.select_effect_instance(empty_index);
        eft_mgr.set_effect_instance_global_matrix(m);
    }

    void flying_instance::render() {
        if (!alive_)
            return;
        render_attach_instance();
    }

    void flying_instance::render_attach_instance() {
        attach_effect_instance_vector::iterator it;
        for (it = attach_effect_instance_value_.begin(); it != attach_effect_instance_value_.end(); ++it) {
            if (it->fly_trace_value)
                it->fly_trace_value->render();
        }
    }

    void flying_instance::set_data_pointer(flying_data *data, const math::vector3 &start_position) {
        set_data_pointer_impl(data, start_position);
    }

    void flying_instance::set_data_pointer_impl(flying_data *data, const math::vector3 &start_position) {
        data_value_ = data;
        rot_ = math::quaternion(0.0f, 0.0f, 0.0f, 1.0f),
        position_value_ = (start_position);
        alive_ = (true);

        start_time_ = get_timer().current_second();

        rot_ = math::quaternion_rotation_yaw_pitch_roll(math::to_radian(data->roll_angle_ - 90.0f), 0.0f, math::to_radian(data->cone_angle_));
        if (data->spreading_) {
            math::quaternion q1, q2;
            const auto vv1 = math::vector3(0.0f, 0.0f, 1.0f), vv2 = math::vector3(0.0f, -1.0f, 0.0f);
            q2 = math::quaternion_rotation_axis(vv1, (metin2::random::fnumber(-3.141592f / 3, +3.141592f / 3) + metin2::random::fnumber(-3.141592f / 3, +3.141592f / 3)) / 2);
            q1 = math::quaternion_rotation_axis(vv2, metin2::random::fnumber(0, 2 * 3.1415926535897931f));
            q1 = q2 * q1;
            rot_ = q1 * rot_;
        }
        velocity_ = local_velocity_ = math::vector3(0.0f, -data->init_vel_, 0.0f);
        accel_ = data->accel_;
        remain_range_ = data->range_;
        attach_rotation_ = math::quaternion(0.0f, 0.0f, 0.0f, 1.0f);

        build_attach_instance();
        update_attach_instance();

        pierce_count_ = data->pierce_count_;
    }
}
