#include <metin2/geometry/grp_math.h>
#include <metin2/log/log.h>
#include "game_util.h"

#include <metin2/core/timer.h>
#include "actor_instance.h"

namespace metin2::inline world::inline actors {

    void actor_instance::initialize_collision_data() {
        can_skip_collision_ = false;
    }

    void actor_instance::enable_skip_collision() {
        can_skip_collision_ = true;
    }

    void actor_instance::disable_skip_collision() {
        can_skip_collision_ = false;
    }

    auto actor_instance::can_skip_collision() -> bool {
        return can_skip_collision_;
    }

    void actor_instance::update_point_instance() {
        collision_point_instance_list_iterator itor;
        for (itor = defending_point_instance_list_.begin(); itor != defending_point_instance_list_.end(); ++itor)
            update_point_instance(&(*itor));
    }

    void actor_instance::update_point_instance(collision_point_instance *point_instance) {
        if (!point_instance) {
            assert(!"CActorInstance::UpdatePointInstance - pPointInstance is NULL");
            return;
        }

        math::matrix mat_bone;

        if (point_instance->is_attached) {
            if (point_instance->model_index >= lod_controller_vector_.size()) {
                return;
            }

            lod_controller *grn_lod_controller = lod_controller_vector_[point_instance->model_index];
            if (!grn_lod_controller) {
                return;
            }

            model_instance *model_instance = grn_lod_controller->get_model_instance();
            if (!model_instance) {
                return;
            }

            auto *pmat_bone = (math::matrix *)model_instance->bone_matrix_pointer(point_instance->bone_index);
            mat_bone = *(math::matrix *)model_instance->composite_bone_matrix_pointer(point_instance->bone_index);
            mat_bone._41 = pmat_bone->_41;
            mat_bone._42 = pmat_bone->_42;
            mat_bone._43 = pmat_bone->_43;
            mat_bone *= world_matrix_;
        } else {
            mat_bone = world_matrix_;
        }

        auto sit = point_instance->c_collision_data->sphere_data_vector.begin();
        auto dit = point_instance->sphere_instance_vector.begin();
        for (; sit != point_instance->c_collision_data->sphere_data_vector.end(); ++sit, ++dit) {
            const sphere_data &c = sit->attribute();

            math::matrix point;
            point = math::matrix_translation(c.position_value.x, c.position_value.y, c.position_value.z);
            point = point * mat_bone;

            dit->last_position = dit->position_value;
            dit->position_value.x = point._41;
            dit->position_value.y = point._42;
            dit->position_value.z = point._43;
        }
    }

    void actor_instance::update_advancing_point_instance() {
        // When riding, the rider has no movement of its own; it comes from the horse.
        math::vector3 movement = movement_;
        if (horse_)
            movement = horse_->movement_;

        if (horse_)
            horse_->update_advancing_point_instance();

        math::matrix point;
        math::matrix center;

        auto itor = body_point_instance_list_.begin();
        for (; itor != body_point_instance_list_.end(); ++itor) {
            collision_point_instance &instance = *itor;

            if (instance.is_attached) {
                if (instance.model_index >= lod_controller_vector_.size()) {
                    metin2::log::debug("Model {} has no LOD controller (only {} controllers)", instance.model_index, lod_controller_vector_.size());
                    continue;
                }

                lod_controller *grn_lod_controller = lod_controller_vector_[instance.model_index];
                if (!grn_lod_controller) {
                    metin2::log::debug("Model {}: LOD controller is missing", instance.model_index);
                    continue;
                }

                model_instance *model_instance = grn_lod_controller->get_model_instance();
                if (!model_instance) {
                    continue;
                }

                center = *(math::matrix *)model_instance->bone_matrix_pointer(instance.bone_index);
                center *= world_matrix_;
            } else {
                center = world_matrix_;
            }

            const race_types::collision_data *collision_data = instance.c_collision_data;
            if (collision_data) {
                for (uint32_t j = 0; j < collision_data->sphere_data_vector.size(); ++j) {
                    const sphere_data &c = collision_data->sphere_data_vector[j].attribute();
                    dynamic_sphere_instance &sphere_instance = instance.sphere_instance_vector[j];

                    point = math::matrix_translation(c.position_value.x, c.position_value.y, c.position_value.z);
                    point = point * center;

                    sphere_instance.last_position.x = point._41;
                    sphere_instance.last_position.y = point._42;
                    sphere_instance.last_position.z = point._43;
                    sphere_instance.position_value = sphere_instance.last_position;
                    sphere_instance.position_value += movement;
                }
            }
        }
    }

    auto actor_instance::check_collision_detection(const dynamic_sphere_instance_vector *attacking_sphere_vector, math::vector3 *position) -> bool {
        if (!attacking_sphere_vector) {
            assert(!"CActorInstance::CheckCollisionDetection - c_pAttackingSphereVector is NULL");
            return false;
        }

        collision_point_instance_list_iterator itor;
        for (itor = defending_point_instance_list_.begin(); itor != defending_point_instance_list_.end(); ++itor) {
            const dynamic_sphere_instance_vector *defending_sphere_vector = &(*itor).sphere_instance_vector;

            for (uint32_t i = 0; i < attacking_sphere_vector->size(); ++i)
                for (uint32_t j = 0; j < defending_sphere_vector->size(); ++j) {
                    const dynamic_sphere_instance &attacking_sphere = attacking_sphere_vector->at(i);
                    const dynamic_sphere_instance &defending_sphere = defending_sphere_vector->at(j);

                    if (detect_collision_dynamic_sphere_vs_dynamic_sphere(attacking_sphere, defending_sphere)) {
                        // FIXME: should compute the intersection of the two circles instead.
                        *position = (attacking_sphere.position_value + defending_sphere.position_value) / 2.0f;
                        return true;
                    }
                }
        }

        return false;
    }

    auto actor_instance::create_collision_instance_piece(uint32_t attaching_model_index, const race_types::attaching_data *attaching_data, collision_point_instance *point_instance) -> bool {
        if (!attaching_data) {
            assert(!"CActorInstance::CreateCollisionInstancePiece - c_pAttachingData is NULL");
            return false;
        }

        if (!attaching_data->collision_data_value) {
            assert(!"CActorInstance::CreateCollisionInstancePiece - c_pAttachingData->pCollisionData is NULL");
            return false;
        }

        if (!point_instance) {
            assert(!"CActorInstance::CreateCollisionInstancePiece - pPointInstance is NULL");
            return false;
        }

        point_instance->model_index = attaching_model_index;
        point_instance->is_attached = false;
        point_instance->bone_index = 0;
        point_instance->c_collision_data = attaching_data->collision_data_value;

        if (attaching_data->is_attaching) {
            int attaching_bone_index;

            model_instance *model_instance = lod_controller_vector_[attaching_model_index]->get_model_instance();

            if (model_instance && model_instance->bone_index_by_name(attaching_data->attaching_bone_name.c_str(),
                                      &attaching_bone_index)) {
                point_instance->is_attached = true;
                point_instance->bone_index = attaching_bone_index;
            } else {
                point_instance->is_attached = true;
                point_instance->bone_index = 0;
            }
        }

        const sphere_collision_instance_vector &sphere_data_vector = attaching_data->collision_data_value->sphere_data_vector;

        point_instance->sphere_instance_vector.clear();
        point_instance->sphere_instance_vector.reserve(sphere_data_vector.size());

        sphere_collision_instance_vector::const_iterator it;
        dynamic_sphere_instance dsi;

        dsi.last_position = math::vector3(0.0f, 0.0f, 0.0f);
        dsi.position_value = math::vector3(0.0f, 0.0f, 0.0f);
        for (it = sphere_data_vector.begin(); it != sphere_data_vector.end(); ++it) {
            const sphere_data &sphere_data = it->attribute();
            dsi.radius = sphere_data.radius;
            point_instance->sphere_instance_vector.push_back(dsi);
        }

        return true;
    }

    auto actor_instance::splash_attack_process(actor_instance &victim) -> bool {
        math::vector3 v3_distance(victim.x_ - x_, victim.y_ - y_, victim.z_ - z_);
        float f_distance = math::length_sq(v3_distance);
        if (f_distance >= 1000.0f * 1000.0f)
            return false;

        if (!is_in_splash_time())
            return false;

        const race_motion_data::motion_attacking_event_data *attacking_event = splash_area_value_.c_attacking_event;
        const race_types::attack_data &attack_data = attacking_event->attack_data_value;
        hitted_instance_map &hitted_instance_map_value = splash_area_value_.hitted_instance_map_value;

        // An actor that was already hit cannot be hit again.
        if (hitted_instance_map_value.end() != hitted_instance_map_value.find(&victim)) {
            return false;
        }

        // In snipe mode only the fly target can be hit.
        if (race_types::ATTACK_TYPE_SNIPE == attack_data.attack_type_value) {
            if (is_fly_target_pc())
                if (!is_same_fly_target(&victim))
                    return false;
        }

        math::vector3 hit_position;
        if (victim.check_collision_detection(&splash_area_value_.sphere_instance_vector, &hit_position)) {
            hitted_instance_map_value.insert(std::make_pair(&victim, local_time() + attack_data.invisible_time));

            int current_hit_count = hitted_instance_map_value.size();
            int max_hit_count = (0 == attack_data.hit_limit_count ? 16 : attack_data.hit_limit_count);

            if (current_hit_count > max_hit_count) {
                return false;
            }

            new_set_atk_pixel_position(new_get_cur_pixel_position_ref());
            process_data_attack_success(attack_data, victim, hit_position, splash_area_value_.skill, splash_area_value_.is_enable_hit_process);
            return true;
        }

        return false;
    }

    auto actor_instance::normal_attack_process(actor_instance &victim) -> bool {
        // Only melee range is checked for now.
        math::vector3 v3_distance(victim.x_ - x_, victim.y_ - y_, victim.z_ - z_);
        float f_distance = math::length_sq(v3_distance);

        extern auto is_huge_race(unsigned int vnum) -> bool;
        if (is_huge_race(victim.race())) {
            if (f_distance >= 500.0f * 500.0f)
                return false;
        } else {
            if (f_distance >= 300.0f * 300.0f)
                return false;
        }

        if (!is_valid_attacking())
            return false;

        const float attack_radius = 20.0f;
        const race_types::motion_attack_data *pad = cur_race_motion_data_->motion_attack_data_pointer();

        const float motiontime = attacking_elapsed_time();

        auto itor_hit_data = pad->hit_data_container_value.begin();
        for (; itor_hit_data != pad->hit_data_container_value.end(); ++itor_hit_data) {
            const race_types::hit_data &hit_data = *itor_hit_data;

            auto it_hit_data = hit_data_map_value_.find(&hit_data);
            if (it_hit_data != hit_data_map_value_.end()) {
                hitted_instance_map &r_hitted_instance_map = it_hit_data->second;

                hitted_instance_map::iterator it_instance;
                if ((it_instance = r_hitted_instance_map.find(&victim)) != r_hitted_instance_map.end()) {
                    if (pad->motion_type_value == race_types::MOTION_TYPE_COMBO || it_instance->second > local_time())
                        continue;
                }
            }

            race_types::hit_time_position_map::const_iterator range_start, range_end;
            range_start = hit_data.hit_position.lower_bound(motiontime - get_timer().elapsed_second());
            range_end = hit_data.hit_position.upper_bound(motiontime);
            float c = cosf(math::to_radian(rotation()));
            float s = sinf(math::to_radian(rotation()));

            for (; range_start != range_end; ++range_start) {
                const dynamic_sphere_instance &dsi_src = range_start->second;

                dynamic_sphere_instance dsi;
                dsi = dsi_src;
                dsi.radius = attack_radius;
                {
                    math::vector3 src_dir = dsi_src.position_value - dsi_src.last_position;
                    src_dir *= reach_scale();

                    const math::vector3 &src = dsi_src.last_position + src_dir;
                    math::vector3 &dst = dsi.position_value;
                    dst.x = src.x * c - src.y * s;
                    dst.y = src.x * s + src.y * c;
                    dst += get_position();
                }
                {
                    const math::vector3 &src = dsi_src.last_position;
                    math::vector3 &dst = dsi.last_position;
                    dst.x = src.x * c - src.y * s;
                    dst.y = src.x * s + src.y * c;
                    dst += get_position();
                }

                collision_point_instance_list::iterator cpit;
                for (cpit = victim.defending_point_instance_list_.begin(); cpit != victim.defending_point_instance_list_.end(); ++cpit) {
                    const dynamic_sphere_instance_vector &defending_sphere_vector = cpit->sphere_instance_vector;
                    dynamic_sphere_instance_vector::const_iterator dsit;
                    for (dsit = defending_sphere_vector.begin(); dsit != defending_sphere_vector.end(); ++dsit) {
                        const dynamic_sphere_instance &sub = *dsit;
                        if (detect_collision_dynamic_z_cylinder_vs_dynamic_z_cylinder(dsi, sub)) {
                            auto it_hit_data = hit_data_map_value_.find(&hit_data);
                            if (it_hit_data == hit_data_map_value_.end()) {
                                hitted_instance_map hitted_instance_map_value;
                                hitted_instance_map_value.insert(std::make_pair(&victim, local_time() + pad->invisible_time));
                                hit_data_map_value_.insert(std::make_pair(&hit_data, hitted_instance_map_value));

                            } else {
                                it_hit_data->second.insert(std::make_pair(&victim, local_time() + pad->invisible_time));

                                int current_hit_count = it_hit_data->second.size();
                                // A normal attack hits at most 16 actors.
                                if (race_types::MOTION_TYPE_COMBO == pad->motion_type_value || race_types::MOTION_TYPE_NORMAL == pad->motion_type_value) {
                                    if (current_hit_count > 16) {
                                        return false;
                                    }
                                } else {
                                    if (current_hit_count > pad->hit_limit_count) {
                                        return false;
                                    }
                                }
                            }

                            math::vector3 hit_position = (get_position() + victim.get_position()) * 0.5f;

                            extern auto is_huge_race(unsigned int vnum) -> bool;
                            if (is_huge_race(victim.race())) {
                                hit_position = (get_position() + sub.position_value) * 0.5f;
                            }

                            process_data_attack_success(*pad, victim, hit_position, cur_mot_node_.skill);
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    auto actor_instance::attacking_process(actor_instance &victim) -> bool {
        if (victim.is_invisible())
            return false;

        if (splash_attack_process(victim))
            return true;

        if (normal_attack_process(victim))
            return true;

        return false;
    }

    auto actor_instance::test_physics_blending_collision(actor_instance &victim) -> bool {
        if (victim.is_dead())
            return false;

        pixel_position p_pos_last;
        blending_position(&p_pos_last);

        math::vector3 v3_distance = math::vector3(victim.x_ - p_pos_last.x, victim.y_ - p_pos_last.y, victim.z_ - p_pos_last.z);
        float f_distance = math::length_sq(v3_distance);
        if (f_distance > 800.0f * 800.0f)
            return false;

        // While attacking or waiting, collision is checked against the defending sphere;
        // otherwise actors slipped through each other while blending into the wait motion.
        collision_point_instance_list *main_list;
        collision_point_instance_list *victim_list;
        if (is_attacking() || is_waiting()) {
            main_list = &defending_point_instance_list_;
            victim_list = &victim.defending_point_instance_list_;
        } else {
            main_list = &body_point_instance_list_;
            victim_list = &victim.body_point_instance_list_;
        }

        pixel_position p_delta;
        physics_object_value_.last_position(&p_delta);

        math::vector3 prev_last_position, prev_position;
        const int sub_check_count = 50;

        auto itor_main = main_list->begin();
        auto itor_victim = victim_list->begin();
        for (; itor_main != main_list->end(); ++itor_main) {
            for (; itor_victim != victim_list->end(); ++itor_victim) {
                dynamic_sphere_instance_vector &main_sphere_vector = (*itor_main).sphere_instance_vector;
                dynamic_sphere_instance_vector &victim_sphere_vector = (*itor_victim).sphere_instance_vector;

                for (uint32_t i = 0; i < main_sphere_vector.size(); ++i) {
                    dynamic_sphere_instance &main_sphere = main_sphere_vector[i];
                    prev_last_position = main_sphere.last_position;
                    prev_position = main_sphere.position_value;

                    main_sphere.last_position = prev_position;

                    for (int i = 1; i <= sub_check_count; ++i) {
                        main_sphere.position_value = prev_position + (float)(i / (float)sub_check_count) * p_delta;

                        for (uint32_t j = 0; j < victim_sphere_vector.size(); ++j) {
                            dynamic_sphere_instance &victim_sphere = victim_sphere_vector[j];

                            if (detect_collision_dynamic_sphere_vs_dynamic_sphere(main_sphere, victim_sphere)) {
                                bool result = get_vector3_distance(main_sphere.position_value, victim_sphere.position_value) <= get_vector3_distance(main_sphere.last_position, victim_sphere.position_value);

                                main_sphere.last_position = prev_last_position;
                                main_sphere.position_value = prev_position;

                                return result;
                            }
                        }
                    }

                    main_sphere.last_position = prev_last_position;
                    main_sphere.position_value = prev_position;
                }
            }
        }

        return false;
    }

    auto actor_instance::test_actor_collision(actor_instance &victim) -> bool {

        if (victim.is_dead())
            return false;

        // Skip the check when reasonably far away. This can fail on frame skips or with very large
        // targets; it checks whether the character moved more than its body sphere radius and
        // otherwise filters by distance.
        math::vector3 v3_distance = math::vector3(victim.x_ - x_, victim.y_ - y_, victim.z_ - z_);
        float f_distance = math::length_sq(v3_distance);
        if (f_distance > 800.0f * 800.0f)
            return false;

        // While attacking or waiting, collision is checked against the defending sphere;
        // otherwise actors slipped through each other while blending into the wait motion.
        collision_point_instance_list *main_list;
        collision_point_instance_list *victim_list;
        if (is_attacking() || is_waiting()) {
            main_list = &defending_point_instance_list_;
            victim_list = &victim.defending_point_instance_list_;
        } else {
            main_list = &body_point_instance_list_;
            victim_list = &victim.body_point_instance_list_;
        }

        auto itor_main = main_list->begin();
        auto itor_victim = victim_list->begin();
        for (; itor_main != main_list->end(); ++itor_main)
            for (; itor_victim != victim_list->end(); ++itor_victim) {
                const dynamic_sphere_instance_vector &main_sphere_vector = (*itor_main).sphere_instance_vector;
                const dynamic_sphere_instance_vector &victim_sphere_vector = (*itor_victim).sphere_instance_vector;

                for (uint32_t i = 0; i < main_sphere_vector.size(); ++i)
                    for (uint32_t j = 0; j < victim_sphere_vector.size(); ++j) {
                        const dynamic_sphere_instance &main_sphere = main_sphere_vector[i];
                        const dynamic_sphere_instance &victim_sphere = victim_sphere_vector[j];

                        if (detect_collision_dynamic_sphere_vs_dynamic_sphere(main_sphere, victim_sphere)) {
                            if (get_vector3_distance(main_sphere.position_value, victim_sphere.position_value) <=
                                get_vector3_distance(main_sphere.last_position, victim_sphere.position_value)) {
                                return true;
                            }
                            return false;
                        }
                    }
            }

        return false;
    }

    auto actor_instance::avoid_object(const graphic_object_instance &bg_obj) -> bool {

        if (this == &bg_obj)
            return false;

        if (!test_object_collision(&bg_obj))
            return false;

        adjust_collision_movement(&bg_obj);
        return true;
    }

    auto actor_instance::is_block_object(const graphic_object_instance &bg_obj) -> bool {
        if (this == &bg_obj)
            return false;

        if (!test_object_collision(&bg_obj))
            return false;

        return true;
    }

    void actor_instance::block_movement() {
        if (horse_) {
            horse_->initialize_movement();
            return;
        }

        initialize_movement();
    }

    auto actor_instance::test_object_collision(const graphic_object_instance *object_instance) -> bool {
        if (horse_) {
            if (horse_->test_object_collision(object_instance))
                return true;

            return false;
        }

        if (can_skip_collision_)
            return false;

        if (movement_.x == 0.0f && movement_.y == 0.0f && movement_.z == 0.0f)
            return false;

        auto itor_main = body_point_instance_list_.begin();
        for (; itor_main != body_point_instance_list_.end(); ++itor_main) {
            const dynamic_sphere_instance_vector &main_sphere_vector = (*itor_main).sphere_instance_vector;
            for (uint32_t i = 0; i < main_sphere_vector.size(); ++i) {
                const dynamic_sphere_instance &main_sphere = main_sphere_vector[i];

                if (object_instance->movement_collision_dynamic_sphere(main_sphere)) {
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    auto actor_instance::test_collision_with_dynamic_sphere(const dynamic_sphere_instance &dsi) -> bool {
        auto itor_main = body_point_instance_list_.begin();
        for (; itor_main != body_point_instance_list_.end(); ++itor_main) {
            const dynamic_sphere_instance_vector &main_sphere_vector = (*itor_main).sphere_instance_vector;
            for (uint32_t i = 0; i < main_sphere_vector.size(); ++i) {
                const dynamic_sphere_instance &main_sphere = main_sphere_vector[i];

                if (detect_collision_dynamic_sphere_vs_dynamic_sphere(main_sphere, dsi)) {
                    return true;
                }
            }
        }

        return false;
    }
}
