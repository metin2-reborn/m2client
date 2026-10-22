#include "python_skill.h"
#include <metin2/core/parse.h>
#include <metin2/log/log.h>

#include <metin2/client/characters/instance_base.h>
#include <metin2/core/utf8.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/poly/poly.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include "python_player.h"

#include <string>
#include <string_view>

namespace metin2::inline client::inline player {

    std::map<std::string, uint32_t> python_skill::skill_data::status_name_map;
    std::map<std::string, uint32_t> python_skill::skill_data::new_min_status_name_map;
    std::map<std::string, uint32_t> python_skill::skill_data::new_max_status_name_map;
    uint32_t python_skill::skill_data::time_increase_skill_number = 0;

    bool skill_effect_upgrade_enable = false;

    auto split_line(const char *text, token_vector *token_vector, const char *delimeter) -> int {
        token_vector->reserve(10);
        token_vector->clear();

        std::string token;
        std::string line = text;

        uint32_t base_pos = 0;

        do {
            int begin_pos = line.find_first_not_of(delimeter, base_pos);

            if (begin_pos < 0)
                return -1;

            int end_pos;

            if (line[begin_pos] == '"') {
                ++begin_pos;
                end_pos = line.find_first_of("\"", begin_pos);

                if (end_pos < 0)
                    return -2;

                base_pos = end_pos + 1;
            } else {
                end_pos = line.find_first_of(delimeter, begin_pos);
                base_pos = end_pos;
            }

            token_vector->push_back(line.substr(begin_pos, end_pos - begin_pos));

            // Stop on a trailing tab.
            if (int(line.find_first_not_of(delimeter, base_pos)) < 0)
                break;
        } while (base_pos < line.length());

        return 0;
    }

    void string_replace_word(const char *base, int base_len, const char *src, int src_len, const char *dst, int dst_len, std::string &result) {
        const std::string_view text(base, base_len);
        const std::string_view from(src, src_len);
        result.clear();
        size_t prev = 0;
        for (size_t found = text.find(from); found != std::string_view::npos; found = text.find(from, prev)) {
            result.append(text.substr(prev, found - prev));
            result.append(dst, dst_len);
            prev = found + from.size();
        }
        result.append(text.substr(prev));
    }

    auto python_skill::register_skill_table(const char *file_name) -> bool {
        const void *data;
        mapped_file file;
        if (!get_file_manager().get(file, file_name, &data))
            return false;

        memory_text_file_loader text_file_loader;
        text_file_loader.bind(file.size(), data);

        std::string src_poly_rand;
        std::string src_poly_atk;
        std::string src_poly_mwep;

        token_vector token_vector;
        for (uint32_t i = 0; i < text_file_loader.line_count(); ++i) {
            if (!text_file_loader.split_line_by_tab(i, &token_vector))
                continue;

            std::string line = text_file_loader.line_string(i);

            if (TABLE_TOKEN_TYPE_MAX_NUM != token_vector.size()) {
                metin2::log::debug("Skill table {}: line {} has {} tokens and is skipped", file_name, i, token_vector.size());
                continue;
            }

            int vnum = metin2::to_number<int>(token_vector[TABLE_TOKEN_TYPE_VNUM].c_str());

            auto itor = skill_data_map_value_.find(vnum);
            if (skill_data_map_value_.end() == itor) {
                metin2::log::debug("Skill table {}: skill {} on line {} has no description", file_name, vnum, i);
                continue;
            }

            skill_data &skill_data_value = itor->second;

            const std::string &sp_cost_poly = token_vector[TABLE_TOKEN_TYPE_SP_COST_POLY];
            if (!sp_cost_poly.empty()) {
                skill_data_value.need_sp_formula = sp_cost_poly;
            }

            const std::string &cooldown_poly = token_vector[TABLE_TOKEN_TYPE_COOLDOWN_POLY];
            if (!cooldown_poly.empty()) {
                skill_data_value.cool_time_formula = cooldown_poly;
            }

            const std::string &duration_sp_cost_poly = token_vector[TABLE_TOKEN_TYPE_DURATION_SP_COST_POLY];
            if (!duration_sp_cost_poly.empty()) {
                skill_data_value.continuation_sp_formula = duration_sp_cost_poly;
            }

            const std::string &target_range = token_vector[TABLE_TOKEN_TYPE_TARGET_RANGE];
            if (!target_range.empty()) {
                skill_data_value.target_range = metin2::to_number<int>(target_range.c_str());
            }

            skill_data_value.duration = token_vector[TABLE_TOKEN_TYPE_DURATION_POLY];

            const uint32_t level_empty = 1;

            const std::string &c_str_max_level = token_vector[TABLE_TOKEN_TYPE_MAX_LEVEL];
            if (!c_str_max_level.empty()) {
                int max_level = metin2::to_number<int>(c_str_max_level.c_str());
                if (uint32_t(max_level) > level_empty)
                    skill_data_value.max_level = max_level;
            }

            const std::string &c_str_level_limit = token_vector[TABLE_TOKEN_TYPE_LEVEL_LIMIT];
            if (!c_str_level_limit.empty()) {
                int level_limit = metin2::to_number<int>(c_str_level_limit.c_str());
                if (skill_data_value.level_limit > level_empty)
                    skill_data_value.level_limit = level_limit;
            }
            const std::string &point_poly = token_vector[TABLE_TOKEN_TYPE_POINT_POLY];

            bool use_skill_proto = true;

            switch (vnum) {
            case 34:

            case 151:
            case 152:
            case 153:
            case 154:
            case 155:
            case 156:
            case 157:
                use_skill_proto = false;
                break;
            }

            if (!skill_data_value.affect_data_vector.empty() && use_skill_proto) {
                affect_data &affect = skill_data_value.affect_data_vector[0];

                if (strstr(point_poly.c_str(), "atk") != nullptr ||
                    strstr(point_poly.c_str(), "mwep") != nullptr ||
                    strstr(point_poly.c_str(), "number") != nullptr) {
                    src_poly_rand = "";
                    src_poly_atk = "";
                    src_poly_mwep = "";

                    string_replace_word(point_poly.c_str(), point_poly.length(),
                        "number", 6, "min", 3, src_poly_rand);
                    string_replace_word(src_poly_rand.c_str(), src_poly_rand.length(),
                        "atk", 3, "minatk", 6, src_poly_atk);
                    string_replace_word(src_poly_atk.c_str(), src_poly_atk.length(),
                        "mwep", 4, "minmwep", 7, affect.affect_min_formula);

                    string_replace_word(point_poly.c_str(), point_poly.length(),
                        "number", 6, "max", 3, src_poly_rand);
                    string_replace_word(src_poly_rand.c_str(), src_poly_rand.length(),
                        "atk", 3, "maxatk", 6, src_poly_atk);
                    string_replace_word(src_poly_atk.c_str(), src_poly_atk.length(),
                        "mwep", 4, "maxmwep", 7, affect.affect_max_formula);

                    switch (vnum) {
                    case 1: // Triple slash
                        affect.affect_min_formula += "* 3";
                        affect.affect_max_formula += "* 3";
                        break;
                    }

                } else {
                    affect.affect_min_formula = point_poly;
                    affect.affect_max_formula = "";
                }
            }
        }

        return true;
    }

    void python_skill::register_grade_icon_image(skill_data &data, const char *header, const char *image_name) {
        for (int j = 0; j < SKILL_GRADE_COUNT; ++j) {
            grade_data &grade_data_value = data.grade_data_value[j];

            char count[8 + 1];
            snprintf(count, sizeof(count), "_%02d", j + 1);

            std::string file_name = "";
            file_name += header;
            file_name += image_name;
            file_name += count;
            file_name += ".sub";
            grade_data_value.image = (graphic_image *)get_resource_manager().resource_pointer(file_name.c_str());
        }
    }

    void python_skill::register_normal_icon_image(skill_data &data, const char *header, const char *image_name) {
        std::string file_name = "";
        file_name += header;
        file_name += image_name;
        file_name += ".sub";
        data.image = (graphic_image *)get_resource_manager().resource_pointer(file_name.c_str());
        for (int j = 0; j < SKILL_GRADE_COUNT; ++j) {
            grade_data &grade_data_value = data.grade_data_value[j];
            grade_data_value.image = data.image;
        }
    }
    extern const uint32_t c_iSkillIndex_Riding;
    auto python_skill::register_skill_desc(const char *file_name_value) -> bool {
        const void *data_value;
        mapped_file file_value;
        if (!get_file_manager().get(file_value, file_name_value, &data_value))
            return false;

        memory_text_file_loader text_file_loader_value;
        text_file_loader_value.bind(file_value.size(), data_value);

        token_vector token_vector_value;
        for (uint32_t i = 0; i < text_file_loader_value.line_count(); ++i) {
            if (!text_file_loader_value.split_line_by_tab(i, &token_vector_value))
                continue;

            if (DESC_TOKEN_TYPE_VNUM >= token_vector_value.size()) {
                metin2::log::warn("Skill description line {}: no vnum", i + 1);
                continue;
            }

            uint32_t skill_index_value = metin2::to_number<int>(token_vector_value[DESC_TOKEN_TYPE_VNUM].c_str());
            if (skill_index_value == 0) {
                metin2::log::warn("Skill description line {}: no index", i + 1);
                continue;
            }

            if (DESC_TOKEN_TYPE_JOB >= token_vector_value.size()) {
                metin2::log::warn("Skill description line {}: no job", i + 1);
                continue;
            }

            skill_data_map_value_.insert(std::make_pair(skill_index_value, skill_data()));
            skill_data &skill_data_value = skill_data_map_value_[skill_index_value];

            skill_data_value.skill_index = skill_index_value;

            {
                std::string str_job = token_vector_value[DESC_TOKEN_TYPE_JOB];
                stl_lowers(str_job);

                auto it = skill_type_index_map_.find(str_job.c_str());
                if (skill_type_index_map_.end() == it) {
                    metin2::log::warn("Skill {}: unknown skill type {}", skill_index_value, str_job.c_str());
                    continue;
                }

                skill_data_value.type_value = uint8_t(it->second);
            }

            {
                skill_data_value.name_value = token_vector_value[DESC_TOKEN_TYPE_NAME1];
                skill_data_value.grade_data_value[0].name_value = token_vector_value[DESC_TOKEN_TYPE_NAME1];
                skill_data_value.grade_data_value[1].name_value = token_vector_value[DESC_TOKEN_TYPE_NAME2];
                skill_data_value.grade_data_value[2].name_value = token_vector_value[DESC_TOKEN_TYPE_NAME3];

                if (skill_data_value.grade_data_value[1].name_value.empty())
                    skill_data_value.grade_data_value[1].name_value = token_vector_value[DESC_TOKEN_TYPE_NAME1];
                if (skill_data_value.grade_data_value[2].name_value.empty())
                    skill_data_value.grade_data_value[2].name_value = token_vector_value[DESC_TOKEN_TYPE_NAME1];
            }

            {
                skill_data_value.description = token_vector_value[DESC_TOKEN_TYPE_DESCRIPTION];
            }

            {
                skill_data_value.condition_data_vector.clear();
                for (int j = 0; j < CONDITION_COLUMN_COUNT; ++j) {
                    const std::string &condition = token_vector_value[DESC_TOKEN_TYPE_CONDITION1 + j];
                    if (!condition.empty())
                        skill_data_value.condition_data_vector.push_back(condition);
                }
            }

            {
                const std::string &attribute_value = token_vector_value[DESC_TOKEN_TYPE_ATTRIBUTE];
                token_vector attribute_token_vector;
                split_line(attribute_value.c_str(), &attribute_token_vector, "|");

                for (auto it = attribute_token_vector.begin(); it != attribute_token_vector.end(); ++it) {
                    std::string &token = *it;
                    auto it2 = skill_attribute_index_map_.find(token);
                    if (skill_attribute_index_map_.end() == it2) {
                        metin2::log::error("Skill {}: unknown attribute {}", skill_index_value, token.c_str());
                        continue;
                    }
                    skill_data_value.skill_attribute |= it2->second;
                }
            }

            {
                const std::string &weapon = token_vector_value[DESC_TOKEN_TYPE_WEAPON];
                token_vector weapon_token_vector;
                split_line(weapon.c_str(), &weapon_token_vector, "|");

                for (auto it = weapon_token_vector.begin(); it != weapon_token_vector.end(); ++it) {
                    std::string &token = *it;
                    auto it2 = skill_need_weapon_index_map_.find(token);
                    if (skill_need_weapon_index_map_.end() == it2) {
                        metin2::log::error("Skill {}: unknown required weapon {}", skill_index_value, token.c_str());
                        continue;
                    }
                    skill_data_value.need_weapon |= it2->second;
                }
            }

            {
                const std::string &c_rstr_job = token_vector_value[DESC_TOKEN_TYPE_JOB];
                const std::string &icon_name = token_vector_value[DESC_TOKEN_TYPE_ICON_NAME];

                // Exception: riding is a support skill with three levels.
                if (c_iSkillIndex_Riding == skill_index_value) {
                    char icon_file_name_header[64 + 1];
                    snprintf(icon_file_name_header, sizeof(icon_file_name_header), "%sskill/common/support/", image_path.c_str());

                    register_grade_icon_image(skill_data_value, icon_file_name_header, icon_name.c_str());
                } else if (path_name_map_value_.end() != path_name_map_value_.find(c_rstr_job)) {
                    char icon_file_name_header[64 + 1];
                    snprintf(icon_file_name_header, sizeof(icon_file_name_header), "%sskill/%s/", image_path.c_str(), path_name_map_value_[c_rstr_job].c_str());

                    switch (skill_data_value.type_value) {
                    case SKILL_TYPE_ACTIVE:
                        {
                            register_grade_icon_image(skill_data_value, icon_file_name_header, icon_name.c_str());
                            break;
                        }
                    case SKILL_TYPE_SUPPORT:
                    case SKILL_TYPE_GUILD:
                    case SKILL_TYPE_HORSE:
                        {
                            register_normal_icon_image(skill_data_value, icon_file_name_header, icon_name.c_str());
                            break;
                        }
                    }
                }
            }

            if (token_vector_value.size() > DESC_TOKEN_TYPE_MOTION_INDEX) {
                const std::string &motion_index = token_vector_value[DESC_TOKEN_TYPE_MOTION_INDEX];
                if (motion_index.empty()) {
                    skill_data_value.no_motion = true;
                    skill_data_value.motion_index = 0;
                } else {
                    skill_data_value.no_motion = false;
                    skill_data_value.motion_index = (uint16_t)metin2::to_number<int>(motion_index.c_str());
                }
            } else {
                skill_data_value.motion_index = 0;
            }

            if (token_vector_value.size() > DESC_TOKEN_TYPE_TARGET_COUNT_FORMULA)
                skill_data_value.target_count_formula = token_vector_value[DESC_TOKEN_TYPE_TARGET_COUNT_FORMULA].c_str();
            if (token_vector_value.size() > DESC_TOKEN_TYPE_MOTION_LOOP_COUNT_FORMULA)
                skill_data_value.motion_loop_count_formula = token_vector_value[DESC_TOKEN_TYPE_MOTION_LOOP_COUNT_FORMULA].c_str();

            skill_data_value.affect_data_new_vector.clear();
            skill_data_value.affect_data_new_vector.reserve(3);

            for (int j = 0; j < AFFECT_COLUMN_COUNT; ++j) {
                int description_slot_index = DESC_TOKEN_TYPE_AFFECT_DESCRIPTION_1 + j * AFFECT_STEP_COUNT;
                int min_slot_index = DESC_TOKEN_TYPE_AFFECT_MIN_1 + j * AFFECT_STEP_COUNT;
                int max_slot_index = DESC_TOKEN_TYPE_AFFECT_MAX_1 + j * AFFECT_STEP_COUNT;

                if (token_vector_value.size() > size_t(description_slot_index))
                    if (!token_vector_value[description_slot_index].empty()) {
                        skill_data_value.affect_data_vector.push_back(affect_data());

                        affect_data &affect_data_value = *skill_data_value.affect_data_vector.rbegin();
                        affect_data_value.affect_description = "";
                        affect_data_value.affect_min_formula = "";
                        affect_data_value.affect_max_formula = "";

                        affect_data_value.affect_description = token_vector_value[description_slot_index];
                        if (token_vector_value.size() > size_t(min_slot_index))
                            if (!token_vector_value[min_slot_index].empty())
                                affect_data_value.affect_min_formula = token_vector_value[min_slot_index];
                        if (token_vector_value.size() > size_t(max_slot_index))
                            if (!token_vector_value[max_slot_index].empty())
                                affect_data_value.affect_max_formula = token_vector_value[max_slot_index];
                    }
            }

            if (token_vector_value.size() > DESC_TOKEN_TYPE_MOTION_INDEX_GRADE_NUM) {
                int num_grade = metin2::to_number<int>(token_vector_value[DESC_TOKEN_TYPE_MOTION_INDEX_GRADE_NUM].c_str());
                if (SKILL_EFFECT_COUNT < num_grade) {
                    metin2::log::error("Skill {}: grade count {} exceeds the limit {}", skill_data_value.name_value.c_str(), token_vector_value[DESC_TOKEN_TYPE_MOTION_INDEX_GRADE_NUM].c_str(), SKILL_EFFECT_COUNT);
                    return false;
                }
                for (int grade = 0; grade < num_grade; grade++) {
                    skill_data_value.grade_data_value[grade].motion_index = skill_data_value.motion_index + grade * SKILL_GRADEGAP;
                }
                for (int space = num_grade; space < SKILL_EFFECT_COUNT; space++) {
                    skill_data_value.grade_data_value[space].motion_index = skill_data_value.motion_index;
                }
            }

            if (token_vector_value.size() > DESC_TOKEN_TYPE_LEVEL_LIMIT) {
                const std::string &level_limit = token_vector_value[DESC_TOKEN_TYPE_LEVEL_LIMIT];
                if (level_limit.empty())
                    skill_data_value.level_limit = 0;
                else
                    skill_data_value.level_limit = (uint16_t)metin2::to_number<int>(level_limit.c_str());
            }

            if (token_vector_value.size() > DESC_TOKEN_TYPE_MAX_LEVEL) {
                const std::string &max_level = token_vector_value[DESC_TOKEN_TYPE_MAX_LEVEL];
                const uint32_t level_limit_empty = 1;
                if (max_level.empty()) {
                    if (skill_data_value.level_limit > level_limit_empty)
                        skill_data_value.max_level = skill_data_value.level_limit;
                    else
                        skill_data_value.max_level = 20;
                } else
                    skill_data_value.max_level = (uint8_t)metin2::to_number<int>(max_level.c_str());
            }
        }

        return true;
    }

    void python_skill::destroy() {
        skill_data_map_value_.clear();
    }

    auto python_skill::register_skill(uint32_t skill_index, const char *file_name) -> bool {
        text_file_loader text_file_loader;
        if (!text_file_loader.load(file_name)) {
            metin2::log::error("Skill {}: cannot find {}", skill_index, file_name);
            return false;
        }

        text_file_loader.set_top();

        skill_data skill_data_value;
        skill_data_value.skill_index = skill_index;

        std::string type_name;
        if (text_file_loader.token_string("type", &type_name)) {
            stl_lowers(type_name);
            auto it = skill_type_index_map_.find(type_name.c_str());
            if (skill_type_index_map_.end() == it) {
                metin2::log::error("Skill {}: unknown skill type in {}", skill_index, file_name);
                return false;
            }

            skill_data_value.type_value = (uint8_t)it->second;
        } else {
            metin2::log::error("Skill {}: no [type] key in {}", skill_index, file_name);
            return false;
        }

        token_vector *attribute_token_vector;
        if (text_file_loader.get_token_vector("attribute", &attribute_token_vector)) {
            for (auto it = attribute_token_vector->begin(); it != attribute_token_vector->end(); ++it) {
                std::string &token = *it;
                auto it2 = skill_attribute_index_map_.find(token.c_str());
                if (skill_attribute_index_map_.end() == it2) {
                    metin2::log::error("Skill {}: unknown attribute in {}", skill_index, file_name);
                    continue;
                }
                skill_data_value.skill_attribute |= it2->second;
            }
        }

        token_vector *need_weapon_vector;
        if (text_file_loader.get_token_vector("needweapon", &need_weapon_vector)) {
            for (auto it = need_weapon_vector->begin(); it != need_weapon_vector->end(); ++it) {
                std::string &token = *it;
                auto it2 = skill_need_weapon_index_map_.find(token.c_str());
                if (skill_need_weapon_index_map_.end() == it2) {
                    metin2::log::error("Skill {}: unknown required weapon in {}", skill_index, file_name);
                    continue;
                }
                skill_data_value.need_weapon |= it2->second;
            }
        }

        if (!text_file_loader.token_string("name", &skill_data_value.name_value)) {
            metin2::log::error("Skill {}: no [name] key in {}", skill_index, file_name);
            return false;
        }

        text_file_loader.token_string("description", &skill_data_value.description);

        if (!text_file_loader.token_string("iconfilename", &skill_data_value.icon_file_name)) {
            metin2::log::error("Skill {}: no [iconfilename] key in {}", skill_index, file_name);
            return false;
        }

        {
            token_vector *condition_data_vector;

            if (text_file_loader.get_token_vector("conditiondata", &condition_data_vector)) {
                uint32_t size = condition_data_vector->size();
                skill_data_value.condition_data_vector.clear();
                skill_data_value.condition_data_vector.resize(size);
                for (uint32_t i = 0; i < size; ++i) {
                    skill_data_value.condition_data_vector[i] = condition_data_vector->at(i);
                }
            }
        }

        {
            token_vector *affect_data_vector;

            if (text_file_loader.get_token_vector("affectdata", &affect_data_vector)) {
                uint32_t size = affect_data_vector->size() / 3;
                skill_data_value.affect_data_vector.clear();
                skill_data_value.affect_data_vector.resize(size);
                for (uint32_t i = 0; i < size; ++i) {
                    skill_data_value.affect_data_vector[i].affect_description = affect_data_vector->at(i * 3 + 0);
                    skill_data_value.affect_data_vector[i].affect_min_formula = affect_data_vector->at(i * 3 + 1);
                    skill_data_value.affect_data_vector[i].affect_max_formula = affect_data_vector->at(i * 3 + 2);
                }
            }
        }

        {
            token_vector *grade_data_vector;

            if (text_file_loader.get_token_vector("gradedata", &grade_data_vector)) {
                if (SKILL_GRADE_COUNT * 2 != grade_data_vector->size())
                    metin2::log::error("Skill {}: wrong grade data count in {}", skill_index, file_name);

                for (uint32_t i = 0; i < std::min<size_t>(SKILL_GRADE_COUNT, grade_data_vector->size() / 2); ++i) {
                    skill_data_value.grade_data_value[i].name_value = grade_data_vector->at(i * 2 + 0);
                    std::string icon_file_name = image_path + grade_data_vector->at(i * 2 + 1);
                    skill_data_value.grade_data_value[i].image = (graphic_image *)get_resource_manager().resource_pointer(icon_file_name.c_str());
                }
            }
        }

        text_file_loader.token_string("cooltimeformula", &skill_data_value.cool_time_formula);
        text_file_loader.token_string("targetcountformula", &skill_data_value.target_count_formula);
        text_file_loader.token_string("motionloopcountformula", &skill_data_value.motion_loop_count_formula);
        text_file_loader.token_string("needspformula", &skill_data_value.need_sp_formula);
        text_file_loader.token_string("continuationspformula", &skill_data_value.continuation_sp_formula);
        text_file_loader.token_double_word("range", &skill_data_value.target_range);
        text_file_loader.token_byte("maxlevel", &skill_data_value.max_level);
        text_file_loader.token_byte("leveluppoint", &skill_data_value.level_up_point);

        uint16_t motion_index;
        if (text_file_loader.token_word("motionindex", &motion_index)) {
            skill_data_value.motion_index = motion_index;
        } else {
            skill_data_value.motion_index = 0;
        }

        uint16_t motion_index_for_me;
        if (text_file_loader.token_word("motionindexforme", &motion_index_for_me)) {
            skill_data_value.motion_index_for_me = motion_index_for_me;
        } else {
            skill_data_value.motion_index_for_me = 0;
        }

        skill_data_value.icon_file_name = image_path + skill_data_value.icon_file_name;
        skill_data_value.image = (graphic_image *)get_resource_manager().resource_pointer(skill_data_value.icon_file_name.c_str());

        skill_data_map_value_.insert(skill_data_map::value_type(skill_index, skill_data_value));

        if (skill_data_value.is_time_increase_skill()) {
            python_skill::skill_data::time_increase_skill_number = skill_data_value.skill_index;
        }

        return true;
    }

    auto python_skill::get_skill_data(uint32_t skill_index, skill_data **skill_data_value) -> bool {
        auto it = skill_data_map_value_.find(skill_index);

        if (skill_data_map_value_.end() == it)
            return false;

        *skill_data_value = &(it->second);
        return true;
    }

    auto python_skill::skill_data_by_name(const char *name, skill_data **skill_data_value) -> bool {
        auto itor = skill_data_map_value_.begin();
        for (; itor != skill_data_map_value_.end(); ++itor) {
            skill_data *data = &(itor->second);
            if (0 == data->name_value.compare(name)) {
                *skill_data_value = &(itor->second);
                return true;
            }
        }
        return false;
    }

    void python_skill::set_path_name(const char *file_name) {
        path_name_ = file_name;
    }

    python_skill::python_skill() {
        skill_type_index_map_.insert(std::map<std::string, uint32_t>::value_type("none", SKILL_TYPE_NONE));
        skill_type_index_map_.insert(std::map<std::string, uint32_t>::value_type("active", SKILL_TYPE_ACTIVE));
        skill_type_index_map_.insert(std::map<std::string, uint32_t>::value_type("support", SKILL_TYPE_SUPPORT));
        skill_type_index_map_.insert(std::map<std::string, uint32_t>::value_type("guild", SKILL_TYPE_GUILD));
        skill_type_index_map_.insert(std::map<std::string, uint32_t>::value_type("horse", SKILL_TYPE_HORSE));
        skill_type_index_map_.insert(std::map<std::string, uint32_t>::value_type("warrior", SKILL_TYPE_ACTIVE));
        skill_type_index_map_.insert(std::map<std::string, uint32_t>::value_type("assassin", SKILL_TYPE_ACTIVE));
        skill_type_index_map_.insert(std::map<std::string, uint32_t>::value_type("sura", SKILL_TYPE_ACTIVE));
        skill_type_index_map_.insert(std::map<std::string, uint32_t>::value_type("shaman", SKILL_TYPE_ACTIVE));
        skill_type_index_map_.insert(std::map<std::string, uint32_t>::value_type("passive", SKILL_TYPE_ACTIVE));

        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("NEED_POISON_BOTTLE", SKILL_ATTRIBUTE_NEED_POISON_BOTTLE));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("NEED_EMPTY_BOTTLE", SKILL_ATTRIBUTE_NEED_EMPTY_BOTTLE));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("CAN_USE_IF_NOT_ENOUGH", SKILL_ATTRIBUTE_CAN_USE_IF_NOT_ENOUGH));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("FAN_RANGE", SKILL_ATTRIBUTE_FAN_RANGE));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("NEED_CORPSE", SKILL_ATTRIBUTE_NEED_CORPSE));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("NEED_TARGET", SKILL_ATTRIBUTE_NEED_TARGET));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("TOGGLE", SKILL_ATTRIBUTE_TOGGLE));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("WEAPON_LIMITATION", SKILL_ATTRIBUTE_WEAPON_LIMITATION));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("MELEE_ATTACK", SKILL_ATTRIBUTE_MELEE_ATTACK));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("CHARGE_ATTACK", SKILL_ATTRIBUTE_CHARGE_ATTACK));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("USE_HP", SKILL_ATTRIBUTE_USE_HP));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("CAN_CHANGE_DIRECTION", SKILL_ATTRIBUTE_CAN_CHANGE_DIRECTION));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("STANDING_SKILL", SKILL_ATTRIBUTE_STANDING_SKILL));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("ONLY_FOR_ALLIANCE", SKILL_ATTRIBUTE_ONLY_FOR_ALLIANCE));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("CAN_USE_FOR_ME", SKILL_ATTRIBUTE_CAN_USE_FOR_ME));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("ATTACK_SKILL", SKILL_ATTRIBUTE_ATTACK_SKILL));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("MOVING_SKILL", SKILL_ATTRIBUTE_MOVING_SKILL));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("HORSE_SKILL", SKILL_ATTRIBUTE_HORSE_SKILL));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("TIME_INCREASE_SKILL", SKILL_ATTRIBUTE_TIME_INCREASE_SKILL));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("PASSIVE", SKILL_ATTRIBUTE_PASSIVE));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("CANNOT_LEVEL_UP", SKILL_ATTRIBUTE_CANNOT_LEVEL_UP));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("ONLY_FOR_GUILD_WAR", SKILL_ATTRIBUTE_ONLY_FOR_GUILD_WAR));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("CIRCLE_RANGE", SKILL_ATTRIBUTE_CIRCLE_RANGE));
        skill_attribute_index_map_.insert(std::map<std::string, uint32_t>::value_type("SEARCH_TARGET", SKILL_ATTRIBUTE_SEARCH_TARGET));

        skill_need_weapon_index_map_.insert(std::map<std::string, uint32_t>::value_type("SWORD", SKILL_NEED_WEAPON_SWORD));
        skill_need_weapon_index_map_.insert(std::map<std::string, uint32_t>::value_type("DAGGER", SKILL_NEED_WEAPON_DAGGER));
        skill_need_weapon_index_map_.insert(std::map<std::string, uint32_t>::value_type("BOW", SKILL_NEED_WEAPON_BOW));
        skill_need_weapon_index_map_.insert(std::map<std::string, uint32_t>::value_type("TWO_HANDED", SKILL_NEED_WEAPON_TWO_HANDED));
        skill_need_weapon_index_map_.insert(std::map<std::string, uint32_t>::value_type("DOUBLE_SWORD", SKILL_NEED_WEAPON_DOUBLE_SWORD));
        skill_need_weapon_index_map_.insert(std::map<std::string, uint32_t>::value_type("BELL", SKILL_NEED_WEAPON_BELL));
        skill_need_weapon_index_map_.insert(std::map<std::string, uint32_t>::value_type("FAN", SKILL_NEED_WEAPON_FAN));
        skill_need_weapon_index_map_.insert(std::map<std::string, uint32_t>::value_type("ARROW", SKILL_NEED_WEAPON_ARROW));
        skill_need_weapon_index_map_.insert(std::map<std::string, uint32_t>::value_type("EMPTY_HAND", SKILL_NEED_WEAPON_EMPTY_HAND));

        skill_weapon_type_index_map_.insert(std::make_pair(std::string("SWORD"), item_data::WEAPON_SWORD));
        skill_weapon_type_index_map_.insert(std::make_pair(std::string("DAGGER"), item_data::WEAPON_DAGGER));
        skill_weapon_type_index_map_.insert(std::make_pair(std::string("BOW"), item_data::WEAPON_BOW));
        skill_weapon_type_index_map_.insert(std::make_pair(std::string("TWO_HANDED"), item_data::WEAPON_TWO_HANDED));
        skill_weapon_type_index_map_.insert(std::make_pair(std::string("DOUBLE_SWORD"), item_data::WEAPON_DAGGER));
        skill_weapon_type_index_map_.insert(std::make_pair(std::string("BELL"), item_data::WEAPON_BELL));
        skill_weapon_type_index_map_.insert(std::make_pair(std::string("FAN"), item_data::WEAPON_FAN));
        skill_weapon_type_index_map_.insert(std::make_pair(std::string("ARROW"), item_data::WEAPON_ARROW));

        skill_data::status_name_map.insert(std::make_pair(std::string("chain"), POINT_NONE));
        skill_data::status_name_map.insert(std::make_pair(std::string("HR"), POINT_HIT_RATE));
        skill_data::status_name_map.insert(std::make_pair(std::string("LV"), POINT_LEVEL));
        skill_data::status_name_map.insert(std::make_pair(std::string("Level"), POINT_LEVEL));
        skill_data::status_name_map.insert(std::make_pair(std::string("MaxHP"), POINT_MAX_HP));
        skill_data::status_name_map.insert(std::make_pair(std::string("MaxSP"), POINT_MAX_SP));
        skill_data::status_name_map.insert(std::make_pair(std::string("MinMWEP"), POINT_MIN_WEP));
        skill_data::status_name_map.insert(std::make_pair(std::string("MaxMWEP"), POINT_MAX_WEP));
        skill_data::status_name_map.insert(std::make_pair(std::string("MinWEP"), POINT_MIN_WEP));
        skill_data::status_name_map.insert(std::make_pair(std::string("MaxWEP"), POINT_MAX_WEP));
        skill_data::status_name_map.insert(std::make_pair(std::string("MinATK"), POINT_MIN_ATK));
        skill_data::status_name_map.insert(std::make_pair(std::string("MaxATK"), POINT_MAX_ATK));
        skill_data::status_name_map.insert(std::make_pair(std::string("ATKSPD"), POINT_ATT_SPEED));
        skill_data::status_name_map.insert(std::make_pair(std::string("AttackPower"), POINT_MIN_ATK));
        skill_data::status_name_map.insert(std::make_pair(std::string("AtkMin"), POINT_MIN_ATK));
        skill_data::status_name_map.insert(std::make_pair(std::string("AtkMax"), POINT_MAX_ATK));
        skill_data::status_name_map.insert(std::make_pair(std::string("DefencePower"), POINT_DEF_GRADE));
        skill_data::status_name_map.insert(std::make_pair(std::string("DEF"), POINT_DEF_GRADE));
        skill_data::status_name_map.insert(std::make_pair(std::string("MWEP"), POINT_MAGIC_ATT_GRADE));

        skill_data::status_name_map.insert(std::make_pair(std::string("MagicAttackPower"), POINT_MAGIC_ATT_GRADE));
        skill_data::status_name_map.insert(std::make_pair(std::string("INT"), POINT_IQ));
        skill_data::status_name_map.insert(std::make_pair(std::string("STR"), POINT_ST));
        skill_data::status_name_map.insert(std::make_pair(std::string("DEX"), POINT_DX));
        skill_data::status_name_map.insert(std::make_pair(std::string("CON"), POINT_HT));

        skill_data::status_name_map.insert(std::make_pair(std::string("minatk"), POINT_MIN_ATK));
        skill_data::status_name_map.insert(std::make_pair(std::string("maxatk"), POINT_MAX_ATK));
        skill_data::status_name_map.insert(std::make_pair(std::string("minmtk"), POINT_MIN_WEP));
        skill_data::status_name_map.insert(std::make_pair(std::string("maxmtk"), POINT_MAX_WEP));

        skill_data::status_name_map.insert(std::make_pair(std::string("maxhp"), POINT_MAX_HP));
        skill_data::status_name_map.insert(std::make_pair(std::string("maxsp"), POINT_MAX_SP));
        skill_data::status_name_map.insert(std::make_pair(std::string("odef"), POINT_DEF_GRADE));

        skill_data::status_name_map.insert(std::make_pair(std::string("minwep"), POINT_MIN_WEP));
        skill_data::status_name_map.insert(std::make_pair(std::string("maxwep"), POINT_MAX_WEP));
        skill_data::status_name_map.insert(std::make_pair(std::string("minmwep"), POINT_MIN_MAGIC_WEP));
        skill_data::status_name_map.insert(std::make_pair(std::string("maxmwep"), POINT_MAX_MAGIC_WEP));
        skill_data::status_name_map.insert(std::make_pair(std::string("lv"), POINT_LEVEL));
        skill_data::status_name_map.insert(std::make_pair(std::string("ar"), POINT_HIT_RATE));
        skill_data::status_name_map.insert(std::make_pair(std::string("iq"), POINT_IQ));
        skill_data::status_name_map.insert(std::make_pair(std::string("str"), POINT_ST));
        skill_data::status_name_map.insert(std::make_pair(std::string("dex"), POINT_DX));
        skill_data::status_name_map.insert(std::make_pair(std::string("con"), POINT_HT));

        skill_data::new_min_status_name_map.insert(std::make_pair(std::string("atk"), POINT_MIN_ATK));
        skill_data::new_min_status_name_map.insert(std::make_pair(std::string("mtk"), POINT_MIN_WEP));
        skill_data::new_min_status_name_map.insert(std::make_pair(std::string("wep"), POINT_MIN_WEP));
        skill_data::new_min_status_name_map.insert(std::make_pair(std::string("lv"), POINT_LEVEL));
        skill_data::new_min_status_name_map.insert(std::make_pair(std::string("ar"), POINT_HIT_RATE));
        skill_data::new_min_status_name_map.insert(std::make_pair(std::string("iq"), POINT_IQ));
        skill_data::new_min_status_name_map.insert(std::make_pair(std::string("str"), POINT_ST));
        skill_data::new_min_status_name_map.insert(std::make_pair(std::string("dex"), POINT_DX));
        skill_data::new_min_status_name_map.insert(std::make_pair(std::string("con"), POINT_HT));

        skill_data::new_max_status_name_map.insert(std::make_pair(std::string("atk"), POINT_MAX_ATK));
        skill_data::new_max_status_name_map.insert(std::make_pair(std::string("mtk"), POINT_MAX_WEP));
        skill_data::new_min_status_name_map.insert(std::make_pair(std::string("wep"), POINT_MAX_WEP));
        skill_data::new_max_status_name_map.insert(std::make_pair(std::string("lv"), POINT_LEVEL));
        skill_data::new_max_status_name_map.insert(std::make_pair(std::string("ar"), POINT_HIT_RATE));
        skill_data::new_max_status_name_map.insert(std::make_pair(std::string("iq"), POINT_IQ));
        skill_data::new_max_status_name_map.insert(std::make_pair(std::string("str"), POINT_ST));
        skill_data::new_max_status_name_map.insert(std::make_pair(std::string("dex"), POINT_DX));
        skill_data::new_max_status_name_map.insert(std::make_pair(std::string("con"), POINT_HT));

        path_name_map_value_.insert(std::make_pair(std::string("WARRIOR"), std::string("warrior")));
        path_name_map_value_.insert(std::make_pair(std::string("ASSASSIN"), std::string("assassin")));
        path_name_map_value_.insert(std::make_pair(std::string("SURA"), std::string("sura")));
        path_name_map_value_.insert(std::make_pair(std::string("SHAMAN"), std::string("shaman")));
        path_name_map_value_.insert(std::make_pair(std::string("PASSIVE"), std::string("passive")));
        path_name_map_value_.insert(std::make_pair(std::string("SUPPORT"), std::string("common/support")));
        path_name_map_value_.insert(std::make_pair(std::string("GUILD"), std::string("common/guild")));
        path_name_map_value_.insert(std::make_pair(std::string("HORSE"), std::string("common/horse")));
    }
    python_skill::~python_skill() = default;

    uint32_t python_skill::skill_data::MELEE_SKILL_TARGET_RANGE = 170;

    auto python_skill::skill_data::get_name() const -> const std::string {
        return name_value;
    }

    auto python_skill::skill_data::get_type() const -> uint8_t {
        return type_value;
    }

    auto python_skill::skill_data::is_charge_skill() -> bool {
        if (skill_attribute & SKILL_ATTRIBUTE_CHARGE_ATTACK)
            return true;

        return false;
    }

    auto python_skill::skill_data::is_only_for_guild_war() -> bool {
        if (skill_attribute & SKILL_ATTRIBUTE_ONLY_FOR_GUILD_WAR)
            return true;

        return false;
    }

    auto python_skill::skill_data::get_target_range() const -> uint32_t {
        if (skill_attribute & SKILL_ATTRIBUTE_MELEE_ATTACK)
            return MELEE_SKILL_TARGET_RANGE;

        if (skill_attribute & SKILL_ATTRIBUTE_CHARGE_ATTACK)
            return MELEE_SKILL_TARGET_RANGE;

        return target_range;
    }

    auto python_skill::skill_data::can_change_direction() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_CAN_CHANGE_DIRECTION);
    }

    auto python_skill::skill_data::is_fan_range() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_FAN_RANGE);
    }

    auto python_skill::skill_data::is_circle_range() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_CIRCLE_RANGE);
    }

    auto python_skill::skill_data::is_auto_search_target() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_SEARCH_TARGET);
    }

    auto python_skill::skill_data::is_need_corpse() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_NEED_CORPSE);
    }

    auto python_skill::skill_data::is_need_target() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_NEED_TARGET);
    }

    auto python_skill::skill_data::is_toggle_skill() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_TOGGLE);
    }

    auto python_skill::skill_data::is_use_hp_skill() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_USE_HP);
    }

    auto python_skill::skill_data::is_standing_skill() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_STANDING_SKILL);
    }

    auto python_skill::skill_data::can_use_weapon_type(uint32_t weapon_type) -> bool {
        if (!(skill_attribute & SKILL_ATTRIBUTE_WEAPON_LIMITATION))
            return true;

        return 0 != (need_weapon & (1 << weapon_type));
    }

    auto python_skill::skill_data::is_only_for_alliance() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_ONLY_FOR_ALLIANCE);
    }

    auto python_skill::skill_data::can_use_for_me() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_CAN_USE_FOR_ME);
    }

    auto python_skill::skill_data::can_use_if_not_enough() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_CAN_USE_IF_NOT_ENOUGH);
    }

    auto python_skill::skill_data::is_need_empty_bottle() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_NEED_EMPTY_BOTTLE);
    }

    auto python_skill::skill_data::is_need_poison_bottle() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_NEED_POISON_BOTTLE);
    }

    auto python_skill::skill_data::is_need_bow() -> bool {
        if (!(skill_attribute & SKILL_ATTRIBUTE_WEAPON_LIMITATION))
            return false;

        return 0 != (need_weapon & SKILL_NEED_WEAPON_BOW);
    }

    auto python_skill::skill_data::is_horse_skill() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_HORSE_SKILL);
    }

    auto python_skill::skill_data::is_moving_skill() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_MOVING_SKILL);
    }

    auto python_skill::skill_data::is_attack_skill() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_ATTACK_SKILL);
    }

    auto python_skill::skill_data::is_time_increase_skill() -> bool {
        return 0 != (skill_attribute & SKILL_ATTRIBUTE_TIME_INCREASE_SKILL);
    }

    auto python_skill::skill_data::get_state(const char *state_name, int *state, int min_max_type) -> bool {
        std::map<std::string, uint32_t> *status_name_map_value = nullptr;

        switch (min_max_type) {
        case VALUE_TYPE_FREE:
            status_name_map_value = &status_name_map;
            break;
        case VALUE_TYPE_MIN:
            status_name_map_value = &new_min_status_name_map;
            break;
        case VALUE_TYPE_MAX:
            status_name_map_value = &new_max_status_name_map;
            break;
        default:
            return false;
            break;
        }

        auto it = status_name_map_value->find(state_name);

        if (it != status_name_map_value->end()) {
            *state = get_python_player().status(it->second);
        } else if (0 == strncmp(state_name, "JeungJi", 7)) {
            *state = 0;

            // Jeungjisul is not supported yet.
        }

        return true;
    }

    auto python_skill::skill_data::process_formula(expression *poly, float skill_level, int min_max_type) -> float {
        if (poly->analyze()) {
            for (uint32_t i = 0; i < uint32_t(poly->var_count()); ++i) {
                const char *var_name = poly->var_name(i);
                float f_state;
                if (!strcmp("SkillPoint", var_name) || !strcmp("k", var_name)) {
                    f_state = skill_level;
                } else {
                    int i_state;
                    if (!get_state(var_name, &i_state, min_max_type))
                        return 0.0f;

                    f_state = float(i_state);

                    if (!strcmp("ar", var_name))
                        f_state /= 100.0f;
                }
                poly->set_var(var_name, f_state);
            }
        } else {
            metin2::log::error("Unknown skill affect formula {}", name_value.c_str());
            return 0.0f;
        }

        return poly->eval();
    }

    auto python_skill::skill_data::affect_description(uint32_t index, float skill_level) -> const char * {
        if (index >= affect_data_vector.size())
            return nullptr;

        const std::string &affect_description = affect_data_vector[index].affect_description;
        const std::string &affect_min_formula = affect_data_vector[index].affect_min_formula;
        const std::string &affect_max_formula = affect_data_vector[index].affect_max_formula;

        expression min_poly;
        expression max_poly;
        min_poly.set_str(affect_min_formula.c_str());
        max_poly.set_str(affect_max_formula.c_str());

        float min_value = process_formula(&min_poly, skill_level);
        float max_value = process_formula(&max_poly, skill_level);

        if (min_value < 0.0)
            min_value = -min_value;
        if (max_value < 0.0)
            max_value = -max_value;

        if (strstr(affect_description.c_str(), "%.0f")) {
            min_value = floorf(min_value);
            max_value = floorf(max_value);
        }

        static char description_value[256 + 1];
        int len = snprintf(description_value, sizeof(description_value), affect_description.c_str(), min_value, max_value);
        if (len >= static_cast<int>(sizeof(description_value))) // Cut on a character boundary.
            description_value[utf8::truncated_size(description_value, sizeof(description_value) - 1)] = '\0';

        return description_value;
    }
    auto python_skill::skill_data::skill_cool_time(float skill_point) -> uint32_t {
        if (cool_time_formula.empty())
            return 0;

        expression poly;
        poly.set_str(cool_time_formula.c_str());

        return uint32_t(process_formula(&poly, skill_point));
    }

    auto python_skill::skill_data::target_count(float skill_point) -> uint32_t {
        if (target_count_formula.empty())
            return 0;

        expression poly;
        poly.set_str(target_count_formula.c_str());

        return uint32_t(process_formula(&poly, skill_point));
    }

    auto python_skill::skill_data::skill_motion_index(int grade) -> uint32_t {
        if (-1 != grade && skill_effect_upgrade_enable) {
            assert(grade >= 0 && grade < SKILL_EFFECT_COUNT);
            if (grade >= 0 && grade < SKILL_EFFECT_COUNT) {
                return grade_data_value[grade].motion_index;
            }
        }

        return motion_index;
    }

    auto python_skill::skill_data::get_max_level() -> uint8_t {
        return max_level;
    }

    auto python_skill::skill_data::get_level_up_point() -> uint8_t {
        return level_up_point;
    }

    auto python_skill::skill_data::is_no_motion() -> bool {
        return no_motion;
    }

    auto python_skill::skill_data::is_can_use_skill() -> bool {
        if (0 != (skill_attribute & SKILL_ATTRIBUTE_PASSIVE))
            return false;

        return true;
    }

    auto python_skill::skill_data::motion_loop_count(float skill_point) -> uint32_t {
        if (motion_loop_count_formula.empty())
            return 0;

        expression poly;
        poly.set_str(motion_loop_count_formula.c_str());

        return uint32_t(process_formula(&poly, skill_point));
    }

    auto python_skill::skill_data::need_sp(float skill_point) -> int {
        if (need_sp_formula.empty())
            return 0;

        expression poly;
        poly.set_str(need_sp_formula.c_str());

        return int(process_formula(&poly, skill_point));
    }

    auto python_skill::skill_data::continuation_sp(float skill_point) -> uint32_t {
        if (continuation_sp_formula.empty())
            return 0;

        expression poly;
        poly.set_str(continuation_sp_formula.c_str());

        return uint32_t(process_formula(&poly, skill_point));
    }

    auto python_skill::skill_data::get_duration(float skill_point) -> uint32_t {
        if (duration.empty())
            return 0;

        expression poly;
        poly.set_str(duration.c_str());

        return uint32_t(process_formula(&poly, skill_point));
    }

    python_skill::skill_data::skill_data() {
        type_value = SKILL_TYPE_ACTIVE;
        max_level = 20;
        level_up_point = 1;
        skill_attribute = 0;
        need_weapon = 0;
        target_range = 0;
        cool_time_formula = "";
        motion_loop_count_formula = "";
        need_sp_formula = "";
        continuation_sp_formula = "";
        is_requirement = false;
        require_skill_name = "";
        require_skill_level = 0;
        duration = "";
        level_limit = 0;
        no_motion = false;

        name_value = "";
        image = nullptr;

        for (int j = 0; j < SKILL_GRADE_COUNT; ++j) {
            grade_data &grade_data = grade_data_value[j];
            grade_data.name_value = "";
            grade_data.image = nullptr;
            grade_data.motion_index = 0;
        }
    }

    auto skill_load_skill_data(PyObject *self, PyObject *args) -> PyObject * {

        return py_build_none();
    }

    auto skill_register_skill_desc(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name))
            return py_bad_argument();

        if (!get_python_skill().register_skill_desc(file_name))
            return py_build_exception("Failed to load Skill Desc");

        return py_build_none();
    }

    auto skill_clear_skill_data(PyObject *self, PyObject *args) -> PyObject * {
        python_skill::skill_data::time_increase_skill_number = 0;
        return py_build_none();
    }

    auto skill_get_skill_name(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        int skill_grade = -1;
        py_tuple_get_integer(args, 1, &skill_grade);

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillName - Failed to find skill by %d", skill_index);

        if (-1 != skill_grade)
            if (skill_grade >= 0 && skill_grade < python_skill::SKILL_GRADE_COUNT) {
                return Py_BuildValue("s", skill_data->grade_data_value[skill_grade].name_value.c_str());
            }

        return Py_BuildValue("s", skill_data->name_value.c_str());
    }

    auto skill_get_skill_description(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillDescription - Failed to find skill by %d", skill_index);

        return Py_BuildValue("s", skill_data->description.c_str());
    }

    auto skill_get_skill_type(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillType - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->type_value);
    }

    auto skill_get_skill_condition_description_count(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillConditionDescriptionCount - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->condition_data_vector.size());
    }

    auto skill_get_skill_condition_description(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        int condition_index;
        if (!py_tuple_get_integer(args, 1, &condition_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillConditionDescription() - Failed to find skill by %d", skill_index);

        if (size_t(condition_index) >= skill_data->condition_data_vector.size())
            return Py_BuildValue("None");

        return Py_BuildValue("s", skill_data->condition_data_vector[condition_index].c_str());
    }

    auto skill_get_skill_affect_description_count(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillAffectDescriptionCount - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->affect_data_vector.size());
    }

    auto skill_get_skill_affect_description(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        int affect_index;
        if (!py_tuple_get_integer(args, 1, &affect_index))
            return py_bad_argument();

        float skill_point;
        if (!py_tuple_get_float(args, 2, &skill_point))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillAffectDescription - Failed to find skill by %d", skill_index);

        return Py_BuildValue("s", skill_data->affect_description(affect_index, skill_point));
    }

    auto skill_get_skill_cool_time(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        float skill_point;
        if (!py_tuple_get_float(args, 1, &skill_point))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillCoolTime - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->skill_cool_time(skill_point));
    }

    auto skill_get_skill_need_sp(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        float skill_point;
        if (!py_tuple_get_float(args, 1, &skill_point))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillNeedSP Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->need_sp(skill_point));
    }

    auto skill_get_skill_continuation_sp(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        float skill_point;
        if (!py_tuple_get_float(args, 1, &skill_point))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillContinuationSP - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->continuation_sp(skill_point));
    }

    auto skill_get_skill_max_level(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillMaxLevel - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->get_max_level());
    }

    auto skill_get_skill_level_up_point(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillLevelUpPoint - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->get_level_up_point());
    }

    auto skill_get_skill_level_limit(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillLevelLimit - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->level_limit);
    }

    auto skill_is_skill_requirement(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.IsSkillRequirement - Failed to find skill by %d", skill_index);

        if (skill_data->is_requirement) {
            python_skill::skill_data *require_skill_data;
            if (!get_python_skill().skill_data_by_name(skill_data->require_skill_name.c_str(), &require_skill_data)) {
                metin2::log::error("Skill {} ({}): required skill {} not found", skill_data->skill_index, skill_data->name_value.c_str(), skill_data->require_skill_name.c_str());
                return Py_BuildValue("i", false);
            }

            uint32_t require_skill_slot_index;
            if (!get_python_player().find_skill_slot_index_by_skill_index(require_skill_data->skill_index, &require_skill_slot_index))
                return Py_BuildValue("i", false);
        }

        return Py_BuildValue("i", skill_data->is_requirement);
    }

    auto skill_get_skill_requirement_data(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillRequirementData - Failed to find skill by %d", skill_index);

        python_skill::skill_data *require_skill_data;
        if (!get_python_skill().skill_data_by_name(skill_data->require_skill_name.c_str(), &require_skill_data))
            return Py_BuildValue("si", 0, "None", 0);

        int ireq_level = (int)ceil(float(skill_data->require_skill_level) / float(std::max<int>(1, require_skill_data->level_up_point)));
        return Py_BuildValue("si", skill_data->require_skill_name.c_str(), ireq_level);
    }

    auto skill_get_skill_require_stat_count(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillRequireStatCount - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->require_stat_data_vector.size());
    }

    auto skill_get_skill_require_stat_data(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();
        int stat_index;
        if (!py_tuple_get_integer(args, 1, &stat_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetSkillRequireStatData - Failed to find skill by %d", skill_index);

        if (size_t(stat_index) >= skill_data->require_stat_data_vector.size())
            return Py_BuildValue("ii", 0, 0);

        const python_skill::require_stat_data &require_stat_data = skill_data->require_stat_data_vector[stat_index];

        return Py_BuildValue("ii", require_stat_data.point, require_stat_data.level_value);
    }

    auto skill_can_level_up_skill(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();
        int skill_level;
        if (!py_tuple_get_integer(args, 1, &skill_level))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.CanLevelUpSkill - Failed to find skill by %d", skill_index);

        if (skill_level >= skill_data->get_max_level())
            return Py_BuildValue("i", false);

        if (skill_data->is_requirement) {
            python_skill::skill_data *require_skill_data;
            if (get_python_skill().skill_data_by_name(skill_data->require_skill_name.c_str(), &require_skill_data)) {
                uint32_t require_skill_slot_index;
                if (get_python_player().find_skill_slot_index_by_skill_index(require_skill_data->skill_index, &require_skill_slot_index)) {
                    int skill_grade = get_python_player().skill_grade(require_skill_slot_index);
                    int skill_level = get_python_player().skill_level(require_skill_slot_index);
                    if (skill_grade <= 0)
                        if (skill_level < skill_data->require_skill_level)
                            return Py_BuildValue("i", false);
                }
            }
        }

        for (uint32_t i = 0; i < skill_data->require_stat_data_vector.size(); ++i) {
            const python_skill::require_stat_data &require_stat_data = skill_data->require_stat_data_vector[i];
            if (get_python_player().status(require_stat_data.point) < require_stat_data.level_value)
                return Py_BuildValue("i", false);
        }

        if (0 != (skill_data->skill_attribute & python_skill::SKILL_ATTRIBUTE_CANNOT_LEVEL_UP))
            return Py_BuildValue("i", false);

        return Py_BuildValue("i", true);
    }

    auto skill_check_requirement_sueccess(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.CheckRequirementSueccess - Failed to find skill by %d", skill_index);

        if (skill_data->is_requirement) {
            python_skill::skill_data *require_skill_data;
            if (get_python_skill().skill_data_by_name(skill_data->require_skill_name.c_str(), &require_skill_data)) {
                uint32_t require_skill_slot_index;
                if (get_python_player().find_skill_slot_index_by_skill_index(require_skill_data->skill_index, &require_skill_slot_index)) {
                    int skill_grade = get_python_player().skill_grade(require_skill_slot_index);
                    int skill_level = get_python_player().skill_level(require_skill_slot_index);
                    if (skill_grade <= 0)
                        if (skill_level < skill_data->require_skill_level)
                            return Py_BuildValue("i", false);
                }
            }
        }

        return Py_BuildValue("i", true);
    }

    auto skill_is_toggle_skill(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.IsToggleSkill - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->is_toggle_skill());
    }

    auto skill_is_use_hp_skill(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.IsUseHPSkill - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->is_use_hp_skill());
    }

    auto skill_is_standing_skill(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.IsStandingSkill - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->is_standing_skill());
    }

    auto skill_can_use_skill(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.CanUseSkill - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->is_can_use_skill());
    }

    auto skill_get_icon_image(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return Py_BuildValue("i", 0); // Returns 0 instead of raising an exception.

        return Py_BuildValue("n", (Py_ssize_t)skill_data->image);
    }

    auto skill_get_icon_image_new(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        int grade_index;
        if (!py_tuple_get_integer(args, 1, &grade_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return Py_BuildValue("i", 0); // Returns 0 instead of raising an exception.

        if (grade_index < 0)
            grade_index = 0;

        if (grade_index >= python_skill::SKILL_GRADE_COUNT)
            grade_index = python_skill::SKILL_GRADE_COUNT - 1;

        return Py_BuildValue("n", (Py_ssize_t)skill_data->grade_data_value[grade_index].image);
    }

    auto skill_get_icon_instance_new(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        int grade_index;
        if (!py_tuple_get_integer(args, 1, &grade_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetIconInstanceNew - Failed to find skill by %d", skill_index);

        if (grade_index < 0)
            grade_index = 0;

        if (grade_index >= python_skill::SKILL_GRADE_COUNT)
            grade_index = python_skill::SKILL_GRADE_COUNT - 1;

        graphic_image_instance *image_instance = graphic_image_instance::new_value();
        image_instance->set_image_pointer(skill_data->grade_data_value[grade_index].image);

        return Py_BuildValue("n", (Py_ssize_t)image_instance);
    }

    auto skill_delete_icon_instance(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle))
            return py_bad_argument();

        graphic_image_instance::delete_value((graphic_image_instance *)handle);
        return py_build_none();
    }

    auto skill_get_new_affect_data_count(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetNewAffectDataCount - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->affect_data_new_vector.size());
    }

    auto skill_get_new_affect_data(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();
        int affect_index;
        if (!py_tuple_get_integer(args, 1, &affect_index))
            return py_bad_argument();
        float skill_level;
        if (!py_tuple_get_float(args, 2, &skill_level))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetNewAffectData - Failed to find skill by %d", skill_index);

        if (affect_index < 0 || size_t(affect_index) >= skill_data->affect_data_new_vector.size())
            return py_build_exception(" skill.GetNewAffectData - Strange AffectIndex %d", affect_index);

        python_skill::affect_data_new &affect_data = skill_data->affect_data_new_vector[affect_index];

        expression min_poly;
        expression max_poly;
        min_poly.set_random(expression::RANDOM_TYPE_FORCE_MIN);
        max_poly.set_random(expression::RANDOM_TYPE_FORCE_MAX);
        min_poly.set_str(affect_data.point_poly.c_str());
        max_poly.set_str(affect_data.point_poly.c_str());
        float min_value = skill_data->process_formula(&min_poly, skill_level, python_skill::VALUE_TYPE_MIN);
        float max_value = skill_data->process_formula(&max_poly, skill_level, python_skill::VALUE_TYPE_MAX);

        return Py_BuildValue("sff", affect_data.point_type.c_str(), min_value, max_value);
    }

    auto skill_get_duration(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_bad_argument();
        float skill_level;
        if (!py_tuple_get_float(args, 1, &skill_level))
            return py_bad_argument();

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return py_build_exception("skill.GetDuration - Failed to find skill by %d", skill_index);

        return Py_BuildValue("i", skill_data->get_duration(skill_level));
    }

    void init_player_skills_module() {
        static PyMethodDef s_methods[] =
            {
                {"LoadSkillData", skill_load_skill_data, METH_VARARGS},
                {"ClearSkillData", skill_clear_skill_data, METH_VARARGS},

                {"GetSkillName", skill_get_skill_name, METH_VARARGS},
                {"GetSkillDescription", skill_get_skill_description, METH_VARARGS},
                {"GetSkillType", skill_get_skill_type, METH_VARARGS},
                {"GetSkillConditionDescriptionCount", skill_get_skill_condition_description_count, METH_VARARGS},
                {"GetSkillConditionDescription", skill_get_skill_condition_description, METH_VARARGS},
                {"GetSkillAffectDescriptionCount", skill_get_skill_affect_description_count, METH_VARARGS},
                {"GetSkillAffectDescription", skill_get_skill_affect_description, METH_VARARGS},
                {"GetSkillCoolTime", skill_get_skill_cool_time, METH_VARARGS},
                {"GetSkillNeedSP", skill_get_skill_need_sp, METH_VARARGS},
                {"GetSkillContinuationSP", skill_get_skill_continuation_sp, METH_VARARGS},
                {"GetSkillMaxLevel", skill_get_skill_max_level, METH_VARARGS},
                {"GetSkillLevelUpPoint", skill_get_skill_level_up_point, METH_VARARGS},
                {"GetSkillLevelLimit", skill_get_skill_level_limit, METH_VARARGS},
                {"IsSkillRequirement", skill_is_skill_requirement, METH_VARARGS},
                {"GetSkillRequirementData", skill_get_skill_requirement_data, METH_VARARGS},
                {"GetSkillRequireStatCount", skill_get_skill_require_stat_count, METH_VARARGS},
                {"GetSkillRequireStatData", skill_get_skill_require_stat_data, METH_VARARGS},
                {"CanLevelUpSkill", skill_can_level_up_skill, METH_VARARGS},
                {"CheckRequirementSueccess", skill_check_requirement_sueccess, METH_VARARGS},
                {"IsToggleSkill", skill_is_toggle_skill, METH_VARARGS},
                {"IsUseHPSkill", skill_is_use_hp_skill, METH_VARARGS},
                {"IsStandingSkill", skill_is_standing_skill, METH_VARARGS},
                {"CanUseSkill", skill_can_use_skill, METH_VARARGS},
                {"GetIconImage", skill_get_icon_image, METH_VARARGS},
                {"GetIconImageNew", skill_get_icon_image_new, METH_VARARGS},
                {"GetIconInstanceNew", skill_get_icon_instance_new, METH_VARARGS},
                {"DeleteIconInstance", skill_delete_icon_instance, METH_VARARGS},

                {"GetNewAffectDataCount", skill_get_new_affect_data_count, METH_VARARGS},
                {"GetNewAffectData", skill_get_new_affect_data, METH_VARARGS},
                {"GetDuration", skill_get_duration, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = init_python_module("metin2.player.skills", s_methods);
        PyModule_AddIntConstant(module_value, "SKILL_TYPE_NONE", python_skill::SKILL_TYPE_NONE);
        PyModule_AddIntConstant(module_value, "SKILL_TYPE_ACTIVE", python_skill::SKILL_TYPE_ACTIVE);
        PyModule_AddIntConstant(module_value, "SKILL_TYPE_SUPPORT", python_skill::SKILL_TYPE_SUPPORT);
        PyModule_AddIntConstant(module_value, "SKILL_TYPE_GUILD", python_skill::SKILL_TYPE_GUILD);
        PyModule_AddIntConstant(module_value, "SKILL_TYPE_HORSE", python_skill::SKILL_TYPE_HORSE);
        PyModule_AddIntConstant(module_value, "SKILL_TYPE_MAX_NUM", python_skill::SKILL_TYPE_MAX_NUM);

        PyModule_AddIntConstant(module_value, "SKILL_GRADE_COUNT", python_skill::SKILL_GRADE_COUNT);
        PyModule_AddIntConstant(module_value, "SKILL_GRADE_STEP_COUNT", python_skill::SKILL_GRADE_STEP_COUNT);
        PyModule_AddIntConstant(module_value, "SKILL_GRADEGAP", python_skill::SKILL_GRADEGAP);
        PyModule_AddIntConstant(module_value, "SKILL_EFFECT_COUNT", python_skill::SKILL_EFFECT_COUNT);
    }
}
