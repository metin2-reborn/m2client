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

    namespace {
        constexpr std::string_view attack_type_names[] = {"Splash", "Snipe"};
        constexpr std::string_view hit_type_names[] = {"None", "Great", "Good"};
        constexpr std::string_view motion_type_names[] = {"None", "Normal", "Combo", "Skill"};
        constexpr std::string_view collision_type_names[] = {"None", "Body", "Attacking", "Defending", "Splash"};
    }

    auto race_types::load_attack_data(asset_table &table, std::string_view type_key, attack_data *data) -> bool {
        data->attack_type_value = ATTACK_TYPE_SPLASH;
        data->hit_limit_count = 0;
        bool ok = table.optional_enum(type_key, attack_type_names, data->attack_type_value);
        ok &= table.required_enum("hitting_type", hit_type_names, data->hitting_type);
        ok &= table.required("stiffen_time", data->stiffen_time);
        ok &= table.required("invisible_time", data->invisible_time);
        ok &= table.required("external_force", data->external_force);
        ok &= table.optional("hit_limit_count", data->hit_limit_count);
        return ok;
    }

    auto race_types::hit_data::load(asset_table &table) -> bool {
        bone_name.clear();
        weapon_length = 0.0f;
        hit_position.clear();

        bool ok = table.required("start_time", attack_start_time);
        ok &= table.required("end_time", attack_end_time);
        ok &= table.optional("bone", bone_name);
        ok &= table.optional("weapon_length", weapon_length);

        if (const toml::array *positions = table.array("positions")) {
            for (const toml::node &row : *positions) {
                float v[7];
                if (!read_numbers(row, v)) {
                    table.error("positions", "must hold rows of seven numbers: time, last position, position");
                    return false;
                }
                hit_time_position &hp = hit_position[v[0]];
                hp.last_position = math::vector3(v[1], v[2], v[3]);
                hp.position_value = math::vector3(v[4], v[5], v[6]);
            }
        }
        return ok;
    }

    auto race_types::load_motion_attack_data(asset_table &table, motion_attack_data *data) -> bool {
        bool ok = load_attack_data(table, "type", data);
        ok &= table.required_enum("motion_type", motion_type_names, data->motion_type_value);

        data->hit_data_container_value.clear();
        if (const toml::array *hits = table.array("hit")) {
            for (const toml::node &node : *hits) {
                const toml::table *hit = node.as_table();
                if (!hit) {
                    table.error("hit", "must be tables ([[attack.hit]])");
                    return false;
                }
                asset_table hit_table(*hit, table.file_name());
                ok &= data->hit_data_container_value.emplace_back().load(hit_table);
                ok &= hit_table.finish();
            }
        }
        return ok;
    }

    auto race_types::load_collision_data(asset_table &table, collision_data *collision_data_value) -> bool {
        bool ok = table.required_enum("collision_type", collision_type_names, collision_data_value->collision_type_value);

        collision_data_value->sphere_data_vector.clear();
        const toml::array *spheres = table.array("spheres");
        if (!spheres) {
            table.error("spheres", "is missing");
            return false;
        }
        for (const toml::node &node : *spheres) {
            const toml::table *sphere = node.as_table();
            if (!sphere) {
                table.error("spheres", "must be tables { radius, position }");
                return false;
            }
            asset_table sphere_table(*sphere, table.file_name());
            sphere_data &r = collision_data_value->sphere_data_vector.emplace_back().attribute();
            ok &= sphere_table.required("radius", r.radius);
            ok &= sphere_table.required("position", r.position_value);
            ok &= sphere_table.finish();
        }
        return ok;
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
