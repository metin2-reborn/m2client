#include "race_manager.h"
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>
#include "race_motion_data.h"

namespace metin2::inline world::inline actors {

    auto is_guild_race(unsigned race) -> bool {
        if (race >= 14000 && race < 15000)
            return true;

        if (20043 == race)
            return true;

        return false;
    }

    auto is_npc_race(unsigned race) -> bool {
        if (race > 9000)
            return true;

        return false;
    }

    void get_race_resource_pathes(unsigned race, std::vector<std::string> &vec_pathes) {
        if (is_guild_race(race)) {
            vec_pathes.push_back("d:/ymir work/guild/");
            vec_pathes.push_back("character:npc/");
            vec_pathes.push_back("character:npc2/");
            vec_pathes.push_back("character:monster/");
            vec_pathes.push_back("character:monster2/");
        } else if (is_npc_race(race)) {
            if (race >= 30000) {
                vec_pathes.push_back("character:npc2/");
                vec_pathes.push_back("character:npc/");
                vec_pathes.push_back("character:monster/");
                vec_pathes.push_back("character:monster2/");
                vec_pathes.push_back("d:/ymir work/guild/");
            } else {
                vec_pathes.push_back("character:npc/");
                vec_pathes.push_back("character:npc2/");
                vec_pathes.push_back("character:monster/");
                vec_pathes.push_back("character:monster2/");
                vec_pathes.push_back("d:/ymir work/guild/");
            }
        }
        // Exception monsters for the April Fools' event.
        else if (8507 == race || 8510 == race) {
            vec_pathes.push_back("character:monster2/");
            vec_pathes.push_back("character:monster/");
            vec_pathes.push_back("character:npc/");
            vec_pathes.push_back("character:npc2/");
            vec_pathes.push_back("d:/ymir work/guild/");
        } else if (race > 8000) {
            vec_pathes.push_back("character:monster/");
            vec_pathes.push_back("character:monster2/");
            vec_pathes.push_back("character:npc/");
            vec_pathes.push_back("character:npc2/");
            vec_pathes.push_back("d:/ymir work/guild/");
        } else if (race > 2000) {
            vec_pathes.push_back("character:monster2/");
            vec_pathes.push_back("character:monster/");
            vec_pathes.push_back("character:npc/");
            vec_pathes.push_back("character:npc2/");
            vec_pathes.push_back("d:/ymir work/guild/");
        } else if (race >= 1400 && race <= 1700) {
            vec_pathes.push_back("character:monster2/");
            vec_pathes.push_back("character:monster/");
            vec_pathes.push_back("character:npc/");
            vec_pathes.push_back("character:npc2/");
            vec_pathes.push_back("d:/ymir work/guild/");
        } else {
            vec_pathes.push_back("character:monster/");
            vec_pathes.push_back("character:monster2/");
            vec_pathes.push_back("character:npc/");
            vec_pathes.push_back("character:npc2/");
            vec_pathes.push_back("d:/ymir work/guild/");
        }
        return;
    }

    auto race_manager::load_race_data_impl(uint32_t race_index) -> race_data * {
        auto f_race_name = map_race_key_race_name_.find(race_index);
        if (map_race_key_race_name_.end() == f_race_name)
            return nullptr;

        const std::string &c_rst_race_name = f_race_name->second;

        if (c_rst_race_name.empty())
            return nullptr;

        if (c_rst_race_name[0] == '#') {
            const char *path_name = c_rst_race_name.c_str() + 1;
            char shape_file_name[256];
            char motion_list_file_name[256];
            snprintf(shape_file_name, sizeof(shape_file_name), "%sshape.msm", path_name);
            snprintf(motion_list_file_name, sizeof(motion_list_file_name), "%smotlist.txt", path_name);

            race_data *race_data = race_data::new_value();
            race_data->set_race(race_index);
            if (!race_data->load_race_data(shape_file_name)) {
                metin2::log::error("Race {}: cannot load race data {}", race_index, shape_file_name);
                race_data::delete_value(race_data);
                return nullptr;
            }

            load_race_motion_list(*race_data, path_name, motion_list_file_name);

            return race_data;
        }
        std::vector<std::string> vec_full_path_name;
        get_race_resource_pathes(race_index, vec_full_path_name);

        race_data *race_data = race_data::new_value();
        race_data->set_race(race_index);

        for (size_t i = 0; i < vec_full_path_name.size(); i++) {
            std::string full_path_name = vec_full_path_name[i];
            {
                auto race_src_name = map_race_name_src_name_.find(c_rst_race_name);
                if (map_race_name_src_name_.end() == race_src_name)
                    full_path_name += c_rst_race_name;
                else
                    full_path_name += race_src_name->second;
            }

            full_path_name += "/";

            std::string msm_file_name = full_path_name;
            msm_file_name += c_rst_race_name;
            msm_file_name += ".msm";

            if (!race_data->load_race_data(msm_file_name.c_str())) {
                if (i != vec_full_path_name.size() - 1) {
                    metin2::log::warn("Race {}: cannot load {}, trying the next path", race_index, msm_file_name.c_str());
                    continue;
                }

                metin2::log::error("Race {}: cannot load {}", race_index, msm_file_name.c_str());
                race_data::delete_value(race_data);
                return nullptr;
            }

            std::string st_motion_list_file_name = full_path_name;
            st_motion_list_file_name += race_data->motion_list_file_name();

            load_race_motion_list(*race_data, full_path_name.c_str(), st_motion_list_file_name.c_str());

            return race_data;
        }
        metin2::log::error("Race {}: no path registered", race_index);
        race_data::delete_value(race_data);
        return nullptr;
    }

    auto race_manager::load_race_motion_list(race_data &race_data, const char *path_name, const char *motion_list_file_name) -> bool {
        static std::map<std::string, uint32_t> map_type_index;
        static bool is_init = false;

        if (!is_init) {
            is_init = true;

            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SPAWN", race_motion_data::NAME_SPAWN));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("WAIT", race_motion_data::NAME_WAIT));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("WAIT1", race_motion_data::NAME_WAIT));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("WAIT2", race_motion_data::NAME_WAIT));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("WALK", race_motion_data::NAME_WALK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("WALK1", race_motion_data::NAME_WALK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("WALK2", race_motion_data::NAME_WALK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("RUN", race_motion_data::NAME_RUN));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("RUN1", race_motion_data::NAME_RUN));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("RUN2", race_motion_data::NAME_RUN));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("STOP", race_motion_data::NAME_STOP));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("DEAD", race_motion_data::NAME_DEAD));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("COMBO_ATTACK", race_motion_data::NAME_COMBO_ATTACK_1));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("COMBO_ATTACK1", race_motion_data::NAME_COMBO_ATTACK_2));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("COMBO_ATTACK2", race_motion_data::NAME_COMBO_ATTACK_3));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("NORMAL_ATTACK", race_motion_data::NAME_NORMAL_ATTACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("NORMAL_ATTACK1", race_motion_data::NAME_NORMAL_ATTACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("NORMAL_ATTACK2", race_motion_data::NAME_NORMAL_ATTACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("FRONT_DAMAGE", race_motion_data::NAME_DAMAGE));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("FRONT_DAMAGE1", race_motion_data::NAME_DAMAGE));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("FRONT_DAMAGE2", race_motion_data::NAME_DAMAGE));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("FRONT_DAMAGE3", race_motion_data::NAME_DAMAGE));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("FRONT_DEAD", race_motion_data::NAME_DEAD));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("FRONT_DEAD1", race_motion_data::NAME_DEAD));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("FRONT_DEAD2", race_motion_data::NAME_DEAD));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("FRONT_KNOCKDOWN", race_motion_data::NAME_DAMAGE_FLYING));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("FRONT_KNOCKDOWN1", race_motion_data::NAME_DAMAGE_FLYING));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("FRONT_STANDUP", race_motion_data::NAME_STAND_UP));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("FRONT_STANDUP1", race_motion_data::NAME_STAND_UP));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("BACK_DAMAGE", race_motion_data::NAME_DAMAGE_BACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("BACK_DAMAGE1", race_motion_data::NAME_DAMAGE_BACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("BACK_DEAD", race_motion_data::NAME_DEAD_BACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("BACK_DEAD1", race_motion_data::NAME_DEAD_BACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("BACK_DEAD2", race_motion_data::NAME_DEAD_BACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("BACK_KNOCKDOWN", race_motion_data::NAME_DAMAGE_FLYING_BACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("BACK_KNOCKDOWN1", race_motion_data::NAME_DAMAGE_FLYING_BACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("BACK_STANDUP", race_motion_data::NAME_STAND_UP_BACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("BACK_STANDUP1", race_motion_data::NAME_STAND_UP_BACK));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SPECIAL", race_motion_data::NAME_SPECIAL_1));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SPECIAL1", race_motion_data::NAME_SPECIAL_2));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SPECIAL2", race_motion_data::NAME_SPECIAL_3));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SPECIAL3", race_motion_data::NAME_SPECIAL_4));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SPECIAL4", race_motion_data::NAME_SPECIAL_5));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SPECIAL5", race_motion_data::NAME_SPECIAL_6));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SKILL1", race_motion_data::NAME_SKILL + 121));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SKILL2", race_motion_data::NAME_SKILL + 122));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SKILL3", race_motion_data::NAME_SKILL + 123));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SKILL4", race_motion_data::NAME_SKILL + 124));
            map_type_index.insert(std::map<std::string, uint32_t>::value_type("SKILL5", race_motion_data::NAME_SKILL + 125));
        }

        const void *data;
        mapped_file mapped_file;
        if (!get_file_manager().get(mapped_file, motion_list_file_name, &data))
            return false;

        memory_text_file_loader text_file_loader;
        text_file_loader.bind(mapped_file.size(), data);

        race_data.register_motion_mode(race_motion_data::MODE_GENERAL);

        char mode[256];
        char type[256];
        char file[256];
        int percent = 0;

        bool is_spawn = false;

        static std::string spawn_motion_file_name;
        static std::string motion_file_name;

        spawn_motion_file_name = "";
        motion_file_name = "";

        uint32_t line_count = text_file_loader.line_count();
        for (uint32_t line_index = 0; line_index < line_count; ++line_index) {
            uint32_t motion_type = race_motion_data::NAME_NONE;

            const std::string &line = text_file_loader.line_string(line_index);
            sscanf(line.c_str(), "%s %s %s %d", mode, type, file, &percent);

            auto type_index = map_type_index.find(type);

            if (map_type_index.end() == type_index) {
                // When the motion list has entries like WAIT, WAIT4 and WAIT20,
                // WAIT4 and WAIT20 are recognised as WAIT.
                const size_t cut_length_limit = 2;
                bool found = false;

                if (cut_length_limit < strlen(type) + 1) {
                    for (size_t i = 1; i <= cut_length_limit; ++i) {
                        std::string type_name = std::string(type).substr(0, strlen(type) - i);
                        type_index = map_type_index.find(type_name);

                        if (map_type_index.end() != type_index) {
                            found = true;
                            break;
                        }
                    }
                }

                if (false == found)
                    continue;
            }

            motion_type = type_index->second;

            motion_file_name = path_name;
            motion_file_name += file;

            race_data.register_motion_data(race_motion_data::MODE_GENERAL, motion_type, motion_file_name.c_str(), percent);

            switch (motion_type) {
            case race_motion_data::NAME_SPAWN:
                is_spawn = true;
                break;
            case race_motion_data::NAME_DAMAGE:
                spawn_motion_file_name = motion_file_name;
                break;
            }
        }

        if (!is_spawn && spawn_motion_file_name != "") {
            race_data.register_motion_data(race_motion_data::MODE_GENERAL, race_motion_data::NAME_SPAWN, spawn_motion_file_name.c_str(), percent);
        }

        race_data.register_normal_attack(race_motion_data::MODE_GENERAL, race_motion_data::NAME_NORMAL_ATTACK);

        return true;
    }

    void race_manager::register_race_src_name(const char *name, const char *src_name) {
        map_race_name_src_name_.insert(std::map<std::string, std::string>::value_type(name, src_name));
    }

    void race_manager::register_race_name(uint32_t race_index, const char *name) {
        map_race_key_race_name_.insert(std::map<uint32_t, std::string>::value_type(race_index, name));
    }

    void race_manager::create_race(uint32_t race_index) {
        if (race_data_map_value_.end() != race_data_map_value_.find(race_index)) {
            metin2::log::error("Race {} already created", race_index);
            return;
        }

        race_data *race_data = race_data::new_value();
        race_data->set_race(race_index);
        race_data_map_value_.insert(race_data_map::value_type(race_index, race_data));

        metin2::log::debug("Creating race {}", race_index);
    }

    void race_manager::select_race(uint32_t race_index) {
        auto itor = race_data_map_value_.find(race_index);
        if (race_data_map_value_.end() == itor) {
            assert(!"Failed to select race data!");
            return;
        }

        selected_race_data_ = itor->second;
    }

    auto race_manager::selected_race_data_pointer() -> race_data * {
        return selected_race_data_;
    }

    auto race_manager::race_data_pointer(uint32_t race_index, race_data **pp_race_data) -> bool {
        auto itor = race_data_map_value_.find(race_index);

        if (race_data_map_value_.end() == itor) {
            race_data *p_race_data = load_race_data_impl(race_index);

            if (p_race_data) {
                race_data_map_value_.insert(race_data_map::value_type(race_index, p_race_data));
                *pp_race_data = p_race_data;
                return true;
            }

            metin2::log::error("Race {}: cannot load race data", race_index);
            return false;
        }

        *pp_race_data = itor->second;
        return true;
    }

    void race_manager::set_path_name(const char *path_name) {
        path_name_ = path_name;
    }

    auto race_manager::full_path_file_name(const char *c_sz_file_name) -> const char * {
        static std::string s_st_file_name;

        if (c_sz_file_name[0] != '.') {
            s_st_file_name = path_name_;
            s_st_file_name += c_sz_file_name;
        } else {
            s_st_file_name = c_sz_file_name;
        }

        return s_st_file_name.c_str();
    }

    void race_manager::create() {
        race_motion_data::create_system(2048);
        race_data::create_system(256, 512);
    }

    void race_manager::initialize_impl() {
        selected_race_data_ = nullptr;
    }

    void race_manager::destroy_race_data_map() {
        race_data_map::iterator i;
        for (i = race_data_map_value_.begin(); i != race_data_map_value_.end(); ++i)
            race_data::delete_value(i->second);

        race_data_map_value_.clear();
    }

    void race_manager::destroy() {
        destroy_race_data_map();

        initialize_impl();
    }

    race_manager::race_manager() {
        initialize_impl();
    }

    race_manager::~race_manager() {
        destroy();
    }
}
