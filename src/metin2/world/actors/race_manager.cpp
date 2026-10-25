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
            vec_pathes.push_back("guild:");
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
                vec_pathes.push_back("guild:");
            } else {
                vec_pathes.push_back("character:npc/");
                vec_pathes.push_back("character:npc2/");
                vec_pathes.push_back("character:monster/");
                vec_pathes.push_back("character:monster2/");
                vec_pathes.push_back("guild:");
            }
        }
        // Exception monsters for the April Fools' event.
        else if (8507 == race || 8510 == race) {
            vec_pathes.push_back("character:monster2/");
            vec_pathes.push_back("character:monster/");
            vec_pathes.push_back("character:npc/");
            vec_pathes.push_back("character:npc2/");
            vec_pathes.push_back("guild:");
        } else if (race > 8000) {
            vec_pathes.push_back("character:monster/");
            vec_pathes.push_back("character:monster2/");
            vec_pathes.push_back("character:npc/");
            vec_pathes.push_back("character:npc2/");
            vec_pathes.push_back("guild:");
        } else if (race > 2000) {
            vec_pathes.push_back("character:monster2/");
            vec_pathes.push_back("character:monster/");
            vec_pathes.push_back("character:npc/");
            vec_pathes.push_back("character:npc2/");
            vec_pathes.push_back("guild:");
        } else if (race >= 1400 && race <= 1700) {
            vec_pathes.push_back("character:monster2/");
            vec_pathes.push_back("character:monster/");
            vec_pathes.push_back("character:npc/");
            vec_pathes.push_back("character:npc2/");
            vec_pathes.push_back("guild:");
        } else {
            vec_pathes.push_back("character:monster/");
            vec_pathes.push_back("character:monster2/");
            vec_pathes.push_back("character:npc/");
            vec_pathes.push_back("character:npc2/");
            vec_pathes.push_back("guild:");
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
            snprintf(shape_file_name, sizeof(shape_file_name), "%sshape.toml", path_name);

            race_data *race_data = race_data::new_value();
            race_data->set_race(race_index);
            if (!race_data->load_race_data(shape_file_name)) {
                metin2::log::error("Race {}: cannot load race data {}", race_index, shape_file_name);
                race_data::delete_value(race_data);
                return nullptr;
            }

            load_race_motion_list(*race_data);

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

            std::string script_file_name = full_path_name;
            script_file_name += c_rst_race_name;
            script_file_name += ".toml";

            if (!race_data->load_race_data(script_file_name.c_str())) {
                if (i != vec_full_path_name.size() - 1) {
                    metin2::log::warn("Race {}: cannot load {}, trying the next path", race_index, script_file_name.c_str());
                    continue;
                }

                metin2::log::error("Race {}: cannot load {}", race_index, script_file_name.c_str());
                race_data::delete_value(race_data);
                return nullptr;
            }

            load_race_motion_list(*race_data);

            return race_data;
        }
        metin2::log::error("Race {}: no path registered", race_index);
        race_data::delete_value(race_data);
        return nullptr;
    }

    void race_manager::load_race_motion_list(race_data &race_data) {
        if (race_data.motion_list().empty()) {
            return;
        }

        race_data.register_motion_mode(race_motion_data::MODE_GENERAL);
        for (const race_data::motion_entry &motion : race_data.motion_list()) {
            race_data.register_motion_data(race_motion_data::MODE_GENERAL, motion.index, motion.file_name.c_str(), motion.percentage);
        }
        race_data.register_normal_attack(race_motion_data::MODE_GENERAL, race_motion_data::NAME_NORMAL_ATTACK);
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
