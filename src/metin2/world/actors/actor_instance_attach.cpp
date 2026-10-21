#include <metin2/effects/effect_manager.h>
#include <metin2/log/log.h>

#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/world/items/item_data.h>
#include <metin2/world/items/item_manager.h>
#include "actor_instance.h"
#include "race_data.h"
#include "weapon_trace.h"

namespace metin2::inline world::inline actors {

    bool use_weapon_specular = true;

    bool use_vietnam_convert_weapon_vnum = false;

    auto vietnam_convert_weapon_vnum(uint32_t vnum) -> uint32_t {
        uint32_t base = vnum / 10 * 10;
        uint32_t rest = vnum % 10;
        switch (base) {
        case 10:   base = 5000; break;
        case 20:   base = 5010; break;
        case 30:   base = 5020; break;
        case 40:   base = 5030; break;
        case 50:   base = 5030; break;
        case 60:   base = 5040; break;
        case 70:   base = 5040; break;
        case 80:   base = 5050; break;
        case 90:   base = 5050; break;
        case 100:  base = 5060; break;
        case 110:  base = 5060; break;
        case 120:  base = 5070; break;
        case 130:  base = 5070; break;
        case 140:  base = 5080; break;
        case 150:  base = 5080; break;
        case 160:  base = 5090; break;
        case 170:  base = 5090; break;
        case 180:  base = 5100; break;
        case 190:  base = 5100; break;
        case 200:  base = 5110; break;
        case 210:  base = 5110; break;
        case 220:  base = 5120; break;
        case 230:  base = 5120; break;
        case 240:  base = 5130; break;
        case 250:  base = 5130; break;
        case 260:  base = 5140; break;
        case 270:  base = 5140; break;
        case 280:  base = 5150; break;
        case 290:  base = 5150; break;
        case 1000: base = 5000; break;
        case 1010: base = 5010; break;
        case 1020: base = 5020; break;
        case 1030: base = 5030; break;
        case 1040: base = 5040; break;
        case 1050: base = 5050; break;
        case 1060: base = 5060; break;
        case 1070: base = 5070; break;
        case 1080: base = 5080; break;
        case 1090: base = 5090; break;
        case 1100: base = 5100; break;
        case 1110: base = 5110; break;
        case 1120: base = 5120; break;
        case 1130: base = 5130; break;
        case 1140: base = 5140; break;
        case 1150: base = 5150; break;
        case 1160: base = 5150; break;
        case 1170: base = 5150; break;
        case 3000: base = 5000; break;
        case 3010: base = 5010; break;
        case 3020: base = 5020; break;
        case 3030: base = 5030; break;
        case 3040: base = 5040; break;
        case 3050: base = 5050; break;
        case 3060: base = 5060; break;
        case 3070: base = 5070; break;
        case 3080: base = 5080; break;
        case 3090: base = 5090; break;
        case 3100: base = 5100; break;
        case 3110: base = 5100; break;
        case 3120: base = 5110; break;
        case 3130: base = 5110; break;
        case 3140: base = 5120; break;
        case 3150: base = 5120; break;
        case 3160: base = 5130; break;
        case 3170: base = 5130; break;
        case 3180: base = 5140; break;
        case 3190: base = 5140; break;
        case 3200: base = 5150; break;
        case 3210: base = 5150; break;
        }
        return base + rest;
    }

    auto actor_instance::attach_smoke_effect(uint32_t smoke) -> uint32_t {
        if (!cur_race_data_)
            return 0;

        uint32_t smoke_effect_id = cur_race_data_->smoke_effect_id(smoke);

        return attach_effect_by_id(0, cur_race_data_->smoke_bone().c_str(), smoke_effect_id);
    }

    auto actor_instance::is_left_hand_weapon(uint32_t type_value) -> bool {
        if (item_data::WEAPON_DAGGER == type_value || (item_data::WEAPON_FAN == type_value && is_mounting_horse_impl()))
            return true;
        else if (item_data::WEAPON_BOW == type_value)
            return true;
        else
            return false;
    }

    auto actor_instance::is_right_hand_weapon(uint32_t type_value) -> bool {
        if (item_data::WEAPON_DAGGER == type_value || (item_data::WEAPON_FAN == type_value && is_mounting_horse_impl()))
            return true;
        else if (item_data::WEAPON_BOW == type_value)
            return false;
        else
            return true;
    }

    auto actor_instance::is_weapon_trace(uint32_t weapon_type) -> bool {
        switch (weapon_type) {
        case item_data::WEAPON_BELL:
        case item_data::WEAPON_FAN:
        case item_data::WEAPON_BOW:
            return false;
        default:
            return true;
        }
    }

    void actor_instance::attach_weapon(uint32_t item_index, uint32_t parent_part_index, uint32_t part_index) {
        if (part_index >= race_data::PART_MAX_NUM)
            return;

        part_item_id_[part_index] = item_index;

        if (use_vietnam_convert_weapon_vnum)
            item_index = vietnam_convert_weapon_vnum(item_index);

        item_data *item_data;
        if (!get_item_manager().item_data_pointer(item_index, &item_data)) {
            register_model_thing(part_index, nullptr);
            set_model_instance(part_index, part_index, 0);

            register_model_thing(race_data::PART_WEAPON_LEFT, nullptr);
            set_model_instance(race_data::PART_WEAPON_LEFT, race_data::PART_WEAPON_LEFT, 0);

            refresh_actor_instance();
            return;
        }

        destroy_weapon_trace();
        // Two-handed weapons (assassin dual blades) attach to both the left and the right hand.
        if (is_right_hand_weapon(item_data->weapon_type()))
            attach_weapon(parent_part_index, race_data::PART_WEAPON, item_data);
        if (is_left_hand_weapon(item_data->weapon_type()))
            attach_weapon(parent_part_index, race_data::PART_WEAPON_LEFT, item_data);
    }

    auto actor_instance::attaching_bone_name(uint32_t part_index, const char **bone_name) -> bool {
        return cur_race_data_->attaching_bone_name(part_index, bone_name);
    }

    void actor_instance::attach_weapon(uint32_t parent_part_index, uint32_t part_index, item_data *item_data) {
        if (!item_data)
            return;

        const char *bone_name;
        if (!attaching_bone_name(part_index, &bone_name))
            return;

        // The left dual blade uses the sub model, which falls back to the default model when there is none.
        if (race_data::PART_WEAPON_LEFT == part_index) {
            register_model_thing(part_index, item_data->sub_model_thing());
        } else {
            register_model_thing(part_index, item_data->model_thing());
        }

        for (uint32_t i = 0; i < item_data->lod_model_thing_count(); ++i) {
            graphic_thing *thing;

            if (!item_data->lod_model_thing_pointer(i, &thing))
                continue;

            register_lod_thing(part_index, thing);
        }

        set_model_instance(part_index, part_index, 0);
        attach_model_instance(parent_part_index, bone_name, part_index);

        // Weapon specular; armour specular is handled directly in set_shape.
        if (use_weapon_specular) {
            material_data material_data;
            material_data.image = nullptr;
            material_data.is_specular_enable = true;
            material_data.specular_power = item_data->specular_powerf();
            material_data.sphere_map_index = 1;
            set_material_data(part_index, nullptr, material_data);
        }

        if (is_weapon_trace(item_data->weapon_type())) {
            weapon_trace *weapon_trace = weapon_trace::new_value();
            weapon_trace->set_weapon_instance(this, part_index, bone_name);
            weapon_trace_vector_.push_back(weapon_trace);
        }
    }

    void actor_instance::dettach_effect(uint32_t eid) {
        auto i = attaching_effect_list_.begin();

        while (i != attaching_effect_list_.end()) {
            attaching_effect &att_eft = (*i);

            if (att_eft.effect_index == eid) {
                i = attaching_effect_list_.erase(i);
                get_effect_manager().destroy_effect_instance(eid);
            } else {
                ++i;
            }
        }
    }

    auto actor_instance::attach_effect_by_name(uint32_t parent_part_index, const char *bone_name, const char *effect_name_value) -> uint32_t {
        std::string str;
        uint32_t crc;
        string_path(effect_name_value, str);
        crc = get_case_crc32(str.c_str(), str.length());

        return attach_effect_by_id(parent_part_index, bone_name, crc);
    }

    auto actor_instance::attach_effect_by_id(uint32_t parent_part_index, const char *bone_name, uint32_t effect_id, const math::vector3 *position) -> uint32_t {
        attaching_effect ae;
        ae.life_type = EFFECT_LIFE_INFINITE;
        ae.end_time = 0;
        ae.model_index = parent_part_index;
        ae.effect_index = get_effect_manager().empty_index();
        ae.is_attaching = true;
        if (position) {
            ae.translation = math::matrix_translation(position->x, position->y, position->z);
        } else {
            ae.translation = math::matrix_identity();
        }
        effect_manager &eft_mgr = get_effect_manager();
        eft_mgr.create_effect_instance(ae.effect_index, effect_id);

        if (bone_name) {
            int bone_index;

            if (!find_bone_index(parent_part_index, bone_name, &bone_index)) {
                ae.bone_index = -1;
            } else {
                ae.bone_index = bone_index;
            }
        } else {
            ae.bone_index = -1;
        }

        attaching_effect_list_.push_back(ae);

        return ae.effect_index;
    }

    void actor_instance::refresh_actor_instance() {
        if (!cur_race_data_) {
            metin2::log::error("Cannot refresh actor: no race data");
            return;
        }

        body_point_instance_list_.clear();
        defending_point_instance_list_.clear();

        for (uint32_t i = 0; i < cur_race_data_->attaching_data_count(); ++i) {
            const race_types::attaching_data *attaching_data;

            if (!cur_race_data_->attaching_data_pointer(i, &attaching_data))
                continue;

            switch (attaching_data->type_value) {
            case race_types::ATTACHING_DATA_TYPE_COLLISION_DATA:
                {
                    const race_types::collision_data *collision_data = attaching_data->collision_data_value;

                    collision_point_instance point_instance;
                    if (race_types::COLLISION_TYPE_ATTACKING == collision_data->collision_type_value)
                        continue;

                    if (!create_collision_instance_piece(race_data::PART_MAIN, attaching_data, &point_instance))
                        continue;

                    switch (collision_data->collision_type_value) {
                    case race_types::COLLISION_TYPE_ATTACKING:
                        break;
                    case race_types::COLLISION_TYPE_DEFENDING:
                        defending_point_instance_list_.push_back(point_instance);
                        break;
                    case race_types::COLLISION_TYPE_BODY:
                        body_point_instance_list_.push_back(point_instance);
                        break;
                    }
                }
                break;

            case race_types::ATTACHING_DATA_TYPE_EFFECT:

                if (attaching_data->is_attaching) {
                    attach_effect_by_name(0, attaching_data->attaching_bone_name.c_str(), attaching_data->effect_data_value->file_name.c_str());
                } else {
                    attach_effect_by_name(0, nullptr, attaching_data->effect_data_value->file_name.c_str());
                }
                break;

            case race_types::ATTACHING_DATA_TYPE_OBJECT:
                break;

            default:
                assert(false /*NOT_IMPLEMENTED*/);
                break;
            }
        }

        for (uint32_t j = 0; j < race_data::PART_MAX_NUM; ++j) {
            if (0 == part_item_id_[j])
                continue;

            item_data *item_data;
            if (!get_item_manager().item_data_pointer(part_item_id_[j], &item_data))
                return;

            for (uint32_t k = 0; k < item_data->attaching_data_count(); ++k) {
                const race_types::attaching_data *attaching_data;

                if (!item_data->attaching_data_pointer(k, &attaching_data))
                    continue;

                switch (attaching_data->type_value) {
                case race_types::ATTACHING_DATA_TYPE_COLLISION_DATA:
                    {
                        const race_types::collision_data *collision_data = attaching_data->collision_data_value;

                        // FIXME: the first argument is the part index. Is the base always 0?
                        collision_point_instance point_instance;
                        if (race_types::COLLISION_TYPE_ATTACKING == collision_data->collision_type_value)
                            continue;
                        if (!create_collision_instance_piece(j, attaching_data, &point_instance))
                            continue;

                        switch (collision_data->collision_type_value) {
                        case race_types::COLLISION_TYPE_ATTACKING:
                            break;
                        case race_types::COLLISION_TYPE_DEFENDING:
                            defending_point_instance_list_.push_back(point_instance);
                            break;
                        case race_types::COLLISION_TYPE_BODY:
                            body_point_instance_list_.push_back(point_instance);
                            break;
                        }
                    }
                    break;

                case race_types::ATTACHING_DATA_TYPE_EFFECT:
                    if (!effect_initialized_) {
                        uint32_t crc;
                        string_path(attaching_data->effect_data_value->file_name);
                        crc = get_case_crc32(attaching_data->effect_data_value->file_name.c_str(), attaching_data->effect_data_value->file_name.length());

                        attaching_effect ae;
                        ae.life_type = EFFECT_LIFE_INFINITE;
                        ae.end_time = 0;
                        ae.model_index = j;
                        ae.effect_index = get_effect_manager().empty_index();
                        ae.is_attaching = true;
                        get_effect_manager().create_effect_instance(ae.effect_index, crc);

                        int bone_index;
                        if (!find_bone_index(j, attaching_data->attaching_bone_name.c_str(), &bone_index)) {
                            metin2::log::debug("Cannot find the attaching bone");
                            assert(false /*Cannot get Bone Index*/);
                        }
                        metin2::log::debug("Creating attached effect for actor {} (model {}, attachment {})", this, j, k);

                        ae.bone_index = bone_index;

                        attaching_effect_list_.push_back(ae);
                    }
                    break;

                case race_types::ATTACHING_DATA_TYPE_OBJECT:
                    break;

                default:
                    assert(false /*NOT_IMPLEMENTED*/);
                    break;
                }
            }
        }

        effect_initialized_ = true;
    }

    void actor_instance::set_weapon_trace_texture(const char *texture_name) {
        std::vector<weapon_trace *>::iterator it;
        for (it = weapon_trace_vector_.begin(); it != weapon_trace_vector_.end(); ++it) {
            (*it)->set_texture(texture_name);
        }
    }

    void actor_instance::use_texture_weapon_trace() {
        for_each(
            weapon_trace_vector_.begin(),
            weapon_trace_vector_.end(),
            std::mem_fn(&weapon_trace::use_texture));
    }

    void actor_instance::use_alpha_weapon_trace() {
        for_each(
            weapon_trace_vector_.begin(),
            weapon_trace_vector_.end(),
            std::mem_fn(&weapon_trace::use_alpha));
    }

    void actor_instance::update_attaching_instances() {
        effect_manager &eft_mgr = get_effect_manager();

        std::list<attaching_effect>::iterator it;
        uint32_t current_time = get_timer().current_millisecond();
        for (it = attaching_effect_list_.begin(); it != attaching_effect_list_.end();) {
            if (EFFECT_LIFE_WITH_MOTION == it->life_type) {
                ++it;
                continue;
            }

            if ((EFFECT_LIFE_NORMAL == it->life_type && it->end_time < current_time) ||
                !eft_mgr.is_alive_effect(it->effect_index)) {
                eft_mgr.destroy_effect_instance(it->effect_index);
                it = attaching_effect_list_.erase(it);
            } else {
                if (it->is_attaching) {
                    eft_mgr.select_effect_instance(it->effect_index);

                    if (it->bone_index == -1) {
                        math::matrix transform;
                        transform = it->translation;
                        transform *= world_matrix_;
                        eft_mgr.set_effect_instance_global_matrix(transform);
                    } else {
                        math::matrix *bone_mat;
                        if (bone_matrix(it->model_index, it->bone_index, &bone_mat)) {
                            math::matrix transform;
                            transform = *bone_mat;
                            transform *= it->translation;
                            transform *= world_matrix_;
                            eft_mgr.set_effect_instance_global_matrix(transform);
                        } else {
                        }
                    }
                }

                ++it;
            }
        }
    }

    void actor_instance::show_all_attaching_effect() {
        std::list<attaching_effect>::iterator it;
        for (it = attaching_effect_list_.begin(); it != attaching_effect_list_.end(); ++it) {
            get_effect_manager().select_effect_instance(it->effect_index);
            get_effect_manager().show_effect();
        }
    }

    void actor_instance::hide_all_attaching_effect() {
        std::list<attaching_effect>::iterator it;
        for (it = attaching_effect_list_.begin(); it != attaching_effect_list_.end(); ++it) {
            get_effect_manager().select_effect_instance(it->effect_index);
            get_effect_manager().hide_effect();
        }
    }

    void actor_instance::clear_attaching_effect_impl() {
        effect_initialized_ = false;

        std::list<attaching_effect>::iterator it;
        for (it = attaching_effect_list_.begin(); it != attaching_effect_list_.end(); ++it) {
            get_effect_manager().destroy_effect_instance(it->effect_index);
        }
        attaching_effect_list_.clear();
    }
}
