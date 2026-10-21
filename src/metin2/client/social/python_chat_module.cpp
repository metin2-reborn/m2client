#include <metin2/client/inventory/python_item.h>
#include <metin2/core/parse.h>
#include <metin2/core/utils.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/items/item_manager.h>
#include "python_chat.h"

#include <string>
#include <string_view>
#include <vector>

namespace metin2::inline client::inline social {

    auto chat_set_chat_color(PyObject *self, PyObject *args) -> PyObject * {
        int type;
        if (!py_tuple_get_integer(args, 0, &type))
            return py_build_exception();

        int r;
        if (!py_tuple_get_integer(args, 1, &r))
            return py_build_exception();

        int g;
        if (!py_tuple_get_integer(args, 2, &g))
            return py_build_exception();

        int b;
        if (!py_tuple_get_integer(args, 3, &b))
            return py_build_exception();

        get_python_chat().set_chat_color(type, r, g, b);
        return py_build_none();
    }

    auto chat_clear(PyObject *self, PyObject *args) -> PyObject * {
        get_python_chat().destroy();
        return py_build_none();
    }

    auto chat_close(PyObject *self, PyObject *args) -> PyObject * {
        get_python_chat().close();
        return py_build_none();
    }

    auto chat_create_chat_set(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();

        return Py_BuildValue("i", get_python_chat().create_chat_set(id));
    }

    auto chat_update(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();

        get_python_chat().update(id);
        return py_build_none();
    }

    auto chat_render(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();

        get_python_chat().render(id);
        return py_build_none();
    }

    auto chat_set_board_state(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();
        int state;
        if (!py_tuple_get_integer(args, 1, &state))
            return py_build_exception();

        get_python_chat().set_board_state(id, state);

        return py_build_none();
    }

    auto chat_set_position(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();
        int ix;
        if (!py_tuple_get_integer(args, 1, &ix))
            return py_build_exception();
        int iy;
        if (!py_tuple_get_integer(args, 2, &iy))
            return py_build_exception();

        get_python_chat().set_position(id, ix, iy);

        return py_build_none();
    }

    auto chat_set_height(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();
        int height;
        if (!py_tuple_get_integer(args, 1, &height))
            return py_build_exception();

        get_python_chat().set_height(id, height);

        return py_build_none();
    }

    auto chat_toggle_chat_mode(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();
        int type;
        if (!py_tuple_get_integer(args, 1, &type))
            return py_build_exception();

        get_python_chat().toggle_chat_mode(id, type);

        return py_build_none();
    }

    auto chat_enable_chat_mode(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();
        int type;
        if (!py_tuple_get_integer(args, 1, &type))
            return py_build_exception();

        get_python_chat().enable_chat_mode(id, type);

        return py_build_none();
    }

    auto chat_disable_chat_mode(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();
        int type;
        if (!py_tuple_get_integer(args, 1, &type))
            return py_build_exception();

        get_python_chat().disable_chat_mode(id, type);

        return py_build_none();
    }

    auto chat_set_end_pos(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();
        float pos;
        if (!py_tuple_get_float(args, 1, &pos))
            return py_build_exception();

        get_python_chat().set_end_pos(id, pos);

        return py_build_none();
    }

    auto chat_get_line_count(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();

        return Py_BuildValue("i", get_python_chat().line_count(id));
    }

    auto chat_get_visible_line_count(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();

        return Py_BuildValue("i", get_python_chat().visible_line_count(id));
    }

    auto chat_get_line_step(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();

        return Py_BuildValue("i", get_python_chat().line_step(id));
    }

    auto chat_append_chat(PyObject *self, PyObject *args) -> PyObject * {
        int type;
        if (!py_tuple_get_integer(args, 0, &type))
            return py_build_exception();

        char *chat;
        if (!py_tuple_get_string(args, 1, &chat))
            return py_build_exception();

        get_python_chat().append_chat(type, chat);

        return py_build_none();
    }

    auto chat_append_chat_with_delay(PyObject *self, PyObject *args) -> PyObject * {
        int type;
        if (!py_tuple_get_integer(args, 0, &type))
            return py_build_exception();

        char *chat;
        if (!py_tuple_get_string(args, 1, &chat))
            return py_build_exception();

        int delay;
        if (!py_tuple_get_integer(args, 2, &delay))
            return py_build_exception();

        get_python_chat().append_chat_with_delay(type, chat, delay);

        return py_build_none();
    }

    auto chat_arrange_showing_chat(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!py_tuple_get_integer(args, 0, &id))
            return py_build_exception();

        get_python_chat().arrange_showing_chat(id);

        return py_build_none();
    }

    auto chat_create_whisper(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        get_python_chat().create_whisper(name);

        return py_build_none();
    }

    auto chat_append_whisper(PyObject *self, PyObject *args) -> PyObject * {
        int type;
        if (!py_tuple_get_integer(args, 0, &type))
            return py_build_exception();

        char *name;
        if (!py_tuple_get_string(args, 1, &name))
            return py_build_exception();

        char *chat;
        if (!py_tuple_get_string(args, 2, &chat))
            return py_build_exception();

        get_python_chat().append_whisper(type, name, chat);
        return py_build_none();
    }

    auto chat_render_whisper(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        float fx;
        if (!py_tuple_get_float(args, 1, &fx))
            return py_build_exception();

        float fy;
        if (!py_tuple_get_float(args, 2, &fy))
            return py_build_exception();

        whisper *whisper_value;
        if (get_python_chat().get_whisper(name, &whisper_value)) {
            whisper_value->render(fx, fy);
        }

        return py_build_none();
    }

    auto chat_set_whisper_box_size(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        float width_value;
        if (!py_tuple_get_float(args, 1, &width_value))
            return py_build_exception();

        float height;
        if (!py_tuple_get_float(args, 2, &height))
            return py_build_exception();

        whisper *whisper_value;
        if (get_python_chat().get_whisper(name, &whisper_value)) {
            whisper_value->set_box_size(width_value, height);
        }

        return py_build_none();
    }

    auto chat_set_whisper_position(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        float position;
        if (!py_tuple_get_float(args, 1, &position))
            return py_build_exception();

        whisper *whisper_value;
        if (get_python_chat().get_whisper(name, &whisper_value)) {
            whisper_value->set_position(position);
        }

        return py_build_none();
    }

    auto chat_clear_whisper(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        get_python_chat().clear_whisper(name);

        return py_build_none();
    }

    auto chat_init_whisper(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *interface_value;
        if (!py_tuple_get_object(args, 0, &interface_value))
            return py_build_exception();

        get_python_chat().init_whisper(interface_value);
        return py_build_none();
    }

    auto chat_get_link_from_hyperlink(PyObject *self, PyObject *args) -> PyObject * {
        char *sz_hyperlink;

        if (!py_tuple_get_string(args, 0, &sz_hyperlink))
            return py_build_exception();

        // ':'-separated fields, empty ones dropped
        std::vector<std::string> results;
        std::string_view st_hyperlink(sz_hyperlink);
        while (!st_hyperlink.empty()) {
            std::string_view::size_type colon = st_hyperlink.find(':');
            std::string_view field = st_hyperlink.substr(0, colon);
            if (!field.empty())
                results.emplace_back(field);
            st_hyperlink = colon == std::string_view::npos ? std::string_view() : st_hyperlink.substr(colon + 1);
        }

        // item:vnum:flags:socket0:socket1:socket2, optionally followed by attribute type:value pairs
        if (!results.empty() && results[0] == "item") {
            if (results.size() < 6)
                return Py_BuildValue("s", "");

            item_data *item_data = nullptr;

            if (get_item_manager().item_data_pointer(htoi(results[1].c_str()), &item_data)) {
                char buf[1024] = {0};
                char itemlink[256];
                int len;
                bool is_attr = false;

                len = snprintf(itemlink, sizeof(itemlink), "item:%x:%x:%x:%x:%x",
                    htoi(results[1].c_str()),
                    htoi(results[2].c_str()),
                    htoi(results[3].c_str()),
                    htoi(results[4].c_str()),
                    htoi(results[5].c_str()));

                if (results.size() >= 8) {
                    for (size_t i = 6; i < results.size(); i += 2) {
                        len += snprintf(itemlink + len, sizeof(itemlink) - len, ":%x:%d",
                            htoi(results[i].c_str()),
                            metin2::to_number<int>(results[i + 1].c_str()));
                        is_attr = true;
                    }
                }

                if (is_attr)
                    snprintf(buf, sizeof(buf), "|cffffc700|H%s|h[%s]|h|r", itemlink, item_data->get_name());
                else
                    snprintf(buf, sizeof(buf), "|cfff1e6c0|H%s|h[%s]|h|r", itemlink, item_data->get_name());

                return Py_BuildValue("s", buf);
            }
        }

        return Py_BuildValue("s", "");
    }

    void init_chat() {
        static PyMethodDef s_methods[] =
            {
                {"SetChatColor", chat_set_chat_color, METH_VARARGS},
                {"Clear", chat_clear, METH_VARARGS},
                {"Close", chat_close, METH_VARARGS},

                {"CreateChatSet", chat_create_chat_set, METH_VARARGS},
                {"Update", chat_update, METH_VARARGS},
                {"Render", chat_render, METH_VARARGS},

                {"SetBoardState", chat_set_board_state, METH_VARARGS},
                {"SetPosition", chat_set_position, METH_VARARGS},
                {"SetHeight", chat_set_height, METH_VARARGS},
                {"ToggleChatMode", chat_toggle_chat_mode, METH_VARARGS},
                {"EnableChatMode", chat_enable_chat_mode, METH_VARARGS},
                {"DisableChatMode", chat_disable_chat_mode, METH_VARARGS},
                {"SetEndPos", chat_set_end_pos, METH_VARARGS},

                {"GetLineCount", chat_get_line_count, METH_VARARGS},
                {"GetVisibleLineCount", chat_get_visible_line_count, METH_VARARGS},
                {"GetLineStep", chat_get_line_step, METH_VARARGS},

                {"AppendChat", chat_append_chat, METH_VARARGS},
                {"AppendChatWithDelay", chat_append_chat_with_delay, METH_VARARGS},
                {"ArrangeShowingChat", chat_arrange_showing_chat, METH_VARARGS},

                {"CreateWhisper", chat_create_whisper, METH_VARARGS},
                {"AppendWhisper", chat_append_whisper, METH_VARARGS},
                {"RenderWhisper", chat_render_whisper, METH_VARARGS},
                {"SetWhisperBoxSize", chat_set_whisper_box_size, METH_VARARGS},
                {"SetWhisperPosition", chat_set_whisper_position, METH_VARARGS},
                {"ClearWhisper", chat_clear_whisper, METH_VARARGS},
                {"InitWhisper", chat_init_whisper, METH_VARARGS},

                {"GetLinkFromHyperlink", chat_get_link_from_hyperlink, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = Py_InitModule("chat", s_methods);

        PyModule_AddIntConstant(module_value, "CHAT_TYPE_TALKING", CHAT_TYPE_TALKING);
        PyModule_AddIntConstant(module_value, "CHAT_TYPE_INFO", CHAT_TYPE_INFO);
        PyModule_AddIntConstant(module_value, "CHAT_TYPE_NOTICE", CHAT_TYPE_NOTICE);
        PyModule_AddIntConstant(module_value, "CHAT_TYPE_PARTY", CHAT_TYPE_PARTY);
        PyModule_AddIntConstant(module_value, "CHAT_TYPE_GUILD", CHAT_TYPE_GUILD);
        PyModule_AddIntConstant(module_value, "CHAT_TYPE_COMMAND", CHAT_TYPE_COMMAND);
        PyModule_AddIntConstant(module_value, "CHAT_TYPE_SHOUT", CHAT_TYPE_SHOUT);
        PyModule_AddIntConstant(module_value, "CHAT_TYPE_WHISPER", CHAT_TYPE_WHISPER);
        PyModule_AddIntConstant(module_value, "WHISPER_TYPE_CHAT", python_chat::WHISPER_TYPE_CHAT);
        PyModule_AddIntConstant(module_value, "WHISPER_TYPE_SYSTEM", python_chat::WHISPER_TYPE_SYSTEM);
        PyModule_AddIntConstant(module_value, "WHISPER_TYPE_GM", python_chat::WHISPER_TYPE_GM);

        PyModule_AddIntConstant(module_value, "BOARD_STATE_VIEW", python_chat::BOARD_STATE_VIEW);
        PyModule_AddIntConstant(module_value, "BOARD_STATE_EDIT", python_chat::BOARD_STATE_EDIT);
        PyModule_AddIntConstant(module_value, "BOARD_STATE_LOG", python_chat::BOARD_STATE_LOG);

        PyModule_AddIntConstant(module_value, "CHAT_SET_CHAT_WINDOW", 0);
        PyModule_AddIntConstant(module_value, "CHAT_SET_LOG_WINDOW", 1);
    }
}
