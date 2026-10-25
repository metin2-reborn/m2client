#include <metin2/core/mapped_file.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/files/asset_toml.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>

#include "race_data.h"
#include "race_motion_data.h"

#include <iterator>

namespace metin2::inline world::inline actors {

    namespace {
        // The motion names of [[motion]] and the motion index each one registers.
        constexpr std::string_view motion_names[] = {
            "Spawn",
            "Wait",
            "Walk",
            "Run",
            "Stop",
            "Dead",
            "DeadBack",
            "NormalAttack",
            "ComboAttack1",
            "ComboAttack2",
            "ComboAttack3",
            "Damage",
            "DamageBack",
            "DamageFlying",
            "DamageFlyingBack",
            "StandUp",
            "StandUpBack",
            "Special1",
            "Special2",
            "Special3",
            "Special4",
            "Special5",
            "Special6",
            "Skill1",
            "Skill2",
            "Skill3",
            "Skill4",
            "Skill5",
        };
        constexpr uint16_t motion_indices[] = {
            race_motion_data::NAME_SPAWN,
            race_motion_data::NAME_WAIT,
            race_motion_data::NAME_WALK,
            race_motion_data::NAME_RUN,
            race_motion_data::NAME_STOP,
            race_motion_data::NAME_DEAD,
            race_motion_data::NAME_DEAD_BACK,
            race_motion_data::NAME_NORMAL_ATTACK,
            race_motion_data::NAME_COMBO_ATTACK_1,
            race_motion_data::NAME_COMBO_ATTACK_2,
            race_motion_data::NAME_COMBO_ATTACK_3,
            race_motion_data::NAME_DAMAGE,
            race_motion_data::NAME_DAMAGE_BACK,
            race_motion_data::NAME_DAMAGE_FLYING,
            race_motion_data::NAME_DAMAGE_FLYING_BACK,
            race_motion_data::NAME_STAND_UP,
            race_motion_data::NAME_STAND_UP_BACK,
            race_motion_data::NAME_SPECIAL_1,
            race_motion_data::NAME_SPECIAL_2,
            race_motion_data::NAME_SPECIAL_3,
            race_motion_data::NAME_SPECIAL_4,
            race_motion_data::NAME_SPECIAL_5,
            race_motion_data::NAME_SPECIAL_6,
            // The monster skills sit after the player skills.
            race_motion_data::NAME_SKILL + 121,
            race_motion_data::NAME_SKILL + 122,
            race_motion_data::NAME_SKILL + 123,
            race_motion_data::NAME_SKILL + 124,
            race_motion_data::NAME_SKILL + 125,
        };
        static_assert(std::size(motion_names) == std::size(motion_indices));
    }

    auto race_data::load(std::string_view text, std::string_view name) -> bool {
        const auto table = parse_asset_toml(text, name, "Race");
        if (!table) {
            return false;
        }

        asset_table root(*table, name);
        root.table("asset");
        bool ok = true;

        if (const toml::table *race = root.table("race")) {
            asset_table values(*race, name);
            ok &= values.optional("model", base_model_file_name_);
            ok &= values.optional("tree", tree_file_name_);
            ok &= values.optional("attribute", attribute_file_name_);
            ok &= values.optional("smoke_bone", smoke_bone_name_);
            if (const toml::array *smoke = values.array("smoke")) {
                if (smoke->size() > SMOKE_NUM) {
                    values.error("smoke", "has more than four effects");
                    ok = false;
                }
                for (size_t i = 0; i < smoke->size() && i < SMOKE_NUM; ++i) {
                    const auto effect = (*smoke)[i].value_exact<std::string>();
                    if (!effect) {
                        values.error("smoke", "must be effect names");
                        ok = false;
                        break;
                    }
                    smoke_file_names_[i] = *effect;
                }
            }
            ok &= values.finish();
        } else {
            root.error("race", "is missing");
            ok = false;
        }

        if (const toml::array *shapes = root.array("shape")) {
            for (const toml::node &node : *shapes) {
                const toml::table *shape_table = node.as_table();
                if (!shape_table) {
                    root.error("shape", "must be tables ([[shape]])");
                    ok = false;
                    continue;
                }
                asset_table values(*shape_table, name);
                uint32_t index = 0;
                std::string model;
                ok &= values.required("index", index);
                ok &= values.required("model", model);
                set_shape_model(index, model.c_str());
                if (const toml::array *skins = values.array("skins")) {
                    for (const toml::node &skin_node : *skins) {
                        const toml::array *pair = skin_node.as_array();
                        const auto source = pair && pair->size() == 2 ? (*pair)[0].value_exact<std::string>() : std::nullopt;
                        const auto target = pair && pair->size() == 2 ? (*pair)[1].value_exact<std::string>() : std::nullopt;
                        if (!source || !target) {
                            values.error("skins", "must be [source, target] name pairs");
                            ok = false;
                            break;
                        }
                        append_shape_skin(index, 0, source->c_str(), target->c_str());
                    }
                }
                ok &= values.finish();
            }
        }

        if (const toml::array *hairs = root.array("hair")) {
            for (const toml::node &node : *hairs) {
                const toml::table *hair_table = node.as_table();
                if (!hair_table) {
                    root.error("hair", "must be tables ([[hair]])");
                    ok = false;
                    continue;
                }
                asset_table values(*hair_table, name);
                uint32_t index = 0;
                std::string model;
                ok &= values.required("index", index);
                ok &= values.required("model", model);
                const toml::array *skin = values.array("skin");
                const auto source = skin && skin->size() == 2 ? (*skin)[0].value_exact<std::string>() : std::nullopt;
                const auto target = skin && skin->size() == 2 ? (*skin)[1].value_exact<std::string>() : std::nullopt;
                if (!source || !target) {
                    values.error("skin", "must be a [source, target] name pair");
                    ok = false;
                } else {
                    set_hair_skin(index, 0, model.c_str(), source->c_str(), target->c_str());
                }
                ok &= values.finish();
            }
        }

        motion_list_.clear();
        if (const toml::array *motions = root.array("motion")) {
            for (const toml::node &node : *motions) {
                const toml::table *motion_table = node.as_table();
                if (!motion_table) {
                    root.error("motion", "must be tables ([[motion]])");
                    ok = false;
                    continue;
                }
                asset_table values(*motion_table, name);
                int motion = 0;
                motion_entry entry{};
                ok &= values.required_enum("name", motion_names, motion);
                entry.index = motion_indices[motion];
                ok &= values.required("file", entry.file_name);
                ok &= values.required("percent", entry.percentage);
                motion_list_.push_back(std::move(entry));
                ok &= values.finish();
            }
        }

        attaching_data_vector_value_.clear();
        if (const toml::array *attachments = root.array("attach")) {
            for (const toml::node &node : *attachments) {
                const toml::table *attach_table = node.as_table();
                if (!attach_table) {
                    root.error("attach", "must be tables ([[attach]])");
                    ok = false;
                    continue;
                }
                asset_table values(*attach_table, name);
                ok &= race_types::load_attaching_data(values, &attaching_data_vector_value_.emplace_back());
                ok &= values.finish();
            }
        }

        return root.finish() && ok;
    }

    auto race_data::load_race_data(const char *file_name) -> bool {
        const std::string name = eter_file_manager::canonical_name(file_name);

        mapped_file file;
        const void *data = nullptr;
        if (!get_file_manager().get(file, name.c_str(), &data)) {
            return false;
        }
        if (!load(std::string_view(static_cast<const char *>(data), file.size()), name)) {
            return false;
        }

        for (uint32_t i = 0; i < SMOKE_NUM; ++i) {
            if (!smoke_file_names_[i].empty() && !get_effect_manager().register_effect2(smoke_file_names_[i].c_str(), &smoke_effect_id_[i])) {
                metin2::log::error("{}: cannot register smoke effect {}", name, smoke_file_names_[i]);
                smoke_effect_id_[i] = 0;
                return false;
            }
        }
        for (const race_types::attaching_data &attaching : attaching_data_vector_value_) {
            if (attaching.effect_data_value) {
                get_effect_manager().register_effect(attaching.effect_data_value->file_name.c_str());
            }
        }
        return true;
    }
}
