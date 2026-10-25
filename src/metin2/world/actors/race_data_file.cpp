#include <metin2/core/mapped_file.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/files/asset_toml.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>

#include "race_data.h"

namespace metin2::inline world::inline actors {

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
            ok &= values.optional("motion_list", motion_list_file_name_);
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
