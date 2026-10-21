#include <metin2/effects/effect_manager.h>
#include <metin2/log/log.h>
#include <metin2/sound/sound_manager.h>

#include <metin2/world/flying/flying_instance.h>
#include <metin2/world/flying/flying_object_manager.h>
#include "actor_instance.h"
#include "game_event_manager.h"

#include <metin2/world/flying/fly_handler.h>

namespace metin2::inline world::inline actors {

    void actor_instance::motion_event_process() {
        if (!cur_race_motion_data_)
            return;

        for (uint32_t i = 0; i < cur_race_motion_data_->motion_event_data_count(); ++i) {
            const race_motion_data::motion_event_data *data;
            if (!cur_race_motion_data_->motion_event_data_pointer(i, &data))
                continue;

            motion_event_process(cur_mot_node_.dwcur_frame, i, data);
        }
    }

    void actor_instance::sound_event_process(bool check_frequency) {
        if (!cur_race_motion_data_)
            return;

        sound_manager &snd_mgr = get_sound_manager();
        const sound_types::sound_instance_vector *vct_snd_inst = cur_race_motion_data_->sound_instance_vector_pointer();
        snd_mgr.update_sound_instance(x_, y_, z_, cur_mot_node_.dwcur_frame, vct_snd_inst, check_frequency);
    }

    void actor_instance::motion_event_process(uint32_t dwcur_frame, int index, const race_motion_data::motion_event_data *data) {
        if (data->frame != dwcur_frame)
            return;

        switch (data->type_value) {
        case race_motion_data::MOTION_EVENT_TYPE_EFFECT:
            process_motion_event_effect_event(data);
            break;

        case race_motion_data::MOTION_EVENT_TYPE_EFFECT_TO_TARGET:
            process_motion_event_effect_to_target_event(data);
            break;

        case race_motion_data::MOTION_EVENT_TYPE_SCREEN_WAVING:
            get_game_event_manager().process_event_screen_waving(this, (const race_motion_data::screen_waving_event_data *)data);
            break;

        case race_motion_data::MOTION_EVENT_TYPE_SPECIAL_ATTACKING:
            process_motion_event_special_attacking(index, data);
            break;

        case race_motion_data::MOTION_EVENT_TYPE_SOUND:
            process_motion_event_sound(data);
            break;

        case race_motion_data::MOTION_EVENT_TYPE_FLY:
            process_motion_event_fly(data);
            break;

        case race_motion_data::MOTION_EVENT_TYPE_CHARACTER_SHOW:
            show_event();
            break;

        case race_motion_data::MOTION_EVENT_TYPE_CHARACTER_HIDE:
            hide_event();
            break;

        case race_motion_data::MOTION_EVENT_TYPE_WARP:
            process_motion_event_warp(data);
            break;
        }
    }

    void actor_instance::show_event() {
        is_hiding_ = false;
        restore_render_mode();
        set_alpha_value(1.0f);
    }

    void actor_instance::hide_event() {
        is_hiding_ = true;
        set_blend_render_mode();
        set_alpha_value(0.0f);
    }

    auto actor_instance::is_hiding() -> bool {
        return is_hiding_;
    }

    void actor_instance::process_motion_event_effect_event(const race_motion_data::motion_event_data *data) {
        if (race_motion_data::MOTION_EVENT_TYPE_EFFECT != data->type_value)
            return;

        const auto *effect_data = (const race_motion_data::motion_effect_event_data *)data;

        if (effect_data->is_independent) {
            int index = get_effect_manager().create_effect(effect_data->effect_index, math::vector3(0.0f, 0.0f, 0.0f), math::vector3(0.0f, 0.0f, 0.0f));

            math::matrix local_position;
            local_position = math::matrix_translation(effect_data->effect_position_value.x, effect_data->effect_position_value.y, effect_data->effect_position_value.z);

            math::matrix world;
            world = local_position;
            world *= world_matrix_;

            get_effect_manager().select_effect_instance(index);
            get_effect_manager().set_effect_instance_global_matrix(world);
            return;
        }

        if (effect_data->is_attaching) {
            if (effect_data->is_following) {
                attach_effect_by_id(0,
                    effect_data->attaching_bone_name.c_str(),
                    effect_data->effect_index,
                    &effect_data->effect_position_value);
            } else {
                int bone_index;
                uint32_t part_index = 0;
                if (find_bone_index(part_index, effect_data->attaching_bone_name.c_str(), &bone_index)) {
                    math::matrix *bone_mat;
                    bone_matrix(part_index, bone_index, &bone_mat);

                    math::matrix local_position;
                    local_position = math::matrix_translation(effect_data->effect_position_value.x, effect_data->effect_position_value.y, effect_data->effect_position_value.z);

                    math::matrix world;
                    world = *bone_mat;
                    world *= local_position;
                    world *= world_matrix_;

                    int index = get_effect_manager().create_effect(effect_data->effect_index,
                        effect_data->effect_position_value,
                        math::vector3(0.0f, 0.0f, 0.0f));
                    get_effect_manager().select_effect_instance(index);
                    get_effect_manager().set_effect_instance_global_matrix(world);
                }
            }
        } else {
            attach_effect_by_id(0, nullptr, effect_data->effect_index, &effect_data->effect_position_value);
        }
    }

    void actor_instance::process_motion_event_effect_to_target_event(const race_motion_data::motion_event_data *data) {
        if (race_motion_data::MOTION_EVENT_TYPE_EFFECT_TO_TARGET != data->type_value)
            return;

        const auto *effect_to_target_data = (const race_motion_data::motion_effect_to_target_event_data *)data;

        if (effect_to_target_data->is_fishing_effect) {
            effect_manager &eft_mgr = get_effect_manager();

            if (-1 != fishing_effect_id_) {
                eft_mgr.deactive_effect_instance(fishing_effect_id_);
            }

            fishing_effect_id_ = eft_mgr.create_effect(effect_to_target_data->effect_index, fishing_position_, math::vector3(0.0f, 0.0f, 0.0f));
        } else {
            if (!fly_target_value_.is_valid_target())
                return;

            if (effect_to_target_data->is_following && is_fly_target_object()) {
                auto *target_instance = (actor_instance *)fly_target_value_.get_fly_target();
                math::vector3 position(effect_to_target_data->effect_position_value.x,
                    effect_to_target_data->effect_position_value.y,
                    effect_to_target_data->effect_position_value.z);
                target_instance->attach_effect_by_id(0, nullptr, effect_to_target_data->effect_index, &position);
            } else {
                const math::vector3 &fly_target = fly_target_value_.fly_target_position();
                math::vector3 position(fly_target.x + effect_to_target_data->effect_position_value.x,
                    fly_target.y + effect_to_target_data->effect_position_value.y,
                    fly_target.z + effect_to_target_data->effect_position_value.z);
                get_effect_manager().create_effect(effect_to_target_data->effect_index, position, math::vector3(0.0f, 0.0f, 0.0f));
            }
        }
    }

    void actor_instance::process_motion_event_special_attacking(int motion_event_index, const race_motion_data::motion_event_data *data) {
        if (race_motion_data::MOTION_EVENT_TYPE_SPECIAL_ATTACKING != data->type_value)
            return;

        const auto *attacking_data = (const race_motion_data::motion_attacking_event_data *)data;

        float radian = math::to_radian(270.0f + 360.0f - rotation());
        splash_area_value_.is_enable_hit_process = attacking_data->is_enable_hit_process;
        splash_area_value_.skill = cur_mot_node_.skill;
        splash_area_value_.motion_key = cur_mot_node_.motion_key;
        splash_area_value_.disappearing_time = local_time() + attacking_data->duration_time;
        splash_area_value_.c_attacking_event = attacking_data;
        splash_area_value_.hitted_instance_map_value.clear();

        splash_area_value_.sphere_instance_vector.clear();
        splash_area_value_.sphere_instance_vector.resize(attacking_data->collision_data_value.sphere_data_vector.size());
        for (uint32_t i = 0; i < attacking_data->collision_data_value.sphere_data_vector.size(); ++i) {
            const sphere_data &sphere_data = attacking_data->collision_data_value.sphere_data_vector[i].attribute();
            dynamic_sphere_instance &sphere_instance = splash_area_value_.sphere_instance_vector[i];

            sphere_instance.radius = sphere_data.radius;

            sphere_instance.position_value.x = x_ + sphere_data.position_value.x * sinf(radian) + sphere_data.position_value.y * cosf(radian);
            sphere_instance.position_value.y = y_ + sphere_data.position_value.x * cosf(radian) - sphere_data.position_value.y * sinf(radian);
            sphere_instance.position_value.z = z_ + sphere_data.position_value.z;
            sphere_instance.last_position = sphere_instance.position_value;
        }
    }

    void actor_instance::process_motion_event_sound(const race_motion_data::motion_event_data *data) {
        if (race_motion_data::MOTION_EVENT_TYPE_SOUND != data->type_value)
            return;

        const auto *sound_data = (const race_motion_data::motion_sound_event_data *)data;

        metin2::log::debug("Playing sound {}", sound_data->sound_file_name.c_str());
        get_sound_manager().play_sound3d(x_, y_, z_, sound_data->sound_file_name.c_str());
    }

    void actor_instance::process_motion_event_fly(const race_motion_data::motion_event_data *data) {
        if (race_motion_data::MOTION_EVENT_TYPE_FLY != data->type_value)
            return;

        const auto *fly_data = (const race_motion_data::motion_fly_event_data *)data;

        if (fly_target_value_.is_valid_target()) {
            flying_manager &rfm = get_flying_manager();

            math::vector3 start(fly_data->fly_position);
            start += position_value_;

            if (fly_data->is_attaching) {
                math::matrix *bone_mat;
                int bone_index;
                uint32_t part_index = 0;

                if (find_bone_index(part_index, fly_data->attaching_bone_name.c_str(), &bone_index))
                    if (bone_matrix(part_index, bone_index, &bone_mat)) {
                        start.x += bone_mat->_41;
                        start.y += bone_mat->_42;
                        start.z += bone_mat->_43;
                    }
            }

            flying_instance *instance = rfm.create_flying_instance_fly_target(fly_data->fly_index, start, fly_target_value_, true);
            if (instance) {
                instance->set_event_handler(fly_event_handler_value_);
                instance->set_owner(this);
                instance->set_skill_index(cur_mot_node_.skill);
            }

            if (fly_event_handler_value_) {
                fly_event_handler_value_->on_shoot(cur_mot_node_.skill);
            }
        } else {
        }
    }

    void actor_instance::process_motion_event_warp(const race_motion_data::motion_event_data *data) {
        if (race_motion_data::MOTION_EVENT_TYPE_WARP != data->type_value)
            return;

        // FIXME: belongs in the warp event data.
        static const float distance_from_target = 270.0f;

        if (fly_target_value_.is_valid_target()) {
            math::vector3 main_position(x_, y_, z_);
            const math::vector3 &target_position = fly_target_position_impl();

            math::vector3 distance = target_position - main_position;
            distance = math::normalize(distance);
            pixel_position dest_pixel_position = target_position - (distance * distance_from_target);

            // Don't warp onto a blocked position; Gungsin Tanyoung could otherwise leave the character stuck in the map.
            background_interface &bg = get_background();
            if (!bg.is_block(dest_pixel_position.x, -dest_pixel_position.y))
                set_pixel_position(dest_pixel_position);

            look_at(target_position.x, target_position.y);

            on_warp_impl();
        } else {
        }
    }
}
