#include <metin2/client/app/abstract_application.h>
#include <metin2/client/app/abstract_character_manager.h>
#include <metin2/client/app/abstract_chat.h>
#include <metin2/client/app/abstract_player.h>
#include <metin2/client/characters/instance_base.h>
#include <metin2/client/characters/python_non_player.h>
#include <metin2/core/parse.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include "python_network_stream.h"

#include <metin2/window/events.h>
#include <strings.h>

namespace metin2::inline client::inline protocol {

    // Server commands are UTF-8; separators are ASCII blanks, which never occur inside a sequence.
    static auto is_blank(char c) -> bool {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
    }

    void skip_spaces(char **string) {
        for (; **string != '\0' && is_blank(**string); ++(*string))
            ;
    }

    auto one_argument(char *argument, char *first_arg) -> char * {
        char mark = false;

        if (!argument) {
            *first_arg = '\0';
            return nullptr;
        }

        skip_spaces(&argument);

        while (*argument) {
            if (*argument == '\"') {
                mark = !mark;
                ++argument;
                continue;
            }

            if (!mark && is_blank(*argument))
                break;

            *(first_arg++) = ascii_tolower(*argument);
            ++argument;
        }

        *first_arg = '\0';

        skip_spaces(&argument);
        return (argument);
    }

    void append_monster_list(const python_non_player::mob_table_list &mob_table_list, const char *header, int type) {
        uint32_t monster_count = 0;
        std::string monster_list = header;

        auto itor = mob_table_list.begin();
        for (; itor != mob_table_list.end(); ++itor) {
            const python_non_player::mob_table *mob_table = *itor;
            if (type == mob_table->rank) {
                if (monster_count != 0)
                    monster_list += ", ";
                monster_list += mob_table->locale_name;
                if (++monster_count > 5)
                    break;
            }
        }
        if (monster_count > 0) {
            abstract_chat &chat = get_abstract_chat();
            chat.append_chat(CHAT_TYPE_INFO, monster_list.c_str());
        }
    }

    auto python_network_stream::client_command(const char *command) -> bool {
        return false;
    }

    auto split_token(const char *c_sz_line, token_vector *token_vector, const char *delimeter = " ") -> bool {
        token_vector->reserve(10);
        token_vector->clear();

        std::string token;
        std::string str_line = c_sz_line;

        uint32_t base_pos = 0;

        do {
            int begin_pos = str_line.find_first_not_of(delimeter, base_pos);
            if (begin_pos < 0)
                return false;

            int end_pos;

            if (str_line[begin_pos] == '"') {
                ++begin_pos;
                end_pos = str_line.find_first_of("\"", begin_pos);

                if (end_pos < 0)
                    return false;

                base_pos = end_pos + 1;
            } else {
                end_pos = str_line.find_first_of(delimeter, begin_pos);
                base_pos = end_pos;
            }

            token_vector->push_back(str_line.substr(begin_pos, end_pos - begin_pos));

            // Stop when only delimiters remain, such as a trailing tab.
            if (int(str_line.find_first_not_of(delimeter, base_pos)) < 0)
                break;
        } while (base_pos < str_line.length());

        return true;
    }

    void python_network_stream::server_command(char *command) {
        // The console window is disabled.
        if (strcasecmp(command, "ConsoleEnable") == 0)
            return;

        if (apo_phase_wnd_[PHASE_WINDOW_GAME]) {
            bool is_true;
            if (py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
                    "server_command_run",
                    Py_BuildValue("(s)", command),
                    &is_true)) {
                if (is_true)
                    return;
            }
        } else if (ser_command_parser_wnd_) {
            bool is_true;
            if (py_call_class_member_func(ser_command_parser_wnd_,
                    "server_command_run",
                    Py_BuildValue("(s)", command),
                    &is_true)) {
                if (is_true)
                    return;
            }
        }

        token_vector token_vector;
        if (!split_token(command, &token_vector))
            return;
        if (token_vector.empty())
            return;

        const char *cmd = token_vector[0].c_str();

        if (!strcasecmp(cmd, "quit")) {
            window::request_quit();
        } else if (!strcasecmp(cmd, "BettingMoney")) {
            if (2 != token_vector.size()) {
                metin2::log::warn("Server command {}: unexpected parameter count {}", command, token_vector.size());
                return;
            }

        } else if (!strcasecmp(cmd, "gift")) {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "gift_show", Py_BuildValue("()"));
        } else if (!strcasecmp(cmd, "cube")) {
            if (token_vector.size() < 2) {
                metin2::log::warn("Server command {}: unexpected parameter count {}", command, token_vector.size());
                return;
            }

            if ("open" == token_vector[1]) {
                if (3 > token_vector.size()) {
                    metin2::log::warn("Server command {}: unexpected parameter count {}", command, token_vector.size());
                    return;
                }

                auto npc_vnum = (uint32_t)metin2::to_number<int>(token_vector[2].c_str());
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "cube_open", Py_BuildValue("(i)", npc_vnum));
            } else if ("close" == token_vector[1]) {
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "cube_close", Py_BuildValue("()"));
            } else if ("info" == token_vector[1]) {
                if (5 != token_vector.size()) {
                    metin2::log::warn("Server command {}: unexpected parameter count {}", command, token_vector.size());
                    return;
                }

                uint32_t gold = metin2::to_number<int>(token_vector[2].c_str());
                uint32_t item_vnum = metin2::to_number<int>(token_vector[3].c_str());
                uint32_t count = metin2::to_number<int>(token_vector[4].c_str());
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "cube_update_info", Py_BuildValue("(iii)", gold, item_vnum, count));
            } else if ("success" == token_vector[1]) {
                if (4 != token_vector.size()) {
                    metin2::log::warn("Server command {}: unexpected parameter count {}", command, token_vector.size());
                    return;
                }

                uint32_t item_vnum = metin2::to_number<int>(token_vector[2].c_str());
                uint32_t count = metin2::to_number<int>(token_vector[3].c_str());
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "cube_succeed", Py_BuildValue("(ii)", item_vnum, count));
            } else if ("fail" == token_vector[1]) {
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "cube_failed", Py_BuildValue("()"));
            } else if ("r_list" == token_vector[1]) {
                // Result list: /cube r_list <npc vnum> <result count> <result text>, where the
                // results are separated by "/", for example: 20383 4 72723,1/72725,1/72730.1/50001,5
                if (5 != token_vector.size()) {
                    metin2::log::warn("Server command {}: unexpected parameter count, expected {}", command, 5);
                    return;
                }

                auto npc_vnum = (uint32_t)metin2::to_number<int>(token_vector[2].c_str());

                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "cube_result_list", Py_BuildValue("(is)", npc_vnum, token_vector[4].c_str()));
            } else if ("m_info" == token_vector[1]) {
                // Material list: /cube m_info <request start index> <result count> <material text>, where
                // the material text holds the materials of every item in the range, for example:
                // 0 5 125,1|126,2|127,2|123,5&555,5&555,4/120000

                if (5 != token_vector.size()) {
                    metin2::log::warn("Server command {}: unexpected parameter count, expected {}", command, 5);
                    return;
                }

                auto request_start_index = (uint32_t)metin2::to_number<int>(token_vector[2].c_str());
                auto result_count = (uint32_t)metin2::to_number<int>(token_vector[3].c_str());

                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "cube_material_info", Py_BuildValue("(iis)", request_start_index, result_count, token_vector[4].c_str()));
            }
        } else if (!strcasecmp(cmd, "ObserverCount")) {
            if (2 != token_vector.size()) {
                metin2::log::warn("Server command {}: unexpected parameter count {}", command, token_vector.size());
                return;
            }

            uint32_t observer_count = metin2::to_number<int>(token_vector[1].c_str());

            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
                "betting_guild_war_update_observer_count",
                Py_BuildValue("(i)", observer_count));
        } else if (!strcasecmp(cmd, "ObserverMode")) {
            if (2 != token_vector.size()) {
                metin2::log::warn("Server command {}: unexpected parameter count {}", command, token_vector.size());
                return;
            }

            uint32_t mode = metin2::to_number<int>(token_vector[1].c_str());

            abstract_player &rk_player = get_abstract_player();
            rk_player.set_observer_mode(mode ? true : false);

            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
                "betting_guild_war_set_observer_mode",
                Py_BuildValue("(i)", mode));
        } else if (!strcasecmp(cmd, "ObserverTeamInfo")) {
        } else if (!strcasecmp(cmd, "StoneDetect")) {
            if (4 != token_vector.size()) {
                metin2::log::warn("Server command {}: unexpected parameter count {}", command, token_vector.size());
                return;
            }

            // Arguments: vid, distance (1-3), angle (0-360).
            uint32_t dw_vid = metin2::to_number<int>(token_vector[1].c_str());
            uint8_t distance = metin2::to_number<int>(token_vector[2].c_str());
            float angle = metin2::to_number<double>(token_vector[3].c_str());
            angle = fmod(540.0f - angle, 360.0f);
            metin2::log::debug("Stone detected (vid {}, distance {}, angle {} -> {:.6f})", dw_vid, distance, metin2::to_number<int>(token_vector[3].c_str()), angle);

            abstract_character_manager &chr_mgr = get_abstract_character_manager();

            instance_base *instance = chr_mgr.instance_ptr(dw_vid);
            if (!instance) {
                metin2::log::warn("Server command {}: character not found", command);
                return;
            }

            pixel_position pixel_position;
            math::vector3 rotation(0.0f, 0.0f, angle);
            instance->new_get_pixel_position(&pixel_position);

            pixel_position.y *= -1.0f;

            switch (distance) {
            case 0:
                get_effect_manager().register_effect("effect:etc/firecracker/find_out.toml");
                get_effect_manager().create_effect("effect:etc/firecracker/find_out.toml", pixel_position, rotation);
                break;
            case 1:
                get_effect_manager().register_effect("effect:etc/compass/appear_small.toml");
                get_effect_manager().create_effect("effect:etc/compass/appear_small.toml", pixel_position, rotation);
                break;
            case 2:
                get_effect_manager().register_effect("effect:etc/compass/appear_middle.toml");
                get_effect_manager().create_effect("effect:etc/compass/appear_middle.toml", pixel_position, rotation);
                break;
            case 3:
                get_effect_manager().register_effect("effect:etc/compass/appear_large.toml");
                get_effect_manager().create_effect("effect:etc/compass/appear_large.toml", pixel_position, rotation);
                break;
            default:
                metin2::log::warn("Server command {}: unknown distance", command);
                break;
            }

        } else if (!strcasecmp(cmd, "StartStaminaConsume")) {
            if (3 != token_vector.size()) {
                metin2::log::warn("Server command {}: unexpected parameter count {}", command, token_vector.size());
                return;
            }

            uint32_t consume_per_sec = metin2::to_number<int>(token_vector[1].c_str());
            uint32_t current_stamina = metin2::to_number<int>(token_vector[2].c_str());

            abstract_player &r_player = get_abstract_player();
            r_player.start_stamina_consume(consume_per_sec, current_stamina);
        }

        else if (!strcasecmp(cmd, "StopStaminaConsume")) {
            if (2 != token_vector.size()) {
                metin2::log::warn("Server command {}: unexpected parameter count {}", command, token_vector.size());
                return;
            }

            uint32_t current_stamina = metin2::to_number<int>(token_vector[1].c_str());

            abstract_player &player = get_abstract_player();
            player.stop_stamina_consume(current_stamina);
        } else if (!strcasecmp(cmd, "messenger_auth")) {
            const std::string &name = token_vector[1].c_str();
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_messenger_add_friend_question", Py_BuildValue("(s)", name.c_str()));
        } else if (!strcasecmp(cmd, "combo")) {
            int flag = metin2::to_number<int>(token_vector[1].c_str());
            abstract_player &player = get_abstract_player();
            player.set_combo_skill_flag(flag);
            combo_skill_flag_ = flag ? true : false;
        } else if (!strcasecmp(cmd, "setblockmode")) {
            int flag = metin2::to_number<int>(token_vector[1].c_str());
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_block_mode", Py_BuildValue("(i)", flag));
        } else if (!strcasecmp(cmd, "french_kiss")) {
            int vid1 = metin2::to_number<int>(token_vector[1].c_str());
            int vid2 = metin2::to_number<int>(token_vector[2].c_str());

            abstract_character_manager &chr_mgr = get_abstract_character_manager();
            instance_base *instance1 = chr_mgr.instance_ptr(vid1);
            instance_base *instance2 = chr_mgr.instance_ptr(vid2);
            if (instance1 && instance2)
                instance1->act_dual_emotion(*instance2, race_motion_data::NAME_FRENCH_KISS_START, race_motion_data::NAME_FRENCH_KISS_START);
        } else if (!strcasecmp(cmd, "kiss")) {
            int vid1 = metin2::to_number<int>(token_vector[1].c_str());
            int vid2 = metin2::to_number<int>(token_vector[2].c_str());

            abstract_character_manager &chr_mgr = get_abstract_character_manager();
            instance_base *instance1 = chr_mgr.instance_ptr(vid1);
            instance_base *instance2 = chr_mgr.instance_ptr(vid2);
            if (instance1 && instance2)
                instance1->act_dual_emotion(*instance2, race_motion_data::NAME_KISS_START, race_motion_data::NAME_KISS_START);
        } else if (!strcasecmp(cmd, "slap")) {
            int vid1 = metin2::to_number<int>(token_vector[1].c_str());
            int vid2 = metin2::to_number<int>(token_vector[2].c_str());

            abstract_character_manager &chr_mgr = get_abstract_character_manager();
            instance_base *instance1 = chr_mgr.instance_ptr(vid1);
            instance_base *instance2 = chr_mgr.instance_ptr(vid2);
            if (instance1 && instance2)
                instance1->act_dual_emotion(*instance2, race_motion_data::NAME_SLAP_HURT_START, race_motion_data::NAME_SLAP_HIT_START);
        } else if (!strcasecmp(cmd, "clap")) {
            int i_vid = metin2::to_number<int>(token_vector[1].c_str());
            abstract_character_manager &chr_mgr = get_abstract_character_manager();
            instance_base *instance = chr_mgr.instance_ptr(i_vid);
            if (instance)
                instance->act_emotion(race_motion_data::NAME_CLAP);
        } else if (!strcasecmp(cmd, "cheer1")) {
            int vid = metin2::to_number<int>(token_vector[1].c_str());
            abstract_character_manager &chr_mgr = get_abstract_character_manager();
            instance_base *instance = chr_mgr.instance_ptr(vid);
            if (instance)
                instance->act_emotion(race_motion_data::NAME_CHEERS_1);
        } else if (!strcasecmp(cmd, "cheer2")) {
            int vid = metin2::to_number<int>(token_vector[1].c_str());
            abstract_character_manager &chr_mgr = get_abstract_character_manager();
            instance_base *instance = chr_mgr.instance_ptr(vid);
            if (instance)
                instance->act_emotion(race_motion_data::NAME_CHEERS_2);
        } else if (!strcasecmp(cmd, "dance1")) {
            int vid = metin2::to_number<int>(token_vector[1].c_str());
            abstract_character_manager &chr_mgr = get_abstract_character_manager();
            instance_base *instance = chr_mgr.instance_ptr(vid);
            if (instance)
                instance->act_emotion(race_motion_data::NAME_DANCE_1);
        } else if (!strcasecmp(cmd, "dance2")) {
            int vid = metin2::to_number<int>(token_vector[1].c_str());
            abstract_character_manager &chr_mgr = get_abstract_character_manager();
            instance_base *instance = chr_mgr.instance_ptr(vid);
            if (instance)
                instance->act_emotion(race_motion_data::NAME_DANCE_2);
        } else if (!strcasecmp(cmd, "dig_motion")) {
            int vid = metin2::to_number<int>(token_vector[1].c_str());
            abstract_character_manager &chr_mgr = get_abstract_character_manager();
            instance_base *instance = chr_mgr.instance_ptr(vid);
            if (instance)
                instance->act_emotion(race_motion_data::NAME_DIG);
        } else {
            static std::map<std::string, int> emotion_dict;

            static bool is_first = true;
            if (is_first) {
                is_first = false;

                emotion_dict["dance3"] = race_motion_data::NAME_DANCE_3;
                emotion_dict["dance4"] = race_motion_data::NAME_DANCE_4;
                emotion_dict["dance5"] = race_motion_data::NAME_DANCE_5;
                emotion_dict["dance6"] = race_motion_data::NAME_DANCE_6;
                emotion_dict["congratulation"] = race_motion_data::NAME_CONGRATULATION;
                emotion_dict["forgive"] = race_motion_data::NAME_FORGIVE;
                emotion_dict["angry"] = race_motion_data::NAME_ANGRY;
                emotion_dict["attractive"] = race_motion_data::NAME_ATTRACTIVE;
                emotion_dict["sad"] = race_motion_data::NAME_SAD;
                emotion_dict["shy"] = race_motion_data::NAME_SHY;
                emotion_dict["cheerup"] = race_motion_data::NAME_CHEERUP;
                emotion_dict["banter"] = race_motion_data::NAME_BANTER;
                emotion_dict["joy"] = race_motion_data::NAME_JOY;
            }

            auto f = emotion_dict.find(cmd);
            if (f == emotion_dict.end()) {
                metin2::log::warn("Server command {}: unknown command {}", command, cmd);
            } else {
                int emotion_index = f->second;

                int vid = metin2::to_number<int>(token_vector[1].c_str());
                abstract_character_manager &chr_mgr = get_abstract_character_manager();
                instance_base *instance = chr_mgr.instance_ptr(vid);

                if (instance)
                    instance->act_emotion(emotion_index);
            }
        }
    }
}
