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
        constexpr std::string_view attaching_type_names[] = {"", "Collision", "Effect", "Object"};
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

    auto race_types::load_attaching_data(asset_table &table, attaching_data *data) -> bool {
        int type = ATTACHING_DATA_TYPE_NONE;
        bool ok = table.required_enum("type", attaching_type_names, type);
        data->type_value = type;
        ok &= table.required("attaching", data->is_attaching);
        ok &= table.required("model_index", data->attaching_model_index);
        ok &= table.required("bone", data->attaching_bone_name);

        switch (data->type_value) {
        case ATTACHING_DATA_TYPE_COLLISION_DATA:
            data->collision_data_value = collision_data_pool.alloc();
            ok &= load_collision_data(table, data->collision_data_value);
            break;
        case ATTACHING_DATA_TYPE_EFFECT:
            {
                attaching_effect_data &effect = *(data->effect_data_value = effect_data_pool.alloc());
                effect.position_value = math::vector3(0.0f, 0.0f, 0.0f);
                effect.rotation = math::vector3(0.0f, 0.0f, 0.0f);
                ok &= table.required("effect", effect.file_name);
                ok &= table.optional("position", effect.position_value);
                ok &= table.optional("rotation", effect.rotation);
                break;
            }
        case ATTACHING_DATA_TYPE_OBJECT:
            data->object_data_value = object_data_pool.alloc();
            ok &= table.required("object", data->object_data_value->file_name);
            break;
        }
        return ok;
    }
}
