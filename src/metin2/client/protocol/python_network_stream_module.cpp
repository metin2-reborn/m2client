

#include <metin2/client/app/abstract_player.h>
#include <metin2/client/app/test.h>
#include <metin2/client/guilds/guild_mark_downloader.h>
#include <metin2/client/guilds/guild_mark_uploader.h>
#include <metin2/client/guilds/python_guild.h>
#include <metin2/log/log.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include "account_connector.h"
#include "python_network_stream.h"

namespace metin2::inline client::inline protocol {
    static std::string server_info;
    std::list<std::string> list_command;

    auto net_set_server_info(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        server_info = file_name;
        Py_RETURN_NONE;
    }

    auto net_get_server_info(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("s", server_info.c_str());
    }

    auto net_preserve_server_command(PyObject *self, PyObject *args) -> PyObject * {
        char *line;
        if (!PyArg_ParseTuple(args, "s", &line)) {
            return nullptr;
        }

        list_command.push_back(line);

        Py_RETURN_NONE;
    }

    auto net_get_preserved_server_command(PyObject *self, PyObject *args) -> PyObject * {
        if (list_command.empty())
            return Py_BuildValue("s", "");

        std::string command = list_command.front();
        list_command.pop_front();

        return Py_BuildValue("s", command.c_str());
    }

    auto net_start_game(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.start_game();

        Py_RETURN_NONE;
    }

    auto net_is_test(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", is_test_server_mode);
    }

    auto net_warp(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!PyArg_ParseTuple(args, "ii", &x, &y)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.warp(x, y);

        Py_RETURN_NONE;
    }

    auto net_upload_mark(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.upload_mark(file_name));
    }

    auto net_upload_symbol(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.upload_symbol(file_name));
    }

    auto net_get_guild_id(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.guild_id());
    }

    auto net_get_empire_id(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.empire_id());
    }

    auto net_get_main_actor_race(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.main_actor_race());
    }

    auto net_get_main_actor_empire(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.main_actor_empire());
    }

    auto net_get_main_actor_skill_group(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.main_actor_skill_group());
    }

    auto net_is_selected_empire(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.is_selected_empire());
    }

    auto net_get_account_character_slot_data_integer(PyObject *self, PyObject *args) -> PyObject * {
        int index, type;
        if (!PyArg_ParseTuple(args, "ii", &index, &type)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        uint32_t result = net_stream.account_character_slot_datau(index, type);
        return Py_BuildValue("i", result);
    }

    auto net_get_account_character_slot_data_string(PyObject *self, PyObject *args) -> PyObject * {
        int index, type;
        if (!PyArg_ParseTuple(args, "ii", &index, &type)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("s", net_stream.account_character_slot_dataz(index, type));
    }

    auto net_get_field_music_file_name(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("s", net_stream.field_music_file_name());
    }

    auto net_get_field_music_volume(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("f", net_stream.field_music_volume());
    }

    auto net_set_phase_window(PyObject *self, PyObject *args) -> PyObject * {
        int e_phase_wnd;
        PyObject *po_phase_wnd;

        if (!PyArg_ParseTuple(args, "iO", &e_phase_wnd, &po_phase_wnd)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_phase_window(e_phase_wnd, po_phase_wnd);
        Py_RETURN_NONE;
    }

    auto net_clear_phase_window(PyObject *self, PyObject *args) -> PyObject * {
        int e_phase_wnd;
        PyObject *po_phase_wnd;

        if (!PyArg_ParseTuple(args, "iO", &e_phase_wnd, &po_phase_wnd)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.clear_phase_window(e_phase_wnd, po_phase_wnd);
        Py_RETURN_NONE;
    }

    auto net_set_server_command_parser_window(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *phase_wnd;
        if (!PyArg_ParseTuple(args, "O", &phase_wnd)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_server_command_parser_window(phase_wnd);
        Py_RETURN_NONE;
    }

    auto net_set_account_connector_handler(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *phase_wnd;
        if (!PyArg_ParseTuple(args, "O", &phase_wnd)) {
            return nullptr;
        }

        account_connector &account_connector_value = get_account_connector();
        account_connector_value.set_handler(phase_wnd);
        Py_RETURN_NONE;
    }

    auto net_set_handler(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handler;
        if (!PyArg_ParseTuple(args, "O", &handler)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_handler(handler);
        Py_RETURN_NONE;
    }

    auto net_set_tcp_recv_buffer_size(PyObject *self, PyObject *args) -> PyObject * {
        int buf_size;
        if (!PyArg_ParseTuple(args, "i", &buf_size)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_recv_buffer_size(buf_size);
        Py_RETURN_NONE;
    }

    auto net_set_tcp_send_buffer_size(PyObject *self, PyObject *args) -> PyObject * {
        int buf_size;
        if (!PyArg_ParseTuple(args, "i", &buf_size)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_send_buffer_size(buf_size);
        Py_RETURN_NONE;
    }

    auto net_set_udp_recv_buffer_size(PyObject *self, PyObject *args) -> PyObject * {
        int buf_size;
        if (!PyArg_ParseTuple(args, "i", &buf_size)) {
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    auto net_set_mark_server(PyObject *self, PyObject *args) -> PyObject * {
        char *addr;
        int port;

        if (!PyArg_ParseTuple(args, "si", &addr, &port)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_mark_server(addr, port);
        Py_RETURN_NONE;
    }

    auto net_connect_tcp(PyObject *self, PyObject *args) -> PyObject * {
        char *addr;
        int port;

        if (!PyArg_ParseTuple(args, "si", &addr, &port)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.connect_login_server(addr, port);
        Py_RETURN_NONE;
    }

    auto net_connect_udp(PyObject *self, PyObject *args) -> PyObject * {
        char *ip;
        int port;

        if (!PyArg_ParseTuple(args, "si", &ip, &port)) {
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    auto net_connect_to_account_server(PyObject *self, PyObject *args) -> PyObject * {
        char *addr;
        int port;
        char *account_addr;
        int account_port;

        if (!PyArg_ParseTuple(args, "sisi", &addr, &port, &account_addr, &account_port)) {
            return nullptr;
        }

        account_connector &account_connector_value = get_account_connector();
        account_connector_value.connect(addr, port, account_addr, account_port);
        Py_RETURN_NONE;
    }

    auto net_set_login_info(PyObject *self, PyObject *args) -> PyObject * {
        char *name, *pwd;
        if (!PyArg_ParseTuple(args, "ss", &name, &pwd)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        account_connector &account_connector_value = get_account_connector();
        net_stream.set_login_info(name, pwd);
        account_connector_value.set_login_info(name, pwd);
        Py_RETURN_NONE;
    }

    auto net_send_select_empire_packet(PyObject *self, PyObject *args) -> PyObject * {
        int empire_index;
        if (!PyArg_ParseTuple(args, "i", &empire_index)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_select_empire_packet(empire_index);
        Py_RETURN_NONE;
    }

    auto net_direct_enter(PyObject *self, PyObject *args) -> PyObject * {
        int chr_slot;
        if (!PyArg_ParseTuple(args, "i", &chr_slot)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.connect_game_server(chr_slot);
        Py_RETURN_NONE;
    }

    auto net_send_select_character_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_select_character_packet((uint8_t)index);
        Py_RETURN_NONE;
    }

    auto net_send_change_name_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        char *name;

        if (!PyArg_ParseTuple(args, "is", &index, &name)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_change_name_packet((uint8_t)index, name);
        Py_RETURN_NONE;
    }

    auto net_enable_chat_insult_filter(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!PyArg_ParseTuple(args, "i", &enable)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.enable_chat_insult_filter(enable ? true : false);
        Py_RETURN_NONE;
    }

    auto net_is_chat_insult_in(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!PyArg_ParseTuple(args, "s", &msg)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.is_chat_insult_in(msg));
    }

    auto net_is_insult_in(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!PyArg_ParseTuple(args, "s", &msg)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.is_insult_in(msg));
    }

    auto net_send_whisper_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *name, *line;
        if (!PyArg_ParseTuple(args, "ss", &name, &line)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_whisper_packet(name, line);
        Py_RETURN_NONE;
    }

    auto net_send_mobile_message_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *name, *text;
        if (!PyArg_ParseTuple(args, "ss", &name, &text)) {
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    auto net_send_chat_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *line;
        int type = CHAT_TYPE_TALKING;

        if (!PyArg_ParseTuple(args, "s|i", &line, &type)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_chat_packet(line, type);
        Py_RETURN_NONE;
    }

    auto net_send_emoticon(PyObject *self, PyObject *args) -> PyObject * {
        int emoticon;
        if (!PyArg_ParseTuple(args, "i", &emoticon)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_emoticon(emoticon);
        Py_RETURN_NONE;
    }

    auto net_send_create_character_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        char *name;
        int job, shape, stat1, stat2, stat3, stat4;

        if (!PyArg_ParseTuple(args, "isiiiiii", &index, &name, &job, &shape, &stat1, &stat2, &stat3, &stat4)) {
            return nullptr;
        }

        if (index < 0 || index > 3) {
            PyErr_Format(PyExc_ValueError, "Character slot %d out of range", index);
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_create_character_packet((uint8_t)index, name, (uint8_t)job, (uint8_t)shape, stat1, stat2, stat3, stat4);
        Py_RETURN_NONE;
    }

    auto net_send_destroy_character_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        char *private_code;

        if (!PyArg_ParseTuple(args, "is", &index, &private_code)) {
            return nullptr;
        }

        if (index < 0 || index > 3) {
            PyErr_Format(PyExc_ValueError, "Character slot %d out of range", index);
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_destroy_character_packet((uint8_t)index, private_code);
        Py_RETURN_NONE;
    }

    auto net_send_enter_game_packet(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_enter_game();
        Py_RETURN_NONE;
    }

    auto net_on_click_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_on_click_packet(index);

        Py_RETURN_NONE;
    }

    auto net_send_item_use_packet(PyObject *self, PyObject *args) -> PyObject * {
        item_pos cell;
        switch (PyTuple_Size(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "H", &cell.cell)) {
                return nullptr;
            }
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "bH", &cell.window_type, &cell.cell)) {
                return nullptr;
            }
            break;
        default:
            PyErr_Format(PyExc_TypeError, "wrong number of arguments (%zd given)", PyTuple_Size(args));
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_item_use_packet(cell);
        Py_RETURN_NONE;
    }

    auto net_send_item_use_to_item_packet(PyObject *self, PyObject *args) -> PyObject * {
        item_pos source_cell;
        item_pos target_cell;
        switch (PyTuple_Size(args)) {
        case 2:

            if (!PyArg_ParseTuple(args, "HH", &source_cell.cell, &target_cell.cell)) {
                return nullptr;
            }
            break;
        case 4:

            if (!PyArg_ParseTuple(args, "bHbH", &source_cell.window_type, &source_cell.cell, &target_cell.window_type, &target_cell.cell)) {
                return nullptr;
            }
            break;
        default:
            PyErr_Format(PyExc_TypeError, "wrong number of arguments (%zd given)", PyTuple_Size(args));
            return nullptr;
        }
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_item_use_to_item_packet(source_cell, target_cell);
        Py_RETURN_NONE;
    }

    auto net_send_item_drop_packet_new(PyObject *self, PyObject *args) -> PyObject * {
        item_pos cell;
        int count;
        switch (PyTuple_Size(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "Hi", &cell.cell, &count)) {
                return nullptr;
            }

            break;
        case 3:
            if (!PyArg_ParseTuple(args, "bHi", &cell.window_type, &cell.cell, &count)) {
                return nullptr;
            }

            break;
        default:
            PyErr_Format(PyExc_TypeError, "wrong number of arguments (%zd given)", PyTuple_Size(args));
            return nullptr;
        }
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_item_drop_packet_new(cell, 0, count);
        Py_RETURN_NONE;
    }

    auto net_send_gold_drop_packet_new(PyObject *self, PyObject *args) -> PyObject * {
        int elk;
        if (!PyArg_ParseTuple(args, "i", &elk)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_item_drop_packet_new(item_pos(RESERVED_WINDOW, 0), (uint32_t)elk, 0);
        Py_RETURN_NONE;
    }

    auto net_send_item_move_packet(PyObject *self, PyObject *args) -> PyObject * {
        item_pos cell;
        item_pos change_cell;
        int num;

        switch (PyTuple_Size(args)) {
        case 3:
            if (!PyArg_ParseTuple(args, "HHi", &cell.cell, &change_cell.cell, &num)) {
                return nullptr;
            }
            break;
        case 5:
            {
                if (!PyArg_ParseTuple(args, "bHbHi", &cell.window_type, &cell.cell, &change_cell.window_type, &change_cell.cell, &num)) {
                    return nullptr;
                }
            }
            break;
        default:
            PyErr_Format(PyExc_TypeError, "wrong number of arguments (%zd given)", PyTuple_Size(args));
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_item_move_packet(cell, change_cell, (uint8_t)num);
        Py_RETURN_NONE;
    }

    auto net_send_give_item_packet(PyObject *self, PyObject *args) -> PyObject * {
        int target_vid;
        item_pos cell;
        int item_count;
        switch (PyTuple_Size(args)) {
        case 3:
            if (!PyArg_ParseTuple(args, "iHi", &target_vid, &cell.cell, &item_count)) {
                return nullptr;
            }
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "ibHi", &target_vid, &cell.window_type, &cell.cell, &item_count)) {
                return nullptr;
            }
            break;
        default:
            PyErr_Format(PyExc_TypeError, "wrong number of arguments (%zd given)", PyTuple_Size(args));
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_give_item_packet(target_vid, cell, item_count);
        Py_RETURN_NONE;
    }

    auto net_send_shop_end_packet(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_shop_end_packet();
        Py_RETURN_NONE;
    }

    auto net_send_shop_buy_packet(PyObject *self, PyObject *args) -> PyObject * {
        int count;
        if (!PyArg_ParseTuple(args, "i", &count)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_shop_buy_packet(count);
        Py_RETURN_NONE;
    }

    // The inventory type the scripts pass is not part of SHOP_SUBHEADER_CG_SELL2; the server sells
    // from the main inventory.
    auto net_send_shop_sell_packet_new(PyObject *self, PyObject *args) -> PyObject * {
        int slot_number, count, inventory_type = 0;
        if (!PyArg_ParseTuple(args, "ii|i", &slot_number, &count, &inventory_type)) {
            return nullptr;
        }
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_shop_sell_packet_new(slot_number, count);
        Py_RETURN_NONE;
    }

    auto net_send_exchange_start_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!PyArg_ParseTuple(args, "i", &vid)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_exchange_start_packet(vid);
        Py_RETURN_NONE;
    }

    auto net_send_exchange_elk_add_packet(PyObject *self, PyObject *args) -> PyObject * {
        int elk;
        if (!PyArg_ParseTuple(args, "i", &elk)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_exchange_elk_add_packet(elk);
        Py_RETURN_NONE;
    }

    auto net_send_exchange_item_add_packet(PyObject *self, PyObject *args) -> PyObject * {
        item_pos cell;
        int display_slot_index;

        switch (PyTuple_Size(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "Hi", &cell.cell, &display_slot_index)) {
                return nullptr;
            }
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "bHi", &cell.window_type, &cell.cell, &display_slot_index)) {
                return nullptr;
            }
            break;
        default:
            PyErr_Format(PyExc_TypeError, "wrong number of arguments (%zd given)", PyTuple_Size(args));
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_exchange_item_add_packet(cell, display_slot_index);
        Py_RETURN_NONE;
    }

    auto net_send_exchange_accept_packet(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_exchange_accept_packet();
        Py_RETURN_NONE;
    }

    auto net_send_exchange_exit_packet(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_exchange_exit_packet();
        Py_RETURN_NONE;
    }

    auto net_exit_application(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.exit_application();
        Py_RETURN_NONE;
    }

    auto net_exit_game(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.exit_game();
        Py_RETURN_NONE;
    }

    auto net_log_out_game(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.log_out_game();
        Py_RETURN_NONE;
    }

    auto net_disconnect(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_off_line_phase();
        net_stream.disconnect();

        Py_RETURN_NONE;
    }

    auto net_toggle_game_debug_info(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.toggle_game_debug_info();
        Py_RETURN_NONE;
    }

    auto net_register_emoticon_string(PyObject *self, PyObject *args) -> PyObject * {
        char *emoticon_string;
        if (!PyArg_ParseTuple(args, "s", &emoticon_string)) {
            return nullptr;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.register_emoticon_string(emoticon_string);
        Py_RETURN_NONE;
    }

    auto net_send_messenger_add_by_vid_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!PyArg_ParseTuple(args, "i", &vid)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_messenger_add_by_vid_packet(vid);

        Py_RETURN_NONE;
    }

    auto net_send_messenger_add_by_name_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!PyArg_ParseTuple(args, "s", &name)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_messenger_add_by_name_packet(name);

        Py_RETURN_NONE;
    }

    auto net_send_messenger_remove_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *key, *name;
        if (!PyArg_ParseTuple(args, "ss", &key, &name)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_messenger_remove_packet(key, name);

        Py_RETURN_NONE;
    }

    auto net_send_party_invite_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!PyArg_ParseTuple(args, "i", &vid)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_invite_packet(vid);

        Py_RETURN_NONE;
    }

    auto net_send_party_invite_answer_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid, answer;
        if (!PyArg_ParseTuple(args, "ii", &vid, &answer)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_invite_answer_packet(vid, answer);

        Py_RETURN_NONE;
    }

    auto net_send_party_exit_packet(PyObject *self, PyObject *args) -> PyObject * {
        abstract_player &player = get_abstract_player();

        python_network_stream &rns = get_python_network_stream();

        uint32_t vid = player.main_character_index();
        uint32_t pid;
        if (player.party_member_vid_to_pid(vid, &pid))
            rns.send_party_remove_packet(pid);

        Py_RETURN_NONE;
    }

    auto net_send_party_remove_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!PyArg_ParseTuple(args, "i", &vid)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_remove_packet(vid);

        Py_RETURN_NONE;
    }

    auto net_send_party_set_state_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid, state, flag;
        if (!PyArg_ParseTuple(args, "iii", &vid, &state, &flag)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_set_state_packet(vid, state, flag);

        Py_RETURN_NONE;
    }

    auto net_send_party_use_skill_packet(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index, vid;
        if (!PyArg_ParseTuple(args, "ii", &skill_index, &vid)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_use_skill_packet(skill_index, vid);

        Py_RETURN_NONE;
    }

    auto net_send_party_parameter_packet(PyObject *self, PyObject *args) -> PyObject * {
        int mode;
        if (!PyArg_ParseTuple(args, "i", &mode)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_parameter_packet(mode);

        Py_RETURN_NONE;
    }

    auto net_send_safebox_save_money_packet(PyObject *self, PyObject *args) -> PyObject * {
        int money;
        if (!PyArg_ParseTuple(args, "i", &money)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_safe_box_money_packet(SAFEBOX_MONEY_STATE_SAVE, money);

        Py_RETURN_NONE;
    }

    auto net_send_safebox_withdraw_money_packet(PyObject *self, PyObject *args) -> PyObject * {
        int money;
        if (!PyArg_ParseTuple(args, "i", &money)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_safe_box_money_packet(SAFEBOX_MONEY_STATE_WITHDRAW, money);

        Py_RETURN_NONE;
    }

    auto net_send_safebox_checkin_packet(PyObject *self, PyObject *args) -> PyObject * {
        item_pos inventory_pos;
        int safe_box_pos;

        switch (PyTuple_Size(args)) {
        case 2:
            inventory_pos.window_type = INVENTORY;
            if (!PyArg_ParseTuple(args, "Hi", &inventory_pos.cell, &safe_box_pos)) {
                return nullptr;
            }
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "bHi", &inventory_pos.window_type, &inventory_pos.cell, &safe_box_pos)) {
                return nullptr;
            }
            break;
        default:
            PyErr_Format(PyExc_TypeError, "wrong number of arguments (%zd given)", PyTuple_Size(args));
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_safe_box_checkin_packet(inventory_pos, safe_box_pos);

        Py_RETURN_NONE;
    }

    auto net_send_safebox_checkout_packet(PyObject *self, PyObject *args) -> PyObject * {
        int safe_box_pos;
        item_pos inventory_pos;

        switch (PyTuple_Size(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "iH", &safe_box_pos, &inventory_pos.cell)) {
                return nullptr;
            }
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "ibH", &safe_box_pos, &inventory_pos.window_type, &inventory_pos.cell)) {
                return nullptr;
            }
            break;
        default:
            PyErr_Format(PyExc_TypeError, "wrong number of arguments (%zd given)", PyTuple_Size(args));
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_safe_box_checkout_packet(safe_box_pos, inventory_pos);

        Py_RETURN_NONE;
    }

    auto net_send_safebox_item_move_packet(PyObject *self, PyObject *args) -> PyObject * {
        int source_pos, target_pos, count;
        if (!PyArg_ParseTuple(args, "iii", &source_pos, &target_pos, &count)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_safe_box_item_move_packet(source_pos, target_pos, count);

        Py_RETURN_NONE;
    }

    auto net_send_mall_checkout_packet(PyObject *self, PyObject *args) -> PyObject * {
        int mall_pos;
        item_pos inventory_pos;

        switch (PyTuple_Size(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "iH", &mall_pos, &inventory_pos.cell)) {
                return nullptr;
            }
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "ibH", &mall_pos, &inventory_pos.window_type, &inventory_pos.cell)) {
                return nullptr;
            }
            break;
        default:
            PyErr_Format(PyExc_TypeError, "wrong number of arguments (%zd given)", PyTuple_Size(args));
            return nullptr;
        }
        python_network_stream &rns = get_python_network_stream();
        rns.send_mall_checkout_packet(mall_pos, inventory_pos);

        Py_RETURN_NONE;
    }

    auto net_send_answer_make_guild_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!PyArg_ParseTuple(args, "s", &name)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_answer_make_guild_packet(name);

        Py_RETURN_NONE;
    }

    auto net_send_quest_input_string_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *string;
        if (!PyArg_ParseTuple(args, "s", &string)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_quest_input_string_packet(string);

        Py_RETURN_NONE;
    }

    auto net_send_quest_confirm_packet(PyObject *self, PyObject *args) -> PyObject * {
        int answer, pid;
        if (!PyArg_ParseTuple(args, "ii", &answer, &pid)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_quest_confirm_packet(answer, pid);

        Py_RETURN_NONE;
    }

    auto net_send_guild_add_member_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!PyArg_ParseTuple(args, "i", &vid)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_add_member_packet(vid);

        Py_RETURN_NONE;
    }

    auto net_send_guild_remove_member_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *key;
        if (!PyArg_ParseTuple(args, "s", &key)) {
            return nullptr;
        }

        python_guild::guild_member_data *guild_member_data;
        if (!get_python_guild().member_data_ptr_by_name(key, &guild_member_data)) {
            metin2::log::warn("Guild member {} not found; cannot remove", key);
            Py_RETURN_NONE;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_remove_member_packet(guild_member_data->pid);

        Py_RETURN_NONE;
    }

    auto net_send_guild_change_grade_name_packet(PyObject *self, PyObject *args) -> PyObject * {
        int grade_number;
        char *grade_name;

        if (!PyArg_ParseTuple(args, "is", &grade_number, &grade_name)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_change_grade_name_packet(grade_number, grade_name);

        Py_RETURN_NONE;
    }

    auto net_send_guild_change_grade_authority_packet(PyObject *self, PyObject *args) -> PyObject * {
        int grade_number, authority;
        if (!PyArg_ParseTuple(args, "ii", &grade_number, &authority)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_change_grade_authority_packet(grade_number, authority);

        Py_RETURN_NONE;
    }

    auto net_send_guild_offer_packet(PyObject *self, PyObject *args) -> PyObject * {
        int experience;
        if (!PyArg_ParseTuple(args, "i", &experience)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_offer_packet(experience);

        Py_RETURN_NONE;
    }

    auto net_send_guild_post_comment_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *comment;
        if (!PyArg_ParseTuple(args, "s", &comment)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_post_comment_packet(comment);

        Py_RETURN_NONE;
    }

    auto net_send_guild_delete_comment_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_delete_comment_packet(index);

        Py_RETURN_NONE;
    }

    auto net_send_guild_refresh_comments_packet(PyObject *self, PyObject *args) -> PyObject * {
        int hightest_index;
        if (!PyArg_ParseTuple(args, "i", &hightest_index)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_refresh_comments_packet(hightest_index);

        Py_RETURN_NONE;
    }

    auto net_send_guild_change_member_grade_packet(PyObject *self, PyObject *args) -> PyObject * {
        int pid, grade_number;
        if (!PyArg_ParseTuple(args, "ii", &pid, &grade_number)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_change_member_grade_packet(pid, grade_number);

        Py_RETURN_NONE;
    }

    auto net_send_guild_use_skill_packet(PyObject *self, PyObject *args) -> PyObject * {
        int skill_id, target_vid;
        if (!PyArg_ParseTuple(args, "ii", &skill_id, &target_vid)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_use_skill_packet(skill_id, target_vid);

        Py_RETURN_NONE;
    }

    auto net_send_guild_change_member_general_packet(PyObject *self, PyObject *args) -> PyObject * {
        int pid, flag;
        if (!PyArg_ParseTuple(args, "ii", &pid, &flag)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_change_member_general_packet(pid, flag);

        Py_RETURN_NONE;
    }

    auto net_send_guild_invite_answer_packet(PyObject *self, PyObject *args) -> PyObject * {
        int guild_id, answer;
        if (!PyArg_ParseTuple(args, "ii", &guild_id, &answer)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_invite_answer_packet(guild_id, answer);

        Py_RETURN_NONE;
    }

    auto net_send_guild_charge_gsp_packet(PyObject *self, PyObject *args) -> PyObject * {
        int gsp;
        if (!PyArg_ParseTuple(args, "i", &gsp)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_charge_gsp_packet(gsp);

        Py_RETURN_NONE;
    }

    auto net_send_guild_deposit_money_packet(PyObject *self, PyObject *args) -> PyObject * {
        int gsp;
        if (!PyArg_ParseTuple(args, "i", &gsp)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_deposit_money_packet(gsp);

        Py_RETURN_NONE;
    }

    auto net_send_guild_withdraw_money_packet(PyObject *self, PyObject *args) -> PyObject * {
        int gsp;
        if (!PyArg_ParseTuple(args, "i", &gsp)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_withdraw_money_packet(gsp);

        Py_RETURN_NONE;
    }

    auto net_send_request_refine_info_packet(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!PyArg_ParseTuple(args, "i", &slot_index)) {
            return nullptr;
        }

        assert(!"netSendRequestRefineInfoPacket - this function is no longer used");

        Py_RETURN_NONE;
    }

    auto net_send_refine_packet(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index, type;
        if (!PyArg_ParseTuple(args, "ii", &slot_index, &type)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_refine_packet(slot_index, type);

        Py_RETURN_NONE;
    }

    auto net_send_select_item_packet(PyObject *self, PyObject *args) -> PyObject * {
        int item_pos;
        if (!PyArg_ParseTuple(args, "i", &item_pos)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_select_item_packet(item_pos);

        Py_RETURN_NONE;
    }

    auto net_set_packet_sequence_mode(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &rns = get_python_network_stream();
        account_connector &account_connector_value = get_account_connector();
        rns.set_packet_sequence_mode(true);
        account_connector_value.set_packet_sequence_mode(true);

        Py_RETURN_NONE;
    }

    auto net_set_empire_language_mode(PyObject *self, PyObject *args) -> PyObject * {
        int mode;
        if (!PyArg_ParseTuple(args, "i", &mode)) {
            return nullptr;
        }

        // The empire language conversion needed the Korean conversion tables, which only the YMIR locale
        // loaded; the shipped scripts switch it off. Kept for the scripts that still call it.

        Py_RETURN_NONE;
    }

    auto net_set_skill_group_fake(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.test_set_skill_group_fake(index);

        Py_RETURN_NONE;
    }

    void init_net_module() {
        static PyMethodDef s_methods[] =
            {
                {"enable_chat_insult_filter", net_enable_chat_insult_filter, METH_VARARGS},
                {"set_server_info", net_set_server_info, METH_VARARGS},
                {"get_server_info", net_get_server_info, METH_VARARGS},
                {"preserve_server_command", net_preserve_server_command, METH_VARARGS},
                {"get_preserved_server_command", net_get_preserved_server_command, METH_VARARGS},

                {"start_game", net_start_game, METH_VARARGS},
                {"warp", net_warp, METH_VARARGS},
                {"is_test", net_is_test, METH_VARARGS},
                {"set_mark_server", net_set_mark_server, METH_VARARGS},
                {"is_chat_insult_in", net_is_chat_insult_in, METH_VARARGS},
                {"is_insult_in", net_is_insult_in, METH_VARARGS},
                {"upload_mark", net_upload_mark, METH_VARARGS},
                {"upload_symbol", net_upload_symbol, METH_VARARGS},
                {"get_guild_id", net_get_guild_id, METH_VARARGS},
                {"get_empire_id", net_get_empire_id, METH_VARARGS},
                {"get_main_actor_race", net_get_main_actor_race, METH_VARARGS},
                {"get_main_actor_empire", net_get_main_actor_empire, METH_VARARGS},
                {"get_main_actor_skill_group", net_get_main_actor_skill_group, METH_VARARGS},
                {"get_account_character_slot_data_integer", net_get_account_character_slot_data_integer, METH_VARARGS},
                {"get_account_character_slot_data_string", net_get_account_character_slot_data_string, METH_VARARGS},

                {"get_field_music_file_name", net_get_field_music_file_name, METH_VARARGS},
                {"get_field_music_volume", net_get_field_music_volume, METH_VARARGS},

                {"toggle_game_debug_info", net_toggle_game_debug_info, METH_VARARGS},
                {"set_login_info", net_set_login_info, METH_VARARGS},
                {"set_phase_window", net_set_phase_window, METH_VARARGS},
                {"clear_phase_window", net_clear_phase_window, METH_VARARGS},
                {"set_server_command_parser_window", net_set_server_command_parser_window, METH_VARARGS},
                {"set_account_connector_handler", net_set_account_connector_handler, METH_VARARGS},
                {"set_handler", net_set_handler, METH_VARARGS},
                {"set_tcp_recv_buffer_size", net_set_tcp_recv_buffer_size, METH_VARARGS},
                {"set_tcp_send_buffer_size", net_set_tcp_send_buffer_size, METH_VARARGS},
                {"set_udp_recv_buffer_size", net_set_udp_recv_buffer_size, METH_VARARGS},
                {"direct_enter", net_direct_enter, METH_VARARGS},

                {"log_out_game", net_log_out_game, METH_VARARGS},
                {"exit_game", net_exit_game, METH_VARARGS},
                {"exit_application", net_exit_application, METH_VARARGS},
                {"connect_tcp", net_connect_tcp, METH_VARARGS},
                {"connect_udp", net_connect_udp, METH_VARARGS},
                {"connect_to_account_server", net_connect_to_account_server, METH_VARARGS},

                {"send_select_empire_packet", net_send_select_empire_packet, METH_VARARGS},
                {"send_select_character_packet", net_send_select_character_packet, METH_VARARGS},
                {"send_change_name_packet", net_send_change_name_packet, METH_VARARGS},
                {"send_create_character_packet", net_send_create_character_packet, METH_VARARGS},
                {"send_destroy_character_packet", net_send_destroy_character_packet, METH_VARARGS},
                {"send_enter_game_packet", net_send_enter_game_packet, METH_VARARGS},

                {"send_item_use_packet", net_send_item_use_packet, METH_VARARGS},
                {"send_item_use_to_item_packet", net_send_item_use_to_item_packet, METH_VARARGS},
                {"send_item_drop_packet_new", net_send_item_drop_packet_new, METH_VARARGS},
                {"send_gold_drop_packet_new", net_send_gold_drop_packet_new, METH_VARARGS},
                {"send_item_move_packet", net_send_item_move_packet, METH_VARARGS},
                {"send_give_item_packet", net_send_give_item_packet, METH_VARARGS},

                {"disconnect", net_disconnect, METH_VARARGS},

                {"send_chat_packet", net_send_chat_packet, METH_VARARGS},
                {"send_emoticon", net_send_emoticon, METH_VARARGS},
                {"send_whisper_packet", net_send_whisper_packet, METH_VARARGS},
                {"send_mobile_message_packet", net_send_mobile_message_packet, METH_VARARGS},

                {"send_shop_end_packet", net_send_shop_end_packet, METH_VARARGS},
                {"send_shop_buy_packet", net_send_shop_buy_packet, METH_VARARGS},
                {"send_shop_sell_packet_new", net_send_shop_sell_packet_new, METH_VARARGS},

                {"send_exchange_start_packet", net_send_exchange_start_packet, METH_VARARGS},
                {"send_exchange_item_add_packet", net_send_exchange_item_add_packet, METH_VARARGS},
                {"send_exchange_elk_add_packet", net_send_exchange_elk_add_packet, METH_VARARGS},
                {"send_exchange_accept_packet", net_send_exchange_accept_packet, METH_VARARGS},
                {"send_exchange_exit_packet", net_send_exchange_exit_packet, METH_VARARGS},

                {"send_on_click_packet", net_on_click_packet, METH_VARARGS},

                {"register_emoticon_string", net_register_emoticon_string, METH_VARARGS},

                {"send_messenger_add_by_vid_packet", net_send_messenger_add_by_vid_packet, METH_VARARGS},
                {"send_messenger_add_by_name_packet", net_send_messenger_add_by_name_packet, METH_VARARGS},
                {"send_messenger_remove_packet", net_send_messenger_remove_packet, METH_VARARGS},

                {"send_party_invite_packet", net_send_party_invite_packet, METH_VARARGS},
                {"send_party_invite_answer_packet", net_send_party_invite_answer_packet, METH_VARARGS},
                {"send_party_exit_packet", net_send_party_exit_packet, METH_VARARGS},
                {"send_party_remove_packet", net_send_party_remove_packet, METH_VARARGS},
                {"send_party_set_state_packet", net_send_party_set_state_packet, METH_VARARGS},
                {"send_party_use_skill_packet", net_send_party_use_skill_packet, METH_VARARGS},
                {"send_party_parameter_packet", net_send_party_parameter_packet, METH_VARARGS},

                {"send_safebox_save_money_packet", net_send_safebox_save_money_packet, METH_VARARGS},
                {"send_safebox_withdraw_money_packet", net_send_safebox_withdraw_money_packet, METH_VARARGS},
                {"send_safebox_checkin_packet", net_send_safebox_checkin_packet, METH_VARARGS},
                {"send_safebox_checkout_packet", net_send_safebox_checkout_packet, METH_VARARGS},
                {"send_safebox_item_move_packet", net_send_safebox_item_move_packet, METH_VARARGS},

                {"send_mall_checkout_packet", net_send_mall_checkout_packet, METH_VARARGS},

                {"send_answer_make_guild_packet", net_send_answer_make_guild_packet, METH_VARARGS},
                {"send_quest_input_string_packet", net_send_quest_input_string_packet, METH_VARARGS},
                {"send_quest_confirm_packet", net_send_quest_confirm_packet, METH_VARARGS},
                {"send_guild_add_member_packet", net_send_guild_add_member_packet, METH_VARARGS},
                {"send_guild_remove_member_packet", net_send_guild_remove_member_packet, METH_VARARGS},
                {"send_guild_change_grade_name_packet", net_send_guild_change_grade_name_packet, METH_VARARGS},
                {"send_guild_change_grade_authority_packet", net_send_guild_change_grade_authority_packet, METH_VARARGS},
                {"send_guild_offer_packet", net_send_guild_offer_packet, METH_VARARGS},
                {"send_guild_post_comment_packet", net_send_guild_post_comment_packet, METH_VARARGS},
                {"send_guild_delete_comment_packet", net_send_guild_delete_comment_packet, METH_VARARGS},
                {"send_guild_refresh_comments_packet", net_send_guild_refresh_comments_packet, METH_VARARGS},
                {"send_guild_change_member_grade_packet", net_send_guild_change_member_grade_packet, METH_VARARGS},
                {"send_guild_use_skill_packet", net_send_guild_use_skill_packet, METH_VARARGS},
                {"send_guild_change_member_general_packet", net_send_guild_change_member_general_packet, METH_VARARGS},
                {"send_guild_invite_answer_packet", net_send_guild_invite_answer_packet, METH_VARARGS},
                {"send_guild_charge_gsp_packet", net_send_guild_charge_gsp_packet, METH_VARARGS},
                {"send_guild_deposit_money_packet", net_send_guild_deposit_money_packet, METH_VARARGS},
                {"send_guild_withdraw_money_packet", net_send_guild_withdraw_money_packet, METH_VARARGS},

                {"send_request_refine_info_packet", net_send_request_refine_info_packet, METH_VARARGS},
                {"send_refine_packet", net_send_refine_packet, METH_VARARGS},
                {"send_select_item_packet", net_send_select_item_packet, METH_VARARGS},

                {"set_packet_sequence_mode", net_set_packet_sequence_mode, METH_VARARGS},
                {"set_empire_language_mode", net_set_empire_language_mode, METH_VARARGS},

                {"set_skill_group_fake", net_set_skill_group_fake, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module = init_python_module("metin2.net", s_methods);

        PyModule_AddIntConstant(module, "ERROR_NONE", python_network_stream::ERROR_NONE);
        PyModule_AddIntConstant(module, "ERROR_CONNECT_MARK_SERVER", python_network_stream::ERROR_CONNECT_MARK_SERVER);
        PyModule_AddIntConstant(module, "ERROR_LOAD_MARK", python_network_stream::ERROR_LOAD_MARK);
        PyModule_AddIntConstant(module, "ERROR_MARK_WIDTH", python_network_stream::ERROR_MARK_WIDTH);
        PyModule_AddIntConstant(module, "ERROR_MARK_HEIGHT", python_network_stream::ERROR_MARK_HEIGHT);

        PyModule_AddIntConstant(module, "ERROR_MARK_UPLOAD_NEED_RECONNECT", python_network_stream::ERROR_MARK_UPLOAD_NEED_RECONNECT);
        PyModule_AddIntConstant(module, "ERROR_MARK_CHECK_NEED_RECONNECT", python_network_stream::ERROR_MARK_CHECK_NEED_RECONNECT);

        PyModule_AddIntConstant(module, "PHASE_WINDOW_LOGIN", python_network_stream::PHASE_WINDOW_LOGIN);
        PyModule_AddIntConstant(module, "PHASE_WINDOW_SELECT", python_network_stream::PHASE_WINDOW_SELECT);
        PyModule_AddIntConstant(module, "PHASE_WINDOW_CREATE", python_network_stream::PHASE_WINDOW_CREATE);
        PyModule_AddIntConstant(module, "PHASE_WINDOW_LOAD", python_network_stream::PHASE_WINDOW_LOAD);
        PyModule_AddIntConstant(module, "PHASE_WINDOW_GAME", python_network_stream::PHASE_WINDOW_GAME);
        PyModule_AddIntConstant(module, "PHASE_WINDOW_EMPIRE", python_network_stream::PHASE_WINDOW_EMPIRE);
        PyModule_AddIntConstant(module, "PHASE_WINDOW_LOGO", python_network_stream::PHASE_WINDOW_LOGO);

        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_ID", python_network_stream::ACCOUNT_CHARACTER_SLOT_ID);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_NAME", python_network_stream::ACCOUNT_CHARACTER_SLOT_NAME);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_RACE", python_network_stream::ACCOUNT_CHARACTER_SLOT_RACE);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_LEVEL", python_network_stream::ACCOUNT_CHARACTER_SLOT_LEVEL);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_STR", python_network_stream::ACCOUNT_CHARACTER_SLOT_STR);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_DEX", python_network_stream::ACCOUNT_CHARACTER_SLOT_DEX);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_INT", python_network_stream::ACCOUNT_CHARACTER_SLOT_INT);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_HTH", python_network_stream::ACCOUNT_CHARACTER_SLOT_HTH);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_PLAYTIME", python_network_stream::ACCOUNT_CHARACTER_SLOT_PLAYTIME);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_FORM", python_network_stream::ACCOUNT_CHARACTER_SLOT_FORM);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_ADDR", python_network_stream::ACCOUNT_CHARACTER_SLOT_ADDR);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_PORT", python_network_stream::ACCOUNT_CHARACTER_SLOT_PORT);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_GUILD_ID", python_network_stream::ACCOUNT_CHARACTER_SLOT_GUILD_ID);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_GUILD_NAME", python_network_stream::ACCOUNT_CHARACTER_SLOT_GUILD_NAME);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_CHANGE_NAME_FLAG", python_network_stream::ACCOUNT_CHARACTER_SLOT_CHANGE_NAME_FLAG);
        PyModule_AddIntConstant(module, "ACCOUNT_CHARACTER_SLOT_HAIR", python_network_stream::ACCOUNT_CHARACTER_SLOT_HAIR);

        PyModule_AddIntConstant(module, "SERVER_COMMAND_LOG_OUT", python_network_stream::SERVER_COMMAND_LOG_OUT);
        PyModule_AddIntConstant(module, "SERVER_COMMAND_RETURN_TO_SELECT_CHARACTER", python_network_stream::SERVER_COMMAND_RETURN_TO_SELECT_CHARACTER);
        PyModule_AddIntConstant(module, "SERVER_COMMAND_QUIT", python_network_stream::SERVER_COMMAND_QUIT);

        PyModule_AddIntConstant(module, "EMPIRE_A", 1);
        PyModule_AddIntConstant(module, "EMPIRE_B", 2);
        PyModule_AddIntConstant(module, "EMPIRE_C", 3);
        PyModule_AddIntConstant(module, "DS_SUB_HEADER_REFINE_FAIL", DS_SUB_HEADER_REFINE_FAIL);
        PyModule_AddIntConstant(module, "DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE", DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE);
        PyModule_AddIntConstant(module, "DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL", DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL);
        PyModule_AddIntConstant(module, "DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY", DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY);
        PyModule_AddIntConstant(module, "DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL", DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL);
        PyModule_AddIntConstant(module, "DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL", DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL);
        PyModule_AddIntConstant(module, "DS_SUB_HEADER_REFINE_SUCCEED", DS_SUB_HEADER_REFINE_SUCCEED);
    }
}
