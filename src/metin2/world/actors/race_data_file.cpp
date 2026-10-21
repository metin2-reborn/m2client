#include <metin2/core/parse.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/log/log.h>

#include <metin2/core/file_name.h>
#include "race_data.h"
#include "race_motion_data.h"

namespace metin2::inline world::inline actors {

    auto race_data::load_race_data(const char *file_name) -> bool {
        text_file_loader text_file_loader;
        if (!text_file_loader.load(file_name))
            return false;

        text_file_loader.set_top();

        text_file_loader.token_string("basemodelfilename", &base_model_file_name_);
        text_file_loader.token_string("treefilename", &tree_file_name_);
        text_file_loader.token_string("attributefilename", &attribute_file_name_);
        text_file_loader.token_string("smokebonename", &smoke_bone_name_);
        text_file_loader.token_string("motionlistfilename", &motion_list_file_name_);

        if (!tree_file_name_.empty()) {
            file_name_helper::string_path(tree_file_name_);
        }

        token_vector *smoke_token_vector;
        if (text_file_loader.get_token_vector("smokefilename", &smoke_token_vector)) {
            if (smoke_token_vector->size() % 2 != 0) {
                metin2::log::error("Smoke file names: argument count {} must be even", smoke_token_vector->size());
                return false;
            }

            uint32_t line_count = smoke_token_vector->size() / 2;

            for (uint32_t line = 0; line < line_count; ++line) {
                int smoke = metin2::to_number<int>(smoke_token_vector->at(line * 2 + 0).c_str());
                if (smoke < 0 || smoke >= SMOKE_NUM) {
                    metin2::log::error("Smoke {} out of range", smoke);
                    return false;
                }

                const std::string &effect_file_name = smoke_token_vector->at(line * 2 + 1);

                uint32_t &crc_eft = smoke_effect_id_[smoke];
                if (!get_effect_manager().register_effect2(effect_file_name.c_str(), &crc_eft)) {
                    metin2::log::error("Cannot register effect {}", effect_file_name.c_str());
                    crc_eft = 0;
                    return false;
                }
            }
        }

        if (text_file_loader.set_child_node("shapedata")) {
            std::string path_name;
            uint32_t shape_data_count = 0;
            if (text_file_loader.token_string("pathname", &path_name) &&
                text_file_loader.token_double_word("shapedatacount", &shape_data_count)) {
                for (uint32_t i = 0; i < shape_data_count; ++i) {
                    if (!text_file_loader.set_child_node("shapedata", i)) {
                        continue;
                    }

                    // specialpath is a temporary event feature.
                    text_file_loader.token_string("specialpath", &path_name);

                    uint32_t shape_index;
                    if (!text_file_loader.token_double_word("shapeindex", &shape_index)) {
                        continue;
                    }

                    std::string model;
                    if (text_file_loader.token_string("model", &model)) {
                        set_shape_model(shape_index, (path_name + model).c_str());
                    } else {
                        if (!text_file_loader.token_string("local_model", &model))
                            continue;

                        set_shape_model(shape_index, model.c_str());
                    }

                    std::string source_skin;
                    std::string target_skin;

                    if (text_file_loader.token_string("local_sourceskin", &source_skin) &&
                        text_file_loader.token_string("local_targetskin", &target_skin)) {
                        append_shape_skin(shape_index, 0, source_skin.c_str(), target_skin.c_str());
                    }

                    if (text_file_loader.token_string("sourceskin", &source_skin) &&
                        text_file_loader.token_string("targetskin", &target_skin)) {
                        append_shape_skin(shape_index, 0, (path_name + source_skin).c_str(), (path_name + target_skin).c_str());
                    }

                    if (text_file_loader.token_string("sourceskin2", &source_skin) &&
                        text_file_loader.token_string("targetskin2", &target_skin)) {
                        append_shape_skin(shape_index, 0, (path_name + source_skin).c_str(), (path_name + target_skin).c_str());
                    }

                    text_file_loader.set_parent_node();
                }
            }

            text_file_loader.set_parent_node();
        }

        if (text_file_loader.set_child_node("hairdata")) {
            std::string path_name;
            uint32_t hair_data_count = 0;
            if (text_file_loader.token_string("pathname", &path_name) &&
                text_file_loader.token_double_word("hairdatacount", &hair_data_count)) {

                for (uint32_t i = 0; i < hair_data_count; ++i) {
                    if (!text_file_loader.set_child_node("hairdata", i)) {
                        continue;
                    }

                    // specialpath is a temporary event feature.
                    text_file_loader.token_string("specialpath", &path_name);

                    uint32_t shape_index;
                    if (!text_file_loader.token_double_word("hairindex", &shape_index)) {
                        continue;
                    }

                    std::string model;
                    std::string source_skin;
                    std::string target_skin;
                    if (text_file_loader.token_string("model", &model) &&
                        text_file_loader.token_string("sourceskin", &source_skin) &&
                        text_file_loader.token_string("targetskin", &target_skin)) {
                        set_hair_skin(shape_index, 0, (path_name + model).c_str(), (path_name + source_skin).c_str(), (path_name + target_skin).c_str());
                    }

                    text_file_loader.set_parent_node();
                }
            }

            text_file_loader.set_parent_node();
        }

        if (text_file_loader.set_child_node("attachingdata")) {
            if (!race_types::load_attaching_data(text_file_loader, &attaching_data_vector_value_))
                return false;

            text_file_loader.set_parent_node();
        }

        return true;
    }
}
