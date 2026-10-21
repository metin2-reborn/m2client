

#include <metin2/client/app/abstract_player.h>
#include <metin2/client/app/test.h>
#include <metin2/client/guilds/guild_mark_downloader.h>
#include <metin2/client/guilds/guild_mark_uploader.h>
#include <metin2/client/guilds/python_guild.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include "account_connector.h"
#include "python_network_stream.h"

namespace metin2::inline client::inline protocol {
    static std::string server_info;
    std::list<std::string> list_command;

    auto net_set_server_info(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name))
            return py_build_exception();

        server_info = file_name;
        return py_build_none();
    }

    auto net_get_server_info(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("s", server_info.c_str());
    }

    auto net_preserve_server_command(PyObject *self, PyObject *args) -> PyObject * {
        char *line;
        if (!py_tuple_get_string(args, 0, &line))
            return py_build_exception();

        list_command.push_back(line);

        return py_build_none();
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

        return py_build_none();
    }

    auto net_is_test(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", is_test_server_mode);
    }

    auto net_warp(PyObject *self, PyObject *args) -> PyObject * {
        int x;
        if (!py_tuple_get_integer(args, 0, &x))
            return py_build_exception();

        int y;
        if (!py_tuple_get_integer(args, 1, &y))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.warp(x, y);

        return py_build_none();
    }

    auto net_upload_mark(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.upload_mark(file_name));
    }

    auto net_upload_symbol(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name))
            return py_build_exception();

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
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        int type;
        if (!py_tuple_get_integer(args, 1, &type))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        uint32_t result = net_stream.account_character_slot_datau(index, type);
        return Py_BuildValue("i", result);
    }

    auto net_get_account_character_slot_data_string(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        int type;
        if (!py_tuple_get_integer(args, 1, &type))
            return py_build_exception();

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
        if (!py_tuple_get_integer(args, 0, &e_phase_wnd))
            return py_build_exception();

        PyObject *po_phase_wnd;
        if (!py_tuple_get_object(args, 1, &po_phase_wnd))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_phase_window(e_phase_wnd, po_phase_wnd);
        return py_build_none();
    }

    auto net_clear_phase_window(PyObject *self, PyObject *args) -> PyObject * {
        int e_phase_wnd;
        if (!py_tuple_get_integer(args, 0, &e_phase_wnd))
            return py_build_exception();

        PyObject *po_phase_wnd;
        if (!py_tuple_get_object(args, 1, &po_phase_wnd))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.clear_phase_window(e_phase_wnd, po_phase_wnd);
        return py_build_none();
    }

    auto net_set_server_command_parser_window(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *phase_wnd;
        if (!py_tuple_get_object(args, 0, &phase_wnd))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_server_command_parser_window(phase_wnd);
        return py_build_none();
    }

    auto net_set_account_connector_handler(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *phase_wnd;
        if (!py_tuple_get_object(args, 0, &phase_wnd))
            return py_build_exception();

        account_connector &account_connector_value = get_account_connector();
        account_connector_value.set_handler(phase_wnd);
        return py_build_none();
    }

    auto net_set_handler(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handler;

        if (!py_tuple_get_object(args, 0, &handler))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_handler(handler);
        return py_build_none();
    }

    auto net_set_tcp_recv_buffer_size(PyObject *self, PyObject *args) -> PyObject * {
        int buf_size;
        if (!py_tuple_get_integer(args, 0, &buf_size))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_recv_buffer_size(buf_size);
        return py_build_none();
    }

    auto net_set_tcp_send_buffer_size(PyObject *self, PyObject *args) -> PyObject * {
        int buf_size;
        if (!py_tuple_get_integer(args, 0, &buf_size))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_send_buffer_size(buf_size);
        return py_build_none();
    }

    auto net_set_udp_recv_buffer_size(PyObject *self, PyObject *args) -> PyObject * {
        int buf_size;
        if (!py_tuple_get_integer(args, 0, &buf_size))
            return py_build_exception();

        return py_build_none();
    }

    auto net_set_mark_server(PyObject *self, PyObject *args) -> PyObject * {
        char *addr;
        if (!py_tuple_get_string(args, 0, &addr))
            return py_build_exception();

        int port;
        if (!py_tuple_get_integer(args, 1, &port))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_mark_server(addr, port);
        return py_build_none();
    }

    auto net_connect_tcp(PyObject *self, PyObject *args) -> PyObject * {
        char *addr;
        if (!py_tuple_get_string(args, 0, &addr))
            return py_build_exception();

        int port;
        if (!py_tuple_get_integer(args, 1, &port))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.connect_login_server(addr, port);
        return py_build_none();
    }

    auto net_connect_udp(PyObject *self, PyObject *args) -> PyObject * {
        char *ip;
        if (!py_tuple_get_string(args, 0, &ip))
            return py_build_exception();
        int port;
        if (!py_tuple_get_integer(args, 1, &port))
            return py_build_exception();

        return py_build_none();
    }

    auto net_connect_to_account_server(PyObject *self, PyObject *args) -> PyObject * {
        char *addr;
        if (!py_tuple_get_string(args, 0, &addr))
            return py_build_exception();

        int port;
        if (!py_tuple_get_integer(args, 1, &port))
            return py_build_exception();

        char *account_addr;
        if (!py_tuple_get_string(args, 2, &account_addr))
            return py_build_exception();

        int account_port;
        if (!py_tuple_get_integer(args, 3, &account_port))
            return py_build_exception();

        account_connector &account_connector_value = get_account_connector();
        account_connector_value.connect(addr, port, account_addr, account_port);
        return py_build_none();
    }

    auto net_set_login_info(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        char *pwd;
        if (!py_tuple_get_string(args, 1, &pwd))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        account_connector &account_connector_value = get_account_connector();
        net_stream.set_login_info(name, pwd);
        account_connector_value.set_login_info(name, pwd);
        return py_build_none();
    }

    auto net_send_select_empire_packet(PyObject *self, PyObject *args) -> PyObject * {
        int empire_index;
        if (!py_tuple_get_integer(args, 0, &empire_index))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_select_empire_packet(empire_index);
        return py_build_none();
    }

    auto net_direct_enter(PyObject *self, PyObject *args) -> PyObject * {
        int chr_slot;
        if (!py_tuple_get_integer(args, 0, &chr_slot))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.connect_game_server(chr_slot);
        return py_build_none();
    }

    auto net_send_select_character_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_select_character_packet((uint8_t)index);
        return py_build_none();
    }

    auto net_send_change_name_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();
        char *name;
        if (!py_tuple_get_string(args, 1, &name))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_change_name_packet((uint8_t)index, name);
        return py_build_none();
    }

    auto net_enable_chat_insult_filter(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!py_tuple_get_integer(args, 0, &enable))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.enable_chat_insult_filter(enable ? true : false);
        return py_build_none();
    }

    auto net_is_chat_insult_in(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!py_tuple_get_string(args, 0, &msg))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.is_chat_insult_in(msg));
    }

    auto net_is_insult_in(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!py_tuple_get_string(args, 0, &msg))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        return Py_BuildValue("i", net_stream.is_insult_in(msg));
    }

    auto net_send_whisper_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        char *line;

        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        if (!py_tuple_get_string(args, 1, &line))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_whisper_packet(name, line);
        return py_build_none();
    }

    auto net_send_mobile_message_packet(PyObject *self, PyObject *args) -> PyObject * {
        return py_build_none();
    }

    auto net_send_chat_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *line;
        if (!py_tuple_get_string(args, 0, &line))
            return py_build_exception();
        int type;
        if (!py_tuple_get_integer(args, 1, &type)) {
            type = CHAT_TYPE_TALKING;
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_chat_packet(line, type);
        return py_build_none();
    }

    auto net_send_emoticon(PyObject *self, PyObject *args) -> PyObject * {
        int emoticon;
        if (!py_tuple_get_integer(args, 0, &emoticon))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_emoticon(emoticon);
        return py_build_none();
    }

    auto net_send_create_character_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        char *name;
        if (!py_tuple_get_string(args, 1, &name))
            return py_build_exception();

        int job;
        if (!py_tuple_get_integer(args, 2, &job))
            return py_build_exception();

        int shape;
        if (!py_tuple_get_integer(args, 3, &shape))
            return py_build_exception();

        int stat1;
        if (!py_tuple_get_integer(args, 4, &stat1))
            return py_build_exception();
        int stat2;
        if (!py_tuple_get_integer(args, 5, &stat2))
            return py_build_exception();
        int stat3;
        if (!py_tuple_get_integer(args, 6, &stat3))
            return py_build_exception();
        int stat4;
        if (!py_tuple_get_integer(args, 7, &stat4))
            return py_build_exception();

        if (index < 0 || index > 3)
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_create_character_packet((uint8_t)index, name, (uint8_t)job, (uint8_t)shape, stat1, stat2, stat3, stat4);
        return py_build_none();
    }

    auto net_send_destroy_character_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        char *private_code;
        if (!py_tuple_get_string(args, 1, &private_code))
            return py_build_exception();

        if (index < 0 || index > 3)
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_destroy_character_packet((uint8_t)index, private_code);
        return py_build_none();
    }

    auto net_send_enter_game_packet(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_enter_game();
        return py_build_none();
    }

    auto net_on_click_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_on_click_packet(index);

        return py_build_none();
    }

    auto net_send_item_use_packet(PyObject *self, PyObject *args) -> PyObject * {
        item_pos cell;
        switch (PyTuple_Size(args)) {
        case 1:
            if (!py_tuple_get_integer(args, 0, &cell.cell))
                return py_build_exception();
            break;
        case 2:
            if (!py_tuple_get_byte(args, 0, &cell.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &cell.cell))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_item_use_packet(cell);
        return py_build_none();
    }

    auto net_send_item_use_to_item_packet(PyObject *self, PyObject *args) -> PyObject * {
        item_pos source_cell;
        item_pos target_cell;
        switch (PyTuple_Size(args)) {
        case 2:
            if (!py_tuple_get_integer(args, 0, &source_cell.cell))
                return py_build_exception();

            if (!py_tuple_get_integer(args, 1, &target_cell.cell))
                return py_build_exception();
            break;
        case 4:
            if (!py_tuple_get_byte(args, 0, &source_cell.window_type))
                return py_build_exception();

            if (!py_tuple_get_integer(args, 1, &source_cell.cell))
                return py_build_exception();

            if (!py_tuple_get_byte(args, 2, &target_cell.window_type))
                return py_build_exception();

            if (!py_tuple_get_integer(args, 3, &target_cell.cell))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_item_use_to_item_packet(source_cell, target_cell);
        return py_build_none();
    }

    auto net_send_item_drop_packet_new(PyObject *self, PyObject *args) -> PyObject * {
        item_pos cell;
        int count;
        switch (PyTuple_Size(args)) {
        case 2:
            if (!py_tuple_get_integer(args, 0, &cell.cell))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &count))
                return py_build_exception();

            break;
        case 3:
            if (!py_tuple_get_byte(args, 0, &cell.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &cell.cell))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 2, &count))
                return py_build_exception();

            break;
        default:
            return py_build_exception();
        }
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_item_drop_packet_new(cell, 0, count);
        return py_build_none();
    }

    auto net_send_gold_drop_packet_new(PyObject *self, PyObject *args) -> PyObject * {
        int elk;
        if (!py_tuple_get_integer(args, 0, &elk))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_item_drop_packet_new(item_pos(RESERVED_WINDOW, 0), (uint32_t)elk, 0);
        return py_build_none();
    }

    auto net_send_item_move_packet(PyObject *self, PyObject *args) -> PyObject * {
        item_pos cell;
        item_pos change_cell;
        int num;

        switch (PyTuple_Size(args)) {
        case 3:
            if (!py_tuple_get_integer(args, 0, &cell.cell))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &change_cell.cell))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 2, &num))
                return py_build_exception();
            break;
        case 5:
            {
                if (!py_tuple_get_byte(args, 0, &cell.window_type))
                    return py_build_exception();
                if (!py_tuple_get_integer(args, 1, &cell.cell))
                    return py_build_exception();
                if (!py_tuple_get_byte(args, 2, &change_cell.window_type))
                    return py_build_exception();
                if (!py_tuple_get_integer(args, 3, &change_cell.cell))
                    return py_build_exception();
                if (!py_tuple_get_integer(args, 4, &num))
                    return py_build_exception();
            }
            break;
        default:
            return py_build_exception();
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_item_move_packet(cell, change_cell, (uint8_t)num);
        return py_build_none();
    }

    auto net_send_give_item_packet(PyObject *self, PyObject *args) -> PyObject * {
        int target_vid;
        item_pos cell;
        int item_count;
        switch (PyTuple_Size(args)) {
        case 3:
            if (!py_tuple_get_integer(args, 0, &target_vid))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &cell.cell))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 2, &item_count))
                return py_build_exception();
            break;
        case 4:
            if (!py_tuple_get_integer(args, 0, &target_vid))
                return py_build_exception();
            if (!py_tuple_get_byte(args, 1, &cell.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 2, &cell.cell))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 3, &item_count))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_give_item_packet(target_vid, cell, item_count);
        return py_build_none();
    }

    auto net_send_shop_end_packet(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_shop_end_packet();
        return py_build_none();
    }

    auto net_send_shop_buy_packet(PyObject *self, PyObject *args) -> PyObject * {
        int count;
        if (!py_tuple_get_integer(args, 0, &count))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_shop_buy_packet(count);
        return py_build_none();
    }

    auto net_send_shop_sell_packet_new(PyObject *self, PyObject *args) -> PyObject * {
        int slot_number;
        if (!py_tuple_get_integer(args, 0, &slot_number))
            return py_build_exception();
        int count;
        if (!py_tuple_get_integer(args, 1, &count))
            return py_build_exception();
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_shop_sell_packet_new(slot_number, count);
        return py_build_none();
    }

    auto net_send_exchange_start_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_exchange_start_packet(vid);
        return py_build_none();
    }

    auto net_send_exchange_elk_add_packet(PyObject *self, PyObject *args) -> PyObject * {
        int elk;
        if (!py_tuple_get_integer(args, 0, &elk))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_exchange_elk_add_packet(elk);
        return py_build_none();
    }

    auto net_send_exchange_item_add_packet(PyObject *self, PyObject *args) -> PyObject * {
        uint8_t window_type;
        if (!py_tuple_get_integer(args, 0, &window_type))
            return py_build_exception();
        uint16_t slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();
        int display_slot_index;
        if (!py_tuple_get_integer(args, 2, &display_slot_index))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_exchange_item_add_packet(item_pos(window_type, slot_index), display_slot_index);
        return py_build_none();
    }

    auto net_send_exchange_accept_packet(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_exchange_accept_packet();
        return py_build_none();
    }

    auto net_send_exchange_exit_packet(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_exchange_exit_packet();
        return py_build_none();
    }

    auto net_exit_application(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.exit_application();
        return py_build_none();
    }

    auto net_exit_game(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.exit_game();
        return py_build_none();
    }

    auto net_log_out_game(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.log_out_game();
        return py_build_none();
    }

    auto net_disconnect(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.set_off_line_phase();
        net_stream.disconnect();

        return py_build_none();
    }

    auto net_toggle_game_debug_info(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.toggle_game_debug_info();
        return py_build_none();
    }

    auto net_register_emoticon_string(PyObject *self, PyObject *args) -> PyObject * {
        char *emoticon_string;
        if (!py_tuple_get_string(args, 0, &emoticon_string))
            return py_build_exception();

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.register_emoticon_string(emoticon_string);
        return py_build_none();
    }

    auto net_send_messenger_add_by_vid_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_messenger_add_by_vid_packet(vid);

        return py_build_none();
    }

    auto net_send_messenger_add_by_name_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_messenger_add_by_name_packet(name);

        return py_build_none();
    }

    auto net_send_messenger_remove_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *key;
        if (!py_tuple_get_string(args, 0, &key))
            return py_build_exception();
        char *name;
        if (!py_tuple_get_string(args, 1, &name))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_messenger_remove_packet(key, name);

        return py_build_none();
    }

    auto net_send_party_invite_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_invite_packet(vid);

        return py_build_none();
    }

    auto net_send_party_invite_answer_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();
        int answer;
        if (!py_tuple_get_integer(args, 1, &answer))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_invite_answer_packet(vid, answer);

        return py_build_none();
    }

    auto net_send_party_exit_packet(PyObject *self, PyObject *args) -> PyObject * {
        abstract_player &player = get_abstract_player();

        python_network_stream &rns = get_python_network_stream();

        uint32_t vid = player.main_character_index();
        uint32_t pid;
        if (player.party_member_vid_to_pid(vid, &pid))
            rns.send_party_remove_packet(pid);

        return py_build_none();
    }

    auto net_send_party_remove_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_remove_packet(vid);

        return py_build_none();
    }

    auto net_send_party_set_state_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();
        int state;
        if (!py_tuple_get_integer(args, 1, &state))
            return py_build_exception();
        int flag;
        if (!py_tuple_get_integer(args, 2, &flag))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_set_state_packet(vid, state, flag);

        return py_build_none();
    }

    auto net_send_party_use_skill_packet(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_build_exception();
        int vid;
        if (!py_tuple_get_integer(args, 1, &vid))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_use_skill_packet(skill_index, vid);

        return py_build_none();
    }

    auto net_send_party_parameter_packet(PyObject *self, PyObject *args) -> PyObject * {
        int mode;
        if (!py_tuple_get_integer(args, 0, &mode))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_party_parameter_packet(mode);

        return py_build_none();
    }

    auto net_send_safebox_save_money_packet(PyObject *self, PyObject *args) -> PyObject * {
        int money;
        if (!py_tuple_get_integer(args, 0, &money))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_safe_box_money_packet(SAFEBOX_MONEY_STATE_SAVE, money);

        return py_build_none();
    }

    auto net_send_safebox_withdraw_money_packet(PyObject *self, PyObject *args) -> PyObject * {
        int money;
        if (!py_tuple_get_integer(args, 0, &money))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_safe_box_money_packet(SAFEBOX_MONEY_STATE_WITHDRAW, money);

        return py_build_none();
    }

    auto net_send_safebox_checkin_packet(PyObject *self, PyObject *args) -> PyObject * {
        item_pos inventory_pos;
        int safe_box_pos;

        switch (PyTuple_Size(args)) {
        case 2:
            inventory_pos.window_type = INVENTORY;
            if (!py_tuple_get_integer(args, 0, &inventory_pos.cell))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &safe_box_pos))
                return py_build_exception();
            break;
        case 3:
            if (!py_tuple_get_integer(args, 0, &inventory_pos.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &inventory_pos.cell))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 2, &safe_box_pos))
                return py_build_exception();
            break;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_safe_box_checkin_packet(inventory_pos, safe_box_pos);

        return py_build_none();
    }

    auto net_send_safebox_checkout_packet(PyObject *self, PyObject *args) -> PyObject * {
        int safe_box_pos;
        item_pos inventory_pos;

        switch (PyTuple_Size(args)) {
        case 2:
            if (!py_tuple_get_integer(args, 0, &safe_box_pos))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &inventory_pos.cell))
                return py_build_exception();
            break;
        case 3:
            if (!py_tuple_get_integer(args, 0, &safe_box_pos))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &inventory_pos.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 2, &inventory_pos.cell))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_safe_box_checkout_packet(safe_box_pos, inventory_pos);

        return py_build_none();
    }

    auto net_send_safebox_item_move_packet(PyObject *self, PyObject *args) -> PyObject * {
        int source_pos;
        if (!py_tuple_get_integer(args, 0, &source_pos))
            return py_build_exception();
        int target_pos;
        if (!py_tuple_get_integer(args, 1, &target_pos))
            return py_build_exception();
        int count;
        if (!py_tuple_get_integer(args, 2, &count))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_safe_box_item_move_packet(source_pos, target_pos, count);

        return py_build_none();
    }

    auto net_send_mall_checkout_packet(PyObject *self, PyObject *args) -> PyObject * {
        int mall_pos;
        item_pos inventory_pos;

        switch (PyTuple_Size(args)) {
        case 2:
            if (!py_tuple_get_integer(args, 0, &mall_pos))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &inventory_pos.cell))
                return py_build_exception();
            break;
        case 3:
            if (!py_tuple_get_integer(args, 0, &mall_pos))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &inventory_pos.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 2, &inventory_pos.cell))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }
        python_network_stream &rns = get_python_network_stream();
        rns.send_mall_checkout_packet(mall_pos, inventory_pos);

        return py_build_none();
    }

    auto net_send_answer_make_guild_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_answer_make_guild_packet(name);

        return py_build_none();
    }

    auto net_send_quest_input_string_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *string;
        if (!py_tuple_get_string(args, 0, &string))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_quest_input_string_packet(string);

        return py_build_none();
    }

    auto net_send_quest_confirm_packet(PyObject *self, PyObject *args) -> PyObject * {
        int answer;
        if (!py_tuple_get_integer(args, 0, &answer))
            return py_build_exception();
        int pid;
        if (!py_tuple_get_integer(args, 1, &pid))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_quest_confirm_packet(answer, pid);

        return py_build_none();
    }

    auto net_send_guild_add_member_packet(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_add_member_packet(vid);

        return py_build_none();
    }

    auto net_send_guild_remove_member_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *key;
        if (!py_tuple_get_string(args, 0, &key))
            return py_build_exception();

        python_guild::guild_member_data *guild_member_data;
        if (!get_python_guild().member_data_ptr_by_name(key, &guild_member_data)) {
            metin2::log::warn("Guild member {} not found; cannot remove", key);
            return py_build_none();
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_remove_member_packet(guild_member_data->pid);

        return py_build_none();
    }

    auto net_send_guild_change_grade_name_packet(PyObject *self, PyObject *args) -> PyObject * {
        int grade_number;
        if (!py_tuple_get_integer(args, 0, &grade_number))
            return py_build_exception();
        char *grade_name;
        if (!py_tuple_get_string(args, 1, &grade_name))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_change_grade_name_packet(grade_number, grade_name);

        return py_build_none();
    }

    auto net_send_guild_change_grade_authority_packet(PyObject *self, PyObject *args) -> PyObject * {
        int grade_number;
        if (!py_tuple_get_integer(args, 0, &grade_number))
            return py_build_exception();
        int authority;
        if (!py_tuple_get_integer(args, 1, &authority))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_change_grade_authority_packet(grade_number, authority);

        return py_build_none();
    }

    auto net_send_guild_offer_packet(PyObject *self, PyObject *args) -> PyObject * {
        int experience;
        if (!py_tuple_get_integer(args, 0, &experience))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_offer_packet(experience);

        return py_build_none();
    }

    auto net_sned_guild_post_comment_packet(PyObject *self, PyObject *args) -> PyObject * {
        char *comment;
        if (!py_tuple_get_string(args, 0, &comment))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_post_comment_packet(comment);

        return py_build_none();
    }

    auto net_sned_guild_delete_comment_packet(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_delete_comment_packet(index);

        return py_build_none();
    }

    auto net_send_guild_refresh_comments_packet(PyObject *self, PyObject *args) -> PyObject * {
        int hightest_index;
        if (!py_tuple_get_integer(args, 0, &hightest_index))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_refresh_comments_packet(hightest_index);

        return py_build_none();
    }

    auto net_send_guild_change_member_grade_packet(PyObject *self, PyObject *args) -> PyObject * {
        int pid;
        if (!py_tuple_get_integer(args, 0, &pid))
            return py_build_exception();
        int grade_number;
        if (!py_tuple_get_integer(args, 1, &grade_number))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_change_member_grade_packet(pid, grade_number);

        return py_build_none();
    }

    auto net_send_guild_use_skill_packet(PyObject *self, PyObject *args) -> PyObject * {
        int skill_id;
        if (!py_tuple_get_integer(args, 0, &skill_id))
            return py_build_exception();
        int target_vid;
        if (!py_tuple_get_integer(args, 1, &target_vid))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_use_skill_packet(skill_id, target_vid);

        return py_build_none();
    }

    auto net_send_guild_change_member_general_packet(PyObject *self, PyObject *args) -> PyObject * {
        int pid;
        if (!py_tuple_get_integer(args, 0, &pid))
            return py_build_exception();
        int flag;
        if (!py_tuple_get_integer(args, 1, &flag))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_change_member_general_packet(pid, flag);

        return py_build_none();
    }

    auto net_send_guild_invite_answer_packet(PyObject *self, PyObject *args) -> PyObject * {
        int guild_id;
        if (!py_tuple_get_integer(args, 0, &guild_id))
            return py_build_exception();
        int answer;
        if (!py_tuple_get_integer(args, 1, &answer))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_invite_answer_packet(guild_id, answer);

        return py_build_none();
    }

    auto net_send_guild_charge_gsp_packet(PyObject *self, PyObject *args) -> PyObject * {
        int gsp;
        if (!py_tuple_get_integer(args, 0, &gsp))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_charge_gsp_packet(gsp);

        return py_build_none();
    }

    auto net_send_guild_deposit_money_packet(PyObject *self, PyObject *args) -> PyObject * {
        int gsp;
        if (!py_tuple_get_integer(args, 0, &gsp))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_deposit_money_packet(gsp);

        return py_build_none();
    }

    auto net_send_guild_withdraw_money_packet(PyObject *self, PyObject *args) -> PyObject * {
        int gsp;
        if (!py_tuple_get_integer(args, 0, &gsp))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_guild_withdraw_money_packet(gsp);

        return py_build_none();
    }

    auto net_send_request_refine_info_packet(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        assert(!"netSendRequestRefineInfoPacket - this function is no longer used");

        return py_build_none();
    }

    auto net_send_refine_packet(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();
        int type;
        if (!py_tuple_get_integer(args, 1, &type))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_refine_packet(slot_index, type);

        return py_build_none();
    }

    auto net_send_select_item_packet(PyObject *self, PyObject *args) -> PyObject * {
        int item_pos;
        if (!py_tuple_get_integer(args, 0, &item_pos))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.send_select_item_packet(item_pos);

        return py_build_none();
    }

    auto net_set_packet_sequence_mode(PyObject *self, PyObject *args) -> PyObject * {
        python_network_stream &rns = get_python_network_stream();
        account_connector &account_connector_value = get_account_connector();
        rns.set_packet_sequence_mode(true);
        account_connector_value.set_packet_sequence_mode(true);

        return py_build_none();
    }

    auto net_set_empire_language_mode(PyObject *self, PyObject *args) -> PyObject * {
        int mode;
        if (!py_tuple_get_integer(args, 0, &mode))
            return py_build_exception();

        // The empire language conversion needed the Korean conversion tables, which only the YMIR locale
        // loaded; the shipped scripts switch it off. Kept for the scripts that still call it.

        return py_build_none();
    }

    auto net_set_skill_group_fake(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        python_network_stream &rns = get_python_network_stream();
        rns.test_set_skill_group_fake(index);

        return py_build_none();
    }

    void initnet() {
        static PyMethodDef s_methods[] =
            {
                {"EnableChatInsultFilter", net_enable_chat_insult_filter, METH_VARARGS},
                {"SetServerInfo", net_set_server_info, METH_VARARGS},
                {"GetServerInfo", net_get_server_info, METH_VARARGS},
                {"PreserveServerCommand", net_preserve_server_command, METH_VARARGS},
                {"GetPreservedServerCommand", net_get_preserved_server_command, METH_VARARGS},

                {"StartGame", net_start_game, METH_VARARGS},
                {"Warp", net_warp, METH_VARARGS},
                {"IsTest", net_is_test, METH_VARARGS},
                {"SetMarkServer", net_set_mark_server, METH_VARARGS},
                {"IsChatInsultIn", net_is_chat_insult_in, METH_VARARGS},
                {"IsInsultIn", net_is_insult_in, METH_VARARGS},
                {"UploadMark", net_upload_mark, METH_VARARGS},
                {"UploadSymbol", net_upload_symbol, METH_VARARGS},
                {"GetGuildID", net_get_guild_id, METH_VARARGS},
                {"GetEmpireID", net_get_empire_id, METH_VARARGS},
                {"GetMainActorRace", net_get_main_actor_race, METH_VARARGS},
                {"GetMainActorEmpire", net_get_main_actor_empire, METH_VARARGS},
                {"GetMainActorSkillGroup", net_get_main_actor_skill_group, METH_VARARGS},
                {"GetAccountCharacterSlotDataInteger", net_get_account_character_slot_data_integer, METH_VARARGS},
                {"GetAccountCharacterSlotDataString", net_get_account_character_slot_data_string, METH_VARARGS},

                {"GetFieldMusicFileName", net_get_field_music_file_name, METH_VARARGS},
                {"GetFieldMusicVolume", net_get_field_music_volume, METH_VARARGS},

                {"ToggleGameDebugInfo", net_toggle_game_debug_info, METH_VARARGS},
                {"SetLoginInfo", net_set_login_info, METH_VARARGS},
                {"SetPhaseWindow", net_set_phase_window, METH_VARARGS},
                {"ClearPhaseWindow", net_clear_phase_window, METH_VARARGS},
                {"SetServerCommandParserWindow", net_set_server_command_parser_window, METH_VARARGS},
                {"SetAccountConnectorHandler", net_set_account_connector_handler, METH_VARARGS},
                {"SetHandler", net_set_handler, METH_VARARGS},
                {"SetTCPRecvBufferSize", net_set_tcp_recv_buffer_size, METH_VARARGS},
                {"SetTCPSendBufferSize", net_set_tcp_send_buffer_size, METH_VARARGS},
                {"SetUDPRecvBufferSize", net_set_udp_recv_buffer_size, METH_VARARGS},
                {"DirectEnter", net_direct_enter, METH_VARARGS},

                {"LogOutGame", net_log_out_game, METH_VARARGS},
                {"ExitGame", net_exit_game, METH_VARARGS},
                {"ExitApplication", net_exit_application, METH_VARARGS},
                {"ConnectTCP", net_connect_tcp, METH_VARARGS},
                {"ConnectUDP", net_connect_udp, METH_VARARGS},
                {"ConnectToAccountServer", net_connect_to_account_server, METH_VARARGS},

                {"SendSelectEmpirePacket", net_send_select_empire_packet, METH_VARARGS},
                {"SendSelectCharacterPacket", net_send_select_character_packet, METH_VARARGS},
                {"SendChangeNamePacket", net_send_change_name_packet, METH_VARARGS},
                {"SendCreateCharacterPacket", net_send_create_character_packet, METH_VARARGS},
                {"SendDestroyCharacterPacket", net_send_destroy_character_packet, METH_VARARGS},
                {"SendEnterGamePacket", net_send_enter_game_packet, METH_VARARGS},

                {"SendItemUsePacket", net_send_item_use_packet, METH_VARARGS},
                {"SendItemUseToItemPacket", net_send_item_use_to_item_packet, METH_VARARGS},
                {"SendItemDropPacketNew", net_send_item_drop_packet_new, METH_VARARGS},
                {"SendGoldDropPacketNew", net_send_gold_drop_packet_new, METH_VARARGS},
                {"SendItemMovePacket", net_send_item_move_packet, METH_VARARGS},
                {"SendGiveItemPacket", net_send_give_item_packet, METH_VARARGS},

                {"Disconnect", net_disconnect, METH_VARARGS},

                {"SendChatPacket", net_send_chat_packet, METH_VARARGS},
                {"SendEmoticon", net_send_emoticon, METH_VARARGS},
                {"SendWhisperPacket", net_send_whisper_packet, METH_VARARGS},
                {"SendMobileMessagePacket", net_send_mobile_message_packet, METH_VARARGS},

                {"SendShopEndPacket", net_send_shop_end_packet, METH_VARARGS},
                {"SendShopBuyPacket", net_send_shop_buy_packet, METH_VARARGS},
                {"SendShopSellPacketNew", net_send_shop_sell_packet_new, METH_VARARGS},

                {"SendExchangeStartPacket", net_send_exchange_start_packet, METH_VARARGS},
                {"SendExchangeItemAddPacket", net_send_exchange_item_add_packet, METH_VARARGS},
                {"SendExchangeElkAddPacket", net_send_exchange_elk_add_packet, METH_VARARGS},
                {"SendExchangeAcceptPacket", net_send_exchange_accept_packet, METH_VARARGS},
                {"SendExchangeExitPacket", net_send_exchange_exit_packet, METH_VARARGS},

                {"SendOnClickPacket", net_on_click_packet, METH_VARARGS},

                {"RegisterEmoticonString", net_register_emoticon_string, METH_VARARGS},

                {"SendMessengerAddByVIDPacket", net_send_messenger_add_by_vid_packet, METH_VARARGS},
                {"SendMessengerAddByNamePacket", net_send_messenger_add_by_name_packet, METH_VARARGS},
                {"SendMessengerRemovePacket", net_send_messenger_remove_packet, METH_VARARGS},

                {"SendPartyInvitePacket", net_send_party_invite_packet, METH_VARARGS},
                {"SendPartyInviteAnswerPacket", net_send_party_invite_answer_packet, METH_VARARGS},
                {"SendPartyExitPacket", net_send_party_exit_packet, METH_VARARGS},
                {"SendPartyRemovePacket", net_send_party_remove_packet, METH_VARARGS},
                {"SendPartySetStatePacket", net_send_party_set_state_packet, METH_VARARGS},
                {"SendPartyUseSkillPacket", net_send_party_use_skill_packet, METH_VARARGS},
                {"SendPartyParameterPacket", net_send_party_parameter_packet, METH_VARARGS},

                {"SendSafeboxSaveMoneyPacket", net_send_safebox_save_money_packet, METH_VARARGS},
                {"SendSafeboxWithdrawMoneyPacket", net_send_safebox_withdraw_money_packet, METH_VARARGS},
                {"SendSafeboxCheckinPacket", net_send_safebox_checkin_packet, METH_VARARGS},
                {"SendSafeboxCheckoutPacket", net_send_safebox_checkout_packet, METH_VARARGS},
                {"SendSafeboxItemMovePacket", net_send_safebox_item_move_packet, METH_VARARGS},

                {"SendMallCheckoutPacket", net_send_mall_checkout_packet, METH_VARARGS},

                {"SendAnswerMakeGuildPacket", net_send_answer_make_guild_packet, METH_VARARGS},
                {"SendQuestInputStringPacket", net_send_quest_input_string_packet, METH_VARARGS},
                {"SendQuestConfirmPacket", net_send_quest_confirm_packet, METH_VARARGS},
                {"SendGuildAddMemberPacket", net_send_guild_add_member_packet, METH_VARARGS},
                {"SendGuildRemoveMemberPacket", net_send_guild_remove_member_packet, METH_VARARGS},
                {"SendGuildChangeGradeNamePacket", net_send_guild_change_grade_name_packet, METH_VARARGS},
                {"SendGuildChangeGradeAuthorityPacket", net_send_guild_change_grade_authority_packet, METH_VARARGS},
                {"SendGuildOfferPacket", net_send_guild_offer_packet, METH_VARARGS},
                {"SendGuildPostCommentPacket", net_sned_guild_post_comment_packet, METH_VARARGS},
                {"SendGuildDeleteCommentPacket", net_sned_guild_delete_comment_packet, METH_VARARGS},
                {"SendGuildRefreshCommentsPacket", net_send_guild_refresh_comments_packet, METH_VARARGS},
                {"SendGuildChangeMemberGradePacket", net_send_guild_change_member_grade_packet, METH_VARARGS},
                {"SendGuildUseSkillPacket", net_send_guild_use_skill_packet, METH_VARARGS},
                {"SendGuildChangeMemberGeneralPacket", net_send_guild_change_member_general_packet, METH_VARARGS},
                {"SendGuildInviteAnswerPacket", net_send_guild_invite_answer_packet, METH_VARARGS},
                {"SendGuildChargeGSPPacket", net_send_guild_charge_gsp_packet, METH_VARARGS},
                {"SendGuildDepositMoneyPacket", net_send_guild_deposit_money_packet, METH_VARARGS},
                {"SendGuildWithdrawMoneyPacket", net_send_guild_withdraw_money_packet, METH_VARARGS},

                {"SendRequestRefineInfoPacket", net_send_request_refine_info_packet, METH_VARARGS},
                {"SendRefinePacket", net_send_refine_packet, METH_VARARGS},
                {"SendSelectItemPacket", net_send_select_item_packet, METH_VARARGS},

                {"SetPacketSequenceMode", net_set_packet_sequence_mode, METH_VARARGS},
                {"SetEmpireLanguageMode", net_set_empire_language_mode, METH_VARARGS},

                {"SetSkillGroupFake", net_set_skill_group_fake, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = Py_InitModule("net", s_methods);

        PyModule_AddIntConstant(module_value, "ERROR_NONE", python_network_stream::ERROR_NONE);
        PyModule_AddIntConstant(module_value, "ERROR_CONNECT_MARK_SERVER", python_network_stream::ERROR_CONNECT_MARK_SERVER);
        PyModule_AddIntConstant(module_value, "ERROR_LOAD_MARK", python_network_stream::ERROR_LOAD_MARK);
        PyModule_AddIntConstant(module_value, "ERROR_MARK_WIDTH", python_network_stream::ERROR_MARK_WIDTH);
        PyModule_AddIntConstant(module_value, "ERROR_MARK_HEIGHT", python_network_stream::ERROR_MARK_HEIGHT);

        PyModule_AddIntConstant(module_value, "ERROR_MARK_UPLOAD_NEED_RECONNECT", python_network_stream::ERROR_MARK_UPLOAD_NEED_RECONNECT);
        PyModule_AddIntConstant(module_value, "ERROR_MARK_CHECK_NEED_RECONNECT", python_network_stream::ERROR_MARK_CHECK_NEED_RECONNECT);

        PyModule_AddIntConstant(module_value, "PHASE_WINDOW_LOGIN", python_network_stream::PHASE_WINDOW_LOGIN);
        PyModule_AddIntConstant(module_value, "PHASE_WINDOW_SELECT", python_network_stream::PHASE_WINDOW_SELECT);
        PyModule_AddIntConstant(module_value, "PHASE_WINDOW_CREATE", python_network_stream::PHASE_WINDOW_CREATE);
        PyModule_AddIntConstant(module_value, "PHASE_WINDOW_LOAD", python_network_stream::PHASE_WINDOW_LOAD);
        PyModule_AddIntConstant(module_value, "PHASE_WINDOW_GAME", python_network_stream::PHASE_WINDOW_GAME);
        PyModule_AddIntConstant(module_value, "PHASE_WINDOW_EMPIRE", python_network_stream::PHASE_WINDOW_EMPIRE);
        PyModule_AddIntConstant(module_value, "PHASE_WINDOW_LOGO", python_network_stream::PHASE_WINDOW_LOGO);

        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_ID", python_network_stream::ACCOUNT_CHARACTER_SLOT_ID);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_NAME", python_network_stream::ACCOUNT_CHARACTER_SLOT_NAME);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_RACE", python_network_stream::ACCOUNT_CHARACTER_SLOT_RACE);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_LEVEL", python_network_stream::ACCOUNT_CHARACTER_SLOT_LEVEL);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_STR", python_network_stream::ACCOUNT_CHARACTER_SLOT_STR);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_DEX", python_network_stream::ACCOUNT_CHARACTER_SLOT_DEX);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_INT", python_network_stream::ACCOUNT_CHARACTER_SLOT_INT);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_HTH", python_network_stream::ACCOUNT_CHARACTER_SLOT_HTH);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_PLAYTIME", python_network_stream::ACCOUNT_CHARACTER_SLOT_PLAYTIME);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_FORM", python_network_stream::ACCOUNT_CHARACTER_SLOT_FORM);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_ADDR", python_network_stream::ACCOUNT_CHARACTER_SLOT_ADDR);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_PORT", python_network_stream::ACCOUNT_CHARACTER_SLOT_PORT);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_GUILD_ID", python_network_stream::ACCOUNT_CHARACTER_SLOT_GUILD_ID);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_GUILD_NAME", python_network_stream::ACCOUNT_CHARACTER_SLOT_GUILD_NAME);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_CHANGE_NAME_FLAG", python_network_stream::ACCOUNT_CHARACTER_SLOT_CHANGE_NAME_FLAG);
        PyModule_AddIntConstant(module_value, "ACCOUNT_CHARACTER_SLOT_HAIR", python_network_stream::ACCOUNT_CHARACTER_SLOT_HAIR);

        PyModule_AddIntConstant(module_value, "SERVER_COMMAND_LOG_OUT", python_network_stream::SERVER_COMMAND_LOG_OUT);
        PyModule_AddIntConstant(module_value, "SERVER_COMMAND_RETURN_TO_SELECT_CHARACTER", python_network_stream::SERVER_COMMAND_RETURN_TO_SELECT_CHARACTER);
        PyModule_AddIntConstant(module_value, "SERVER_COMMAND_QUIT", python_network_stream::SERVER_COMMAND_QUIT);

        PyModule_AddIntConstant(module_value, "EMPIRE_A", 1);
        PyModule_AddIntConstant(module_value, "EMPIRE_B", 2);
        PyModule_AddIntConstant(module_value, "EMPIRE_C", 3);
        PyModule_AddIntConstant(module_value, "DS_SUB_HEADER_REFINE_FAIL", DS_SUB_HEADER_REFINE_FAIL);
        PyModule_AddIntConstant(module_value, "DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE", DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE);
        PyModule_AddIntConstant(module_value, "DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL", DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL);
        PyModule_AddIntConstant(module_value, "DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY", DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY);
        PyModule_AddIntConstant(module_value, "DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL", DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL);
        PyModule_AddIntConstant(module_value, "DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL", DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL);
        PyModule_AddIntConstant(module_value, "DS_SUB_HEADER_REFINE_SUCCEED", DS_SUB_HEADER_REFINE_SUCCEED);
    }
}
