#include "python_event_manager.h"
#include <metin2/client/characters/python_non_player.h>
#include <metin2/client/protocol/python_network_stream.h>
#include <metin2/core/parse.h>
#include <metin2/core/utf8.h>
#include <metin2/core/utils.h>
#include <metin2/log/log.h>

#include <metin2/client/app/abstract_application.h>

#include <metin2/world/items/item_data.h>
#include <metin2/world/items/item_manager.h>

#include <metin2/files/eter_file_manager.h>

#include <metin2/client/background/python_mini_map.h>
#include <metin2/core/timer.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline quest {

    const int32_t c_lNormal_Waiting_Time = 10;
    const int c_fLine_Temp = 16;

    void show_argument(event_script::arg_list &argument_list) {
        for (auto itor = argument_list.begin(); itor != argument_list.end(); ++itor) {
            const std::string &name = (*itor).name_value;

            metin2::log::debug("Event argument {}", name.c_str());
        }
    }

    auto get_argument_string(const char *c_sz_name, event_script::arg_list &argument_list) -> const std::string & {
        for (auto itor = argument_list.begin(); itor != argument_list.end(); ++itor) {
            const std::string &r_name = (*itor).name_value;

            if (0 == r_name.compare(c_sz_name))
                return (*itor).value;
        }

        static std::string empty_value = "";
        return empty_value;
    }

    auto get_argument(const char *name, event_script::arg_list &argument_list) -> const char * {
        return get_argument_string(name, argument_list).c_str();
    }

    void get_camera_setting_from_arg_list(event_script::arg_list &arg_list, abstract_application::camera_setting *camera_setting) {
        int ix = metin2::to_number<int>(get_argument("x", arg_list));
        int iy = metin2::to_number<int>(get_argument("y", arg_list));
        int iz = metin2::to_number<int>(get_argument("z", arg_list));
        int up_dir = metin2::to_number<int>(get_argument("up", arg_list));
        int view_dir = metin2::to_number<int>(get_argument("view", arg_list));
        int cross_dir = metin2::to_number<int>(get_argument("cross", arg_list));
        int distance = metin2::to_number<int>(get_argument("distance", arg_list));
        int rot = metin2::to_number<int>(get_argument("rot", arg_list));
        int pitch = metin2::to_number<int>(get_argument("pitch", arg_list));

        memset(static_cast<void *>(camera_setting), 0, sizeof(abstract_application::camera_setting));
        camera_setting->center_position.x = float(ix);
        camera_setting->center_position.y = float(iy);
        camera_setting->center_position.z = float(iz);
        camera_setting->cmr_pos.up_dir = float(up_dir);
        camera_setting->cmr_pos.view_dir = float(view_dir);
        camera_setting->cmr_pos.cross_dir = float(cross_dir);
        camera_setting->zoom = float(distance);
        camera_setting->rotation = float(rot);
        camera_setting->pitch = float(pitch);
    }

    void python_event_manager::init_event_set(event_set &event_set_value) {
        event_set_value.ix = 0;
        event_set_value.iy = 0;
        event_set_value.width = 0;
        event_set_value.iy_local = 0;

        event_set_value.is_lock = false;
        event_set_value.last_delay_time = 0;
        event_set_value.current_letter = 0;
        event_set_value.current_color = math::color(1, 1, 1, 1);
        event_set_value.current_line = "";

        event_set_value.current_text_line = nullptr;
        event_set_value.script_text_line_list_value.clear();

        event_set_value.is_confirm_wait = false;
        event_set_value.confirm_time_text_line = nullptr;
        event_set_value.confirm_end_time = 0;

        event_set_value.diffuse_color = math::color(1, 1, 1, 1);
        event_set_value.waiting_time = c_lNormal_Waiting_Time;
        event_set_value.restricted_character_count = 30;

        event_set_value.visible_start_line = 0;
        event_set_value.visible_line_count = BOX_VISIBLE_LINE_COUNT;

        event_set_value.adjust_line = 0;

        event_set_value.is_text_center_mode = false;
        event_set_value.is_wait_flag = false;

        insert_line(event_set_value);
    }

    auto python_event_manager::register_event_set(const char *file_name) -> int {
        mapped_file file;
        const void *map;

        if (!get_file_manager().get(file, file_name, &map))
            return -1;

        std::string event_string;
        event_string.resize(file.size() + 1);

        file.read(&event_string[0], file.size());

        event_set *event_set_value = event_set_pool_.alloc();
        if (!event_set_value)
            return -1;

        if (!event_set_value->script_group.create(event_string)) {
            clear_event_setp(event_set_value);
            return -1;
        }

        utf8::copy_field(event_set_value->file_name, file_name);

        event_set_value->current_text_line = nullptr;
        event_set_value->event_handler_value = nullptr;

        init_event_set(*event_set_value);

        int empty_slot_index = empty_event_set_slot();
        event_set_vector_value_[empty_slot_index] = event_set_value;
        return empty_slot_index;
    }

    auto python_event_manager::register_event_set_from_string(const std::string &script) -> int {
        event_set *event_set_value = event_set_pool_.alloc();
        if (!event_set_value)
            return -1;

        // Reset before parsing, or clear_event_setp fails after a script parse failure.
        event_set_value->current_text_line = nullptr;

        if (!event_set_value->script_group.create(script)) {
            clear_event_setp(event_set_value);
            return -1;
        }

        event_set_value->file_name[0] = 0;
        event_set_value->event_handler_value = nullptr;
        init_event_set(*event_set_value);

        // A script that starts with a run-cinema command is replaced by the cinema file it names.
        event_script::cmd script_command;
        int event_position;
        int event_type_value;
        if (event_set_value->script_group.read_cmd(script_command)) {
            if (script_event_index(script_command.name_value.c_str(), &event_position, &event_type_value)) {
                if (EVENT_TYPE_RUN_CINEMA == event_type_value) {
                    std::string file_name = get_argument("value", script_command.arg_list_value);

                    clear_event_setp(event_set_value);

                    return register_event_set(file_name.c_str());
                }
            }
        }

        int empty_slot_index = empty_event_set_slot();
        event_set_vector_value_[empty_slot_index] = event_set_value;
        return empty_slot_index;
    }

    void python_event_manager::clear_event_seti(int index) {
        if (!check_event_set_index(index))
            return;

        clear_event_setp(event_set_vector_value_[index]);
        event_set_vector_value_[index] = nullptr;
    }

    void python_event_manager::clear_event_setp(event_set *event_set_value) {
        if (!event_set_value)
            return;

        for (auto itor = event_set_value->script_text_line_list_value.begin(); itor != event_set_value->script_text_line_list_value.end(); ++itor) {
            text_line &line = *itor;
            line.instance->destroy();
            script_text_line_pool_.free(line.instance);
        }
        event_set_value->script_text_line_list_value.clear();

        if (event_set_value->current_text_line) {
            event_set_value->current_text_line->destroy();
            script_text_line_pool_.free(event_set_value->current_text_line);
        }
        event_set_value->current_text_line = nullptr;
        event_set_value->current_line = "";
        event_set_value->current_letter = 0;

        event_set_pool_.free(event_set_value);
    }

    auto python_event_manager::empty_event_set_slot() -> uint32_t {
        for (uint32_t i = 0; i < event_set_vector_value_.size(); ++i) {
            if (nullptr == event_set_vector_value_[i]) {
                return i;
            }
        }

        event_set_vector_value_.push_back(nullptr);
        return event_set_vector_value_.size() - 1;
    }

    void python_event_manager::set_restricted_count(int index, int count) {
        if (!check_event_set_index(index))
            return;

        event_set *event_set_value = event_set_vector_value_[index];
        if (!event_set_value) {
            metin2::log::error("Event set {} not found; cannot set the restricted count", index);
            return;
        }

        event_set_value->restricted_character_count = count;
    }

    void python_event_manager::set_event_handler(int index, PyObject *event_handler) {
        if (!check_event_set_index(index))
            return;

        event_set *event_set_value = event_set_vector_value_[index];
        if (!event_set_value) {
            metin2::log::error("Event set {} not found; cannot set the event handler", index);
            return;
        }

        event_set_value->event_handler_value = event_handler;
    }

    auto python_event_manager::event_set_local_y_position(int index) -> int {
        if (!check_event_set_index(index))
            return 0;

        event_set *event_set_value = event_set_vector_value_[index];
        if (!event_set_value)
            return 0;

        return event_set_value->iy_local;
    }

    void python_event_manager::add_event_set_local_y_position(int index, int add_value) {
        if (!check_event_set_index(index))
            return;

        event_set *event_set_value = event_set_vector_value_[index];
        if (!event_set_value)
            return;

        event_set_value->iy_local += add_value;
    }

    void python_event_manager::insert_text(int index, const char *text, int x_pos) {
        if (!check_event_set_index(index))
            return;

        event_set *event_set_value = event_set_vector_value_[index];
        if (!event_set_value)
            return;

        event_set_value->current_line = text;
        if (event_set_value->current_text_line) {
            event_set_value->current_text_line->set_value(text);
            if (x_pos == 0) {
                event_set_value->current_text_line->set_horizonal_align(graphic_text_instance::HORIZONTAL_ALIGN_CENTER);
            }
        }

        event_set_value->current_text_line->update();
        insert_line(*event_set_value, false, x_pos);
    }

    void python_event_manager::update_event_set(int index, int ix, int iy) {
        if (!check_event_set_index(index))
            return;

        event_set *event_set_value = event_set_vector_value_[index];
        if (!event_set_value)
            return;

        event_set_value->ix = ix;
        event_set_value->iy = iy;

        refresh_line_position(event_set_value);

        if (event_set_value->is_confirm_wait) {
            int left_time = std::max(0, event_set_value->confirm_end_time - int(el_timer_get_m_sec() / 1000));
            event_set_value->confirm_time_text_line->set_value(format_string(left_time_string_.c_str(), left_time));
        }

        if (event_set_value->is_wait_flag)
            return;

        int32_t elapsed_time = get_timer().elapsed_milliecond();

        event_set_value->last_delay_time = std::max(0, event_set_value->last_delay_time - elapsed_time);

        while (elapsed_time > 0) {
            event_set_value->last_delay_time -= elapsed_time;

            if (event_set_value->last_delay_time <= 0) {
                elapsed_time = -event_set_value->last_delay_time;
                if (elapsed_time <= 0)
                    break;

                process_event_set(event_set_value);

                if (event_set_value->last_delay_time < 0) {
                    event_set_value->last_delay_time = 0;
                    break;
                }
            } else
                break;
        }
    }

    void python_event_manager::set_event_set_width(int index, int width_value) {
        if (!check_event_set_index(index))
            return;

        event_set *event_set_value = event_set_vector_value_[index];
        if (!event_set_value)
            return;

        event_set_value->width = width_value;
    }

    void python_event_manager::process_event_set(event_set *event_set_value) {
        if (event_set_value->is_lock)
            return;

        event_script::cmd script_command;

        abstract_application &app = get_abstract_application();

        if (!event_set_value->script_group.get_cmd(script_command)) {
            event_set_value->is_lock = true;
            return;
        }

        int event_position;
        int event_type_value;
        if (!script_event_index(script_command.name_value.c_str(), &event_position, &event_type_value))
            return;

        switch (event_type_value) {
        case EVENT_TYPE_LETTER:
            {
                const std::string &value = get_argument_string("value", script_command.arg_list_value);
                event_set_value->current_line.append(value);
                event_set_value->current_text_line->set_value_string(event_set_value->current_line);
                event_set_value->current_text_line->set_color(event_set_value->current_color.r, event_set_value->current_color.g, event_set_value->current_color.b);
                event_set_value->current_letter += utf8::columns(value);

                if (event_set_value->current_letter >= event_set_value->restricted_character_count) {
                    insert_line(*event_set_value);
                }

                event_set_value->last_delay_time = event_set_value->waiting_time;
                break;
            }

        case EVENT_TYPE_DELAY:
            {
                if (EVENT_POSITION_START == event_position)
                    event_set_value->waiting_time = metin2::to_number<int>(get_argument("value", script_command.arg_list_value));
                else
                    event_set_value->waiting_time = c_lNormal_Waiting_Time;
                break;
            }

        case EVENT_TYPE_COLOR:
            {
                if (EVENT_POSITION_START == event_position) {
                    event_set_value->current_color.r = (float)metin2::to_number<double>(get_argument("r", script_command.arg_list_value));
                    event_set_value->current_color.g = (float)metin2::to_number<double>(get_argument("g", script_command.arg_list_value));
                    event_set_value->current_color.b = (float)metin2::to_number<double>(get_argument("b", script_command.arg_list_value));
                    event_set_value->current_color.a = 1.0f;
                } else {
                    event_set_value->current_color.r = 1.0f;
                    event_set_value->current_color.g = 1.0f;
                    event_set_value->current_color.a = 1.0f;
                    event_set_value->current_color.b = 1.0f;
                }
                break;
            }

        case EVENT_TYPE_COLOR256:
            {
                if (EVENT_POSITION_START == event_position) {
                    event_set_value->current_color.r = float(metin2::to_number<double>(get_argument("r", script_command.arg_list_value)) / 255.0f);
                    event_set_value->current_color.g = float(metin2::to_number<double>(get_argument("g", script_command.arg_list_value)) / 255.0f);
                    event_set_value->current_color.b = float(metin2::to_number<double>(get_argument("b", script_command.arg_list_value)) / 255.0f);
                    event_set_value->current_color.a = 1.0f;
                } else {
                    event_set_value->current_color.r = 1.0f;
                    event_set_value->current_color.g = 1.0f;
                    event_set_value->current_color.a = 1.0f;
                    event_set_value->current_color.b = 1.0f;
                }
                break;
            }

        case EVENT_TYPE_ENTER:
            {
                insert_line(*event_set_value);
                break;
            }

        case EVENT_TYPE_WAIT:
            {
                // Reset the local offset so the empire descriptions in character creation do not slide down.
                event_set_value->iy_local = 0;
                event_set_value->is_lock = true;
                break;
            }

        case EVENT_TYPE_NEXT:
            {
                make_next_button(event_set_value, BUTTON_TYPE_NEXT);
                event_set_value->adjust_line += 2;
                break;
            }

        case EVENT_TYPE_DONE:
            {
                make_next_button(event_set_value, BUTTON_TYPE_DONE);
                py_call_class_member_func(event_set_value->event_handler_value, "done_event", Py_BuildValue("()"));
                event_set_value->adjust_line += 2;
                break;
            }

        case EVENT_TYPE_CLEAR:
            {
                clear_line(event_set_value);
                break;
            }

        case EVENT_TYPE_QUESTION:
            {
                make_question(event_set_value, script_command.arg_list_value);
                break;
            }

        case EVENT_TYPE_LEFT_IMAGE:
            {
                py_call_class_member_func(event_set_value->event_handler_value, "on_left_image", Py_BuildValue("(s)", get_argument("src", script_command.arg_list_value)));
                break;
            }

        case EVENT_TYPE_TOP_IMAGE:
            {
                py_call_class_member_func(event_set_value->event_handler_value, "on_top_image", Py_BuildValue("(s)", get_argument("src", script_command.arg_list_value)));
                break;
            }

        case EVENT_TYPE_BACKGROUND_IMAGE:
            {
                py_call_class_member_func(event_set_value->event_handler_value, "on_background_image", Py_BuildValue("(s)", get_argument("src", script_command.arg_list_value)));
                break;
            }

        case EVENT_TYPE_IMAGE:
            {
                int x = metin2::to_number<int>(get_argument("x", script_command.arg_list_value));
                int y = metin2::to_number<int>(get_argument("y", script_command.arg_list_value));
                const char *src = get_argument("src", script_command.arg_list_value);

                py_call_class_member_func(event_set_value->event_handler_value, "on_image", Py_BuildValue("(iis)", x, y, src));
                break;
            }

        case EVENT_TYPE_INSERT_IMAGE:
            {
                const std::string &image_file = get_argument_string("image_name", script_command.arg_list_value);
                const char *title = get_argument("title", script_command.arg_list_value);
                const char *desc = get_argument("desc", script_command.arg_list_value);
                int index = metin2::to_number<int>(get_argument("index", script_command.arg_list_value));
                int total = metin2::to_number<int>(get_argument("total", script_command.arg_list_value));

                if (image_file.empty()) {
                    const char *image_type = get_argument("image_type", script_command.arg_list_value);
                    int item_index = metin2::to_number<int>(get_argument("idx", script_command.arg_list_value));
                    py_call_class_member_func(event_set_value->event_handler_value, "on_insert_item_icon", Py_BuildValue("(sissii)", image_type, item_index, title, desc, index, total));
                } else {
                    py_call_class_member_func(event_set_value->event_handler_value, "on_insert_image", Py_BuildValue("(ssssii)", image_file.c_str(), title, title, desc, index, total));
                }
                event_set_value->adjust_line += 2;
                break;
            }

        case EVENT_TYPE_ADD_MAP_SIGNAL:
            {
                float x, y;
                x = (float)metin2::to_number<double>(get_argument("x", script_command.arg_list_value));
                y = (float)metin2::to_number<double>(get_argument("y", script_command.arg_list_value));
                get_python_mini_map().add_signal_point(x, y);
                get_python_mini_map().open_atlas_window();
                break;
            }

        case EVENT_TYPE_CLEAR_MAP_SIGNAL:
            {
                get_python_mini_map().clear_all_signal_point();
                break;
            }

        case EVENT_TYPE_QUEST_BUTTON_CLOSE:
            {
                py_call_class_member_func(interface_, "clear_quest",
                    Py_BuildValue("(i)", metin2::to_number<int>(get_argument("idx", script_command.arg_list_value))));
                break;
            }

        case EVENT_TYPE_QUEST_BUTTON:
            {
                const std::string &type = get_argument_string("icon_type", script_command.arg_list_value);
                const std::string &file = get_argument_string("icon_name", script_command.arg_list_value);

                int idx = metin2::to_number<int>(get_argument("idx", script_command.arg_list_value));
                const char *name = get_argument("name", script_command.arg_list_value);

                // Workaround until the quest UI is reworked: the server sends this Korean quest name
                // ("broken dragon soul stone?...."), which is shown highlighted.
                if (!strcmp(name, "조각난 용혼석?....")) {
                    py_call_class_member_func(interface_, "recv_quest_icon", Py_BuildValue("(isss)", idx, name, "highlight", ""));
                } else {
                    if (file.empty()) {
                        py_call_class_member_func(interface_, "recv_quest", Py_BuildValue("(is)", idx, name));
                    } else {
                        py_call_class_member_func(interface_, "recv_quest_icon",
                            Py_BuildValue("(isss)", idx, name, type.c_str(), file.c_str()));
                    }
                }
                break;
            }
        case EVENT_TYPE_SET_MESSAGE_POSITION:
            {
                break;
            }
        case EVENT_TYPE_ADJUST_MESSAGE_POSITION:
            {
                break;
            }
        case EVENT_TYPE_SET_CENTER_MAP_POSITION:
            {
                get_python_mini_map().set_atlas_center_position(metin2::to_number<int>(get_argument("x", script_command.arg_list_value)), metin2::to_number<int>(get_argument("y", script_command.arg_list_value)));
                break;
            }
        case EVENT_TYPE_SLEEP:
            event_set_value->last_delay_time = metin2::to_number<int>(get_argument("value", script_command.arg_list_value));
            break;
        case EVENT_TYPE_SET_CAMERA:
            {
                abstract_application::camera_setting camera_setting;
                get_camera_setting_from_arg_list(script_command.arg_list_value, &camera_setting);
                app.set_event_camera(camera_setting);
                break;
            }
        case EVENT_TYPE_BLEND_CAMERA:
            {
                abstract_application::camera_setting camera_setting;
                get_camera_setting_from_arg_list(script_command.arg_list_value, &camera_setting);

                float blend_time = metin2::to_number<int>(get_argument("blendtime", script_command.arg_list_value));

                app.blend_event_camera(camera_setting, blend_time);
                break;
            }
        case EVENT_TYPE_RESTORE_CAMERA:
            {
                app.set_default_camera();
                break;
            }
        case EVENT_TYPE_FADE_OUT:
            {
                auto speed = (float)metin2::to_number<double>(get_argument("speed", script_command.arg_list_value));
                py_call_class_member_func(event_set_value->event_handler_value, "fade_out", Py_BuildValue("(f)", speed));
                event_set_value->is_wait_flag = true;
                break;
            }
        case EVENT_TYPE_FADE_IN:
            {
                py_call_class_member_func(event_set_value->event_handler_value, "fade_in", Py_BuildValue("(f)", metin2::to_number<double>(get_argument("speed", script_command.arg_list_value))));
                event_set_value->is_wait_flag = true;
                break;
            }
        case EVENT_TYPE_WHITE_OUT:
            {
                py_call_class_member_func(event_set_value->event_handler_value, "white_out", Py_BuildValue("(f)", metin2::to_number<double>(get_argument("speed", script_command.arg_list_value))));
                event_set_value->is_wait_flag = true;
                break;
            }
        case EVENT_TYPE_WHITE_IN:
            {
                py_call_class_member_func(event_set_value->event_handler_value, "white_in", Py_BuildValue("(f)", metin2::to_number<double>(get_argument("speed", script_command.arg_list_value))));
                event_set_value->is_wait_flag = true;
                break;
            }
        case EVENT_TYPE_CLEAR_TEXT:
            {
                clear_line(event_set_value);
                break;
            }
        case EVENT_TYPE_TEXT_HORIZONTAL_ALIGN_CENTER:
            {
                event_set_value->is_text_center_mode = true;
                if (event_set_value->current_text_line)
                    event_set_value->current_text_line->set_horizonal_align(graphic_text_instance::HORIZONTAL_ALIGN_CENTER);
                break;
            }
        case EVENT_TYPE_TITLE_IMAGE:
            {
                py_call_class_member_func(event_set_value->event_handler_value, "on_title_image", Py_BuildValue("(s)", get_argument("src", script_command.arg_list_value)));
                break;
            }
        case EVENT_TYPE_DUNGEON_RESULT:
            {
                int killstone_count = metin2::to_number<int>(get_argument("killstone_count", script_command.arg_list_value));
                int killmob_count = metin2::to_number<int>(get_argument("killmob_count", script_command.arg_list_value));
                int find_hidden = metin2::to_number<int>(get_argument("find_hidden", script_command.arg_list_value));
                int hidden_total = metin2::to_number<int>(get_argument("hidden_total", script_command.arg_list_value));
                int use_potion = metin2::to_number<int>(get_argument("use_potion", script_command.arg_list_value));
                int is_revived = metin2::to_number<int>(get_argument("is_revived", script_command.arg_list_value));
                int killallmob = metin2::to_number<int>(get_argument("killallmob", script_command.arg_list_value));
                int total_time = metin2::to_number<int>(get_argument("total_time", script_command.arg_list_value));
                int bonus_exp = metin2::to_number<int>(get_argument("bonus_exp", script_command.arg_list_value));

                py_call_class_member_func(interface_, "show_dungeon_result",
                    Py_BuildValue("(iiiiiiiii)",
                        killstone_count,
                        killmob_count,
                        find_hidden,
                        hidden_total,
                        use_potion,
                        is_revived,
                        killallmob,
                        total_time,
                        bonus_exp));
                break;
            }
        case EVENT_TYPE_ITEM_NAME:
            {
                int i_index = metin2::to_number<int>(get_argument("value", script_command.arg_list_value));
                item_data *item_data;
                if (get_item_manager().item_data_pointer(i_index, &item_data)) {
                    event_set_value->current_line.append(item_data->get_name());
                    event_set_value->current_text_line->set_value(event_set_value->current_line.c_str());
                    event_set_value->current_text_line->set_color(1.0f, 0.2f, 0.2f);
                    event_set_value->current_letter += utf8::columns(item_data->get_name());

                    if (event_set_value->current_letter >= event_set_value->restricted_character_count)
                        insert_line(*event_set_value);

                    event_set_value->last_delay_time = event_set_value->waiting_time;
                }

                break;
            }
        case EVENT_TYPE_MONSTER_NAME:
            {
                int index_2 = metin2::to_number<int>(get_argument("value", script_command.arg_list_value));
                const char *c_sz_name;

                python_non_player &non_player = get_python_non_player();
                if (non_player.get_name(index_2, &c_sz_name)) {
                    event_set_value->current_line.append(c_sz_name);
                    event_set_value->current_text_line->set_value(event_set_value->current_line.c_str());
                    event_set_value->current_letter += utf8::columns(c_sz_name);

                    if (event_set_value->current_letter >= event_set_value->restricted_character_count)
                        insert_line(*event_set_value);

                    event_set_value->last_delay_time = event_set_value->waiting_time;
                }

                break;
            }
        case EVENT_TYPE_WINDOW_SIZE:
            {
                int width_value = metin2::to_number<int>(get_argument("width", script_command.arg_list_value));
                int height = metin2::to_number<int>(get_argument("height", script_command.arg_list_value));
                py_call_class_member_func(event_set_value->event_handler_value, "on_size", Py_BuildValue("(ii)", width_value, height));
                break;
            }
        case EVENT_TYPE_INPUT:
            {
                insert_line(*event_set_value);
                py_call_class_member_func(event_set_value->event_handler_value, "on_input", Py_BuildValue("()"));
                break;
            }
        case EVENT_TYPE_CONFIRM_WAIT:
            {
                int time_out = metin2::to_number<int>(get_argument("timeout", script_command.arg_list_value));
                event_set_value->is_confirm_wait = true;
                event_set_value->confirm_time_text_line = event_set_value->current_text_line;
                event_set_value->confirm_end_time = el_timer_get_m_sec() / 1000 + time_out;
                insert_line(*event_set_value, true);
                make_next_button(event_set_value, BUTTON_TYPE_CANCEL);
                break;
            }
        case EVENT_TYPE_END_CONFIRM_WAIT:
            {
                for (uint32_t i = 0; i < event_set_vector_value_.size(); ++i) {
                    if (nullptr == event_set_vector_value_[i])
                        continue;

                    event_set *set = event_set_vector_value_[i];
                    if (!set->is_confirm_wait)
                        continue;

                    set->is_confirm_wait = false;
                    set->confirm_time_text_line = nullptr;
                    set->confirm_end_time = 0;

                    py_call_class_member_func(set->event_handler_value, "close_self", Py_BuildValue("()"));
                }
                break;
            }
        case EVENT_TYPE_SELECT_ITEM:
            {
                py_call_class_member_func(interface_, "open_select_item_window", Py_BuildValue("()"));
                break;
            }
        }
    }

    void python_event_manager::render_event_set(int index) {
        if (!check_event_set_index(index))
            return;

        event_set *event_set_value = event_set_vector_value_[index];
        if (!event_set_value)
            return;

        int count = 0;

        for (auto itor = event_set_value->script_text_line_list_value.begin(); itor != event_set_value->script_text_line_list_value.end(); ++itor, ++count) {
            if (count < event_set_value->visible_start_line)
                continue;

            if (count >= event_set_value->visible_start_line + event_set_value->visible_line_count)
                continue;

            text_line &line = *itor;
            graphic_text_instance *instance = line.instance;

            instance->update();
            instance->render();
        }

        if (count >= event_set_value->visible_start_line)
            if (count < event_set_value->visible_start_line + event_set_value->visible_line_count) {
                event_set_value->current_text_line->update();
                event_set_value->current_text_line->render();
            }
    }

    void python_event_manager::skip(int index) {
        if (!check_event_set_index(index))
            return;

        event_set *event_set_value = event_set_vector_value_[index];

        if (true == event_set_value->is_lock) {
            event_set_value->last_delay_time = 0;
            event_set_value->is_lock = false;
        } else {
            event_set_value->last_delay_time = -1000;
        }
    }

    auto python_event_manager::is_wait(int index) -> bool {
        if (!check_event_set_index(index))
            return false;

        event_set *event_set_value = event_set_vector_value_[index];
        if (!event_set_value) {
            return false;
        }

        return event_set_value->is_lock;
    }

    void python_event_manager::end_event_process(int index) {
        if (!check_event_set_index(index))
            return;

        event_set *event_set_value = event_set_vector_value_[index];
        if (!event_set_value) {
            return;
        }

        event_set_value->is_wait_flag = false;
    }

    void python_event_manager::make_next_button(event_set *event_set_value, int button_type_value) {
        add_space(*event_set_value, c_fLine_Temp + 5);
        py_call_class_member_func(event_set_value->event_handler_value, "make_next_button", Py_BuildValue("(i)", button_type_value));
    }

    void python_event_manager::make_question(event_set *event_set_value, event_script::arg_list &argument_list) {
        if (argument_list.empty())
            return;

        py_call_class_member_func(event_set_value->event_handler_value, "make_question", Py_BuildValue("(i)", argument_list.size()));
        event_set_value->answer = argument_list.size();

        int index = 0;
        for (auto itor = argument_list.begin(); itor != argument_list.end(); ++itor) {
            event_script::arg &argument = *itor;
            py_call_class_member_func(event_set_value->event_handler_value, "append_question", Py_BuildValue("(si)", argument.value.c_str(), index));
            ++index;
        }
    }

    void python_event_manager::select_answer(int index, int answer) {
        if (!check_event_set_index(index))
            return;

        get_python_network_stream().send_script_answer_packet(answer);
    }

    void python_event_manager::set_visible_start_line(int index, int start_line) {
        if (!check_event_set_index(index))
            return;

        event_set *event_set_value = event_set_vector_value_[index];

        if (uint32_t(start_line) > event_set_value->script_text_line_list_value.size())
            return;

        event_set_value->visible_start_line = start_line;
    }

    void python_event_manager::set_visible_line_count(int index, int line_count) {
        if (!check_event_set_index(index))
            return;

        event_set *event_set_value = event_set_vector_value_[index];

        event_set_value->visible_line_count = line_count;
    }

    auto python_event_manager::visible_start_line(int index) -> int {
        if (!check_event_set_index(index))
            return 0;

        event_set *event_set_value = event_set_vector_value_[index];
        return event_set_value->visible_start_line;
    }

    auto python_event_manager::line_count(int index) -> int {
        if (!check_event_set_index(index))
            return 0;

        event_set *event_set_value = event_set_vector_value_[index];
        return event_set_value->script_text_line_list_value.size() + event_set_value->adjust_line;
    }

    void python_event_manager::clear_line(event_set *event_set_value) {
        if (!event_set_value)
            return;

        for (auto itor = event_set_value->script_text_line_list_value.begin(); itor != event_set_value->script_text_line_list_value.end(); ++itor) {
            text_line &line = *itor;
            graphic_text_instance *instance = line.instance;
            instance->destroy();
            instance->update();
        }

        event_set_value->current_text_line->destroy();
        event_set_value->current_text_line->update();

        event_set_value->current_text_line = nullptr;
        event_set_value->script_text_line_list_value.clear();

        insert_line(*event_set_value);
    }

    void python_event_manager::insert_line(event_set &event_set_value, bool is_center, int x_pos) {
        if (event_set_value.current_text_line) {
            text_line line;
            if (graphic_text_instance::HORIZONTAL_ALIGN_CENTER == event_set_value.current_text_line->get_horizontal_align()) {
                line.ix_local = event_set_value.width / 2;
                line.iy_local = event_set_value.iy_local;

            } else {
                int text_width;
                int text_height;
                event_set_value.current_text_line->text_size(&text_width, &text_height);
                line.ix_local = 0;
                if (x_pos != 0) {
                    line.ix_local += (x_pos - 20);
                    line.ix_local -= text_width / 2;
                }

                line.iy_local = event_set_value.iy_local;
            }
            line.instance = event_set_value.current_text_line;
            event_set_value.script_text_line_list_value.push_back(line);
            add_space(event_set_value, c_fLine_Temp);
        }

        auto *default_font_value = static_cast<graphic_text *>(default_font_get_resource());
        if (!default_font_value) {
            metin2::log::error("Cannot find the default font");
            return;
        }

        event_set_value.current_text_line = script_text_line_pool_.alloc();
        if (!event_set_value.current_text_line) {
            metin2::log::error("Cannot allocate a new event text line");
            return;
        }

        event_set_value.current_text_line->set_text_pointer(default_font_value);

        event_set_value.current_text_line->set_color(1.0f, 1.0f, 1.0f);
        event_set_value.current_text_line->set_value("");

        if (event_set_value.is_text_center_mode || is_center) {
            event_set_value.current_text_line->set_horizonal_align(graphic_text_instance::HORIZONTAL_ALIGN_CENTER);
            event_set_value.current_text_line->set_position(event_set_value.ix + event_set_value.width / 2, event_set_value.iy + event_set_value.iy_local);
        } else {
            event_set_value.current_text_line->set_horizonal_align(graphic_text_instance::HORIZONTAL_ALIGN_LEFT);
            event_set_value.current_text_line->set_position(event_set_value.ix, event_set_value.iy + event_set_value.iy_local);
        }

        event_set_value.current_letter = 0;
        event_set_value.current_line = "";
    }

    void python_event_manager::refresh_line_position(event_set *event_set_value) {
        for (auto itor = event_set_value->script_text_line_list_value.begin(); itor != event_set_value->script_text_line_list_value.end(); ++itor) {
            text_line &line = *itor;
            graphic_text_instance *instance = line.instance;
            instance->set_position(event_set_value->ix + line.ix_local, event_set_value->iy + line.iy_local);
        }

        int ix_text_pos;
        if (graphic_text_instance::HORIZONTAL_ALIGN_CENTER == event_set_value->current_text_line->get_horizontal_align()) {
            ix_text_pos = event_set_value->ix + event_set_value->width / 2;
        } else {
            ix_text_pos = event_set_value->ix;
        }
        event_set_value->current_text_line->set_position(ix_text_pos, event_set_value->iy + event_set_value->iy_local);
    }

    void python_event_manager::add_space(event_set &event_set_value, int space) {
        event_set_value.iy_local += space;
    }

    auto python_event_manager::script_event_index(const char *name, int *event_position, int *event_type_value) -> bool {
        const char *event_name;

        if ('/' == name[0]) {
            *event_position = EVENT_POSITION_END;
            event_name = &name[1];
        } else {
            *event_position = EVENT_POSITION_START;
            event_name = &name[0];
        }

        auto it = event_type_map_.find(event_name);
        if (it == event_type_map_.end()) {
            metin2::log::debug("Unknown event script command {}", event_name);
            return false;
        }

        *event_type_value = it->second;

        return true;
    }

    auto python_event_manager::check_event_set_index(int index) -> bool {
        if (index < 0)
            return false;

        if ((uint32_t)index >= event_set_vector_value_.size())
            return false;

        return true;
    }

    void python_event_manager::destroy() {
        event_set_vector_value_.clear();
        event_set_pool_.clear();
        script_text_line_pool_.clear();
    }

    void python_event_manager::set_interface_window(PyObject *interface_value) {
        interface_ = interface_value;
    }

    void python_event_manager::set_left_time_string(const char *string) {
        left_time_string_ = string;
    }

    python_event_manager::python_event_manager()
        : interface_(nullptr), left_time_string_("Time left: %d s") {
        event_type_map_["LETTER"] = EVENT_TYPE_LETTER;
        event_type_map_["COLOR"] = EVENT_TYPE_COLOR;
        event_type_map_["DELAY"] = EVENT_TYPE_DELAY;
        event_type_map_["ENTER"] = EVENT_TYPE_ENTER;
        event_type_map_["WAIT"] = EVENT_TYPE_WAIT;
        event_type_map_["CLEAR"] = EVENT_TYPE_CLEAR;
        event_type_map_["QUESTION"] = EVENT_TYPE_QUESTION;
        event_type_map_["NEXT"] = EVENT_TYPE_NEXT;
        event_type_map_["DONE"] = EVENT_TYPE_DONE;

        event_type_map_["LEFTIMAGE"] = EVENT_TYPE_LEFT_IMAGE;
        event_type_map_["TOPIMAGE"] = EVENT_TYPE_TOP_IMAGE;
        event_type_map_["BGIMAGE"] = EVENT_TYPE_BACKGROUND_IMAGE;
        event_type_map_["IMAGE"] = EVENT_TYPE_IMAGE;

        event_type_map_["ADDMAPSIGNAL"] = EVENT_TYPE_ADD_MAP_SIGNAL;
        event_type_map_["CLEARMAPSIGNAL"] = EVENT_TYPE_CLEAR_MAP_SIGNAL;

        event_type_map_["SETMSGPOS"] = EVENT_TYPE_SET_MESSAGE_POSITION;
        event_type_map_["ADJMSGPOS"] = EVENT_TYPE_ADJUST_MESSAGE_POSITION;
        event_type_map_["SETCMAPPOS"] = EVENT_TYPE_SET_CENTER_MAP_POSITION;

        event_type_map_["QUESTBUTTON"] = EVENT_TYPE_QUEST_BUTTON;

        event_type_map_["QUESTBUTTON_CLOSE"] = EVENT_TYPE_QUEST_BUTTON_CLOSE;

        event_type_map_["SLEEP"] = EVENT_TYPE_SLEEP;
        event_type_map_["SET_CAMERA"] = EVENT_TYPE_SET_CAMERA;
        event_type_map_["BLEND_CAMERA"] = EVENT_TYPE_BLEND_CAMERA;
        event_type_map_["RESTORE_CAMERA"] = EVENT_TYPE_RESTORE_CAMERA;
        event_type_map_["FADE_OUT"] = EVENT_TYPE_FADE_OUT;
        event_type_map_["FADE_IN"] = EVENT_TYPE_FADE_IN;
        event_type_map_["WHITE_OUT"] = EVENT_TYPE_WHITE_OUT;
        event_type_map_["WHITE_IN"] = EVENT_TYPE_WHITE_IN;
        event_type_map_["CLEAR_TEXT"] = EVENT_TYPE_CLEAR_TEXT;
        event_type_map_["TEXT_HORIZONTAL_ALIGN_CENTER"] = EVENT_TYPE_TEXT_HORIZONTAL_ALIGN_CENTER;
        event_type_map_["TITLE_IMAGE"] = EVENT_TYPE_TITLE_IMAGE;

        event_type_map_["RUN_CINEMA"] = EVENT_TYPE_RUN_CINEMA;
        event_type_map_["DUNGEON_RESULT"] = EVENT_TYPE_DUNGEON_RESULT;

        event_type_map_["ITEM"] = EVENT_TYPE_ITEM_NAME;
        event_type_map_["MOB"] = EVENT_TYPE_MONSTER_NAME;

        event_type_map_["COLOR256"] = EVENT_TYPE_COLOR256;
        event_type_map_["WINDOW_SIZE"] = EVENT_TYPE_WINDOW_SIZE;

        event_type_map_["INPUT"] = EVENT_TYPE_INPUT;
        event_type_map_["CONFIRM_WAIT"] = EVENT_TYPE_CONFIRM_WAIT;
        event_type_map_["END_CONFIRM_WAIT"] = EVENT_TYPE_END_CONFIRM_WAIT;

        event_type_map_["INSERT_IMAGE"] = EVENT_TYPE_INSERT_IMAGE;

        event_type_map_["SELECT_ITEM"] = EVENT_TYPE_SELECT_ITEM;
    }

    python_event_manager::~python_event_manager() = default;
}
