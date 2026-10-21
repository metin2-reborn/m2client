#include "game_type.h"
#include <metin2/core/parse.h>
#include <metin2/effects/effect_manager.h>

namespace metin2::inline world::inline actors {

    float game_fps = 60.0f;

    dynamic_pool<race_types::collision_data> race_types::collision_data_pool;
    dynamic_pool<race_types::attaching_effect_data> race_types::effect_data_pool;
    dynamic_pool<race_types::attaching_object_data> race_types::object_data_pool;

    void race_types::destroy_system() {
        collision_data_pool.destroy();
        effect_data_pool.destroy();
        object_data_pool.destroy();
    }

    auto race_types::load_attack_data(text_file_loader &text_file_loader, attack_data *data) -> bool {
        if (!text_file_loader.token_integer("attacktype", &data->attack_type_value)) {
            data->attack_type_value = ATTACK_TYPE_SPLASH;
        }
        if (!text_file_loader.token_integer("hittingtype", &data->hitting_type))
            return false;
        if (!text_file_loader.token_float("stiffentime", &data->stiffen_time))
            return false;
        if (!text_file_loader.token_float("invisibletime", &data->invisible_time))
            return false;
        if (!text_file_loader.token_float("externalforce", &data->external_force))
            return false;
        if (!text_file_loader.token_integer("hitlimitcount", &data->hit_limit_count)) {
            data->hit_limit_count = 0;
        }

        return true;
    }

    auto race_types::hit_data::load(text_file_loader &text_file_loader) -> bool {
        if (!text_file_loader.token_float("attackingstarttime", &attack_start_time))
            return false;
        if (!text_file_loader.token_float("attackingendtime", &attack_end_time))
            return false;

        if (!text_file_loader.token_string("attackingbone", &bone_name))
            bone_name = "";
        if (!text_file_loader.token_float("weaponlength", &weapon_length))
            weapon_length = 0.0f;

        hit_position.clear();

        token_vector *tv;
        if (text_file_loader.get_token_vector("hitposition", &tv)) {
            auto it = tv->begin();

            while (it != tv->end()) {
                float time;
                race_types::hit_time_position hp;
                time = metin2::to_number<double>(it++->c_str());
                hp.last_position.x = metin2::to_number<double>(it++->c_str());
                hp.last_position.y = metin2::to_number<double>(it++->c_str());
                hp.last_position.z = metin2::to_number<double>(it++->c_str());
                hp.position_value.x = metin2::to_number<double>(it++->c_str());
                hp.position_value.y = metin2::to_number<double>(it++->c_str());
                hp.position_value.z = metin2::to_number<double>(it++->c_str());

                hit_position[time] = hp;
            }
        }

        return true;
    }

    auto race_types::load_motion_attack_data(text_file_loader &text_file_loader, motion_attack_data *data) -> bool {
        if (!load_attack_data(text_file_loader, data))
            return false;

        // Motion type used to be called attacking type; older data without "motiontype" is read
        // through the old key.
        if (!text_file_loader.token_integer("motiontype", &data->motion_type_value)) {
            if (!text_file_loader.token_integer("attackingtype", &data->motion_type_value))
                return false;
        }

        data->hit_data_container_value.clear();

        uint32_t hit_data_count;
        if (!text_file_loader.token_double_word("hitdatacount", &hit_data_count)) {
            data->hit_data_container_value.push_back(hit_data());
            hit_data &hit_data_value = *(data->hit_data_container_value.rbegin());
            if (!hit_data_value.load(text_file_loader))
                return false;
        } else {
            for (uint32_t i = 0; i < hit_data_count; ++i) {
                if (!text_file_loader.set_child_node(i))
                    return false;

                data->hit_data_container_value.push_back(hit_data());
                hit_data &hit_data_value = *(data->hit_data_container_value.rbegin());
                if (!hit_data_value.load(text_file_loader))
                    return false;

                text_file_loader.set_parent_node();
            }
        }

        return true;
    }

    auto race_types::load_collision_data(text_file_loader &text_file_loader, collision_data *collision_data_value) -> bool {
        if (!text_file_loader.token_integer("collisiontype", &collision_data_value->collision_type_value))
            return false;

        uint32_t sphere_data_count;
        if (!text_file_loader.token_double_word("spheredatacount", &sphere_data_count))
            return false;

        collision_data_value->sphere_data_vector.clear();
        collision_data_value->sphere_data_vector.resize(sphere_data_count);
        for (uint32_t i = 0; i < sphere_data_count; ++i) {
            sphere_data &r = collision_data_value->sphere_data_vector[i].attribute();

            if (!text_file_loader.set_child_node("spheredata", i))
                return false;

            if (!text_file_loader.token_float("radius", &r.radius))
                return false;
            if (!text_file_loader.token_position("position", &r.position_value))
                return false;

            text_file_loader.set_parent_node();
        }

        return true;
    }

    auto race_types::load_effect_data(text_file_loader &text_file_loader, attaching_effect_data *effect_data) -> bool {
        if (!text_file_loader.token_string("effectscriptname", &effect_data->file_name))
            return false;

        if (!text_file_loader.token_position("effectposition", &effect_data->position_value)) {
            effect_data->position_value = math::vector3(0.0f, 0.0f, 0.0f);
        }

        if (!text_file_loader.token_position("effectrotation", &effect_data->rotation)) {
            effect_data->rotation = math::vector3(0.0f, 0.0f, 0.0f);
        }

        get_effect_manager().register_effect(effect_data->file_name.c_str());

        return true;
    }

    auto race_types::load_object_data(text_file_loader &text_file_loader, attaching_object_data *object_data) -> bool {
        if (!text_file_loader.token_string("objectscriptname", &object_data->file_name))
            return false;

        return true;
    }

    auto race_types::load_attaching_data(text_file_loader &text_file_loader, attaching_data_vector *attaching_data_vector_value) -> bool {
        uint32_t data_count;
        if (!text_file_loader.token_double_word("attachingdatacount", &data_count))
            return false;

        attaching_data_vector_value->clear();
        attaching_data_vector_value->resize(data_count);

        for (uint32_t i = 0; i < data_count; ++i) {
            race_types::attaching_data &attaching_data_value = attaching_data_vector_value->at(i);
            if (!text_file_loader.set_child_node("attachingdata", i))
                return false;

            if (!text_file_loader.token_double_word("attachingdatatype", &attaching_data_value.type_value))
                return false;

            if (!text_file_loader.token_boolean("isattaching", &attaching_data_value.is_attaching))
                return false;
            if (!text_file_loader.token_double_word("attachingmodelindex", &attaching_data_value.attaching_model_index))
                return false;
            if (!text_file_loader.token_string("attachingbonename", &attaching_data_value.attaching_bone_name))
                return false;

            switch (attaching_data_value.type_value) {
            case race_types::ATTACHING_DATA_TYPE_COLLISION_DATA:
                attaching_data_value.collision_data_value = collision_data_pool.alloc();
                if (!race_types::load_collision_data(text_file_loader, attaching_data_value.collision_data_value))
                    return false;
                break;
            case race_types::ATTACHING_DATA_TYPE_EFFECT:
                attaching_data_value.effect_data_value = effect_data_pool.alloc();
                if (!race_types::load_effect_data(text_file_loader, attaching_data_value.effect_data_value))
                    return false;
                break;
            case race_types::ATTACHING_DATA_TYPE_OBJECT:
                attaching_data_value.object_data_value = object_data_pool.alloc();
                if (!race_types::load_object_data(text_file_loader, attaching_data_value.object_data_value))
                    return false;
                break;
            }

            text_file_loader.set_parent_node();
        }
        return true;
    }
}
