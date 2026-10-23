#include <metin2/client/inventory/python_item.h>
#include <metin2/core/parse.h>
#include <metin2/core/utils.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/items/item_manager.h>
#include "python_chat.h"

#include <string>
#include <string_view>
#include <vector>

namespace metin2::inline client::inline social {

    auto chat_set_chat_color(PyObject *self, PyObject *args) -> PyObject * {
        int type, r, g, b;
        if (!PyArg_ParseTuple(args, "iiii", &type, &r, &g, &b)) {
            return nullptr;
        }

        get_python_chat().set_chat_color(type, r, g, b);
        Py_RETURN_NONE;
    }

    auto chat_clear(PyObject *self, PyObject *args) -> PyObject * {
        get_python_chat().destroy();
        Py_RETURN_NONE;
    }

    auto chat_close(PyObject *self, PyObject *args) -> PyObject * {
        get_python_chat().close();
        Py_RETURN_NONE;
    }

    auto chat_create_chat_set(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!PyArg_ParseTuple(args, "i", &id)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_chat().create_chat_set(id));
    }

    auto chat_update(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!PyArg_ParseTuple(args, "i", &id)) {
            return nullptr;
        }

        get_python_chat().update(id);
        Py_RETURN_NONE;
    }

    auto chat_render(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!PyArg_ParseTuple(args, "i", &id)) {
            return nullptr;
        }

        get_python_chat().render(id);
        Py_RETURN_NONE;
    }

    auto chat_set_board_state(PyObject *self, PyObject *args) -> PyObject * {
        int id, state;
        if (!PyArg_ParseTuple(args, "ii", &id, &state)) {
            return nullptr;
        }

        get_python_chat().set_board_state(id, state);

        Py_RETURN_NONE;
    }

    auto chat_set_position(PyObject *self, PyObject *args) -> PyObject * {
        int id, ix, iy;
        if (!PyArg_ParseTuple(args, "iii", &id, &ix, &iy)) {
            return nullptr;
        }

        get_python_chat().set_position(id, ix, iy);

        Py_RETURN_NONE;
    }

    auto chat_set_height(PyObject *self, PyObject *args) -> PyObject * {
        int id, height;
        if (!PyArg_ParseTuple(args, "ii", &id, &height)) {
            return nullptr;
        }

        get_python_chat().set_height(id, height);

        Py_RETURN_NONE;
    }

    auto chat_toggle_chat_mode(PyObject *self, PyObject *args) -> PyObject * {
        int id, type;
        if (!PyArg_ParseTuple(args, "ii", &id, &type)) {
            return nullptr;
        }

        get_python_chat().toggle_chat_mode(id, type);

        Py_RETURN_NONE;
    }

    auto chat_enable_chat_mode(PyObject *self, PyObject *args) -> PyObject * {
        int id, type;
        if (!PyArg_ParseTuple(args, "ii", &id, &type)) {
            return nullptr;
        }

        get_python_chat().enable_chat_mode(id, type);

        Py_RETURN_NONE;
    }

    auto chat_disable_chat_mode(PyObject *self, PyObject *args) -> PyObject * {
        int id, type;
        if (!PyArg_ParseTuple(args, "ii", &id, &type)) {
            return nullptr;
        }

        get_python_chat().disable_chat_mode(id, type);

        Py_RETURN_NONE;
    }

    auto chat_set_end_pos(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        float pos;

        if (!PyArg_ParseTuple(args, "if", &id, &pos)) {
            return nullptr;
        }

        get_python_chat().set_end_pos(id, pos);

        Py_RETURN_NONE;
    }

    auto chat_get_line_count(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!PyArg_ParseTuple(args, "i", &id)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_chat().line_count(id));
    }

    auto chat_get_visible_line_count(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!PyArg_ParseTuple(args, "i", &id)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_chat().visible_line_count(id));
    }

    auto chat_get_line_step(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!PyArg_ParseTuple(args, "i", &id)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_chat().line_step(id));
    }

    auto chat_append_chat(PyObject *self, PyObject *args) -> PyObject * {
        int type;
        char *chat;

        if (!PyArg_ParseTuple(args, "is", &type, &chat)) {
            return nullptr;
        }

        get_python_chat().append_chat(type, chat);

        Py_RETURN_NONE;
    }

    auto chat_append_chat_with_delay(PyObject *self, PyObject *args) -> PyObject * {
        int type;
        char *chat;
        int delay;

        if (!PyArg_ParseTuple(args, "isi", &type, &chat, &delay)) {
            return nullptr;
        }

        get_python_chat().append_chat_with_delay(type, chat, delay);

        Py_RETURN_NONE;
    }

    auto chat_arrange_showing_chat(PyObject *self, PyObject *args) -> PyObject * {
        int id;
        if (!PyArg_ParseTuple(args, "i", &id)) {
            return nullptr;
        }

        get_python_chat().arrange_showing_chat(id);

        Py_RETURN_NONE;
    }

    auto chat_create_whisper(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!PyArg_ParseTuple(args, "s", &name)) {
            return nullptr;
        }

        get_python_chat().create_whisper(name);

        Py_RETURN_NONE;
    }

    auto chat_append_whisper(PyObject *self, PyObject *args) -> PyObject * {
        int type;
        char *name, *chat;

        if (!PyArg_ParseTuple(args, "iss", &type, &name, &chat)) {
            return nullptr;
        }

        get_python_chat().append_whisper(type, name, chat);
        Py_RETURN_NONE;
    }

    auto chat_render_whisper(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        float fx, fy;

        if (!PyArg_ParseTuple(args, "sff", &name, &fx, &fy)) {
            return nullptr;
        }

        whisper *whisper_value;
        if (get_python_chat().get_whisper(name, &whisper_value)) {
            whisper_value->render(fx, fy);
        }

        Py_RETURN_NONE;
    }

    auto chat_set_whisper_box_size(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        float width_value, height;

        if (!PyArg_ParseTuple(args, "sff", &name, &width_value, &height)) {
            return nullptr;
        }

        whisper *whisper_value;
        if (get_python_chat().get_whisper(name, &whisper_value)) {
            whisper_value->set_box_size(width_value, height);
        }

        Py_RETURN_NONE;
    }

    auto chat_set_whisper_position(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        float position;

        if (!PyArg_ParseTuple(args, "sf", &name, &position)) {
            return nullptr;
        }

        whisper *whisper_value;
        if (get_python_chat().get_whisper(name, &whisper_value)) {
            whisper_value->set_position(position);
        }

        Py_RETURN_NONE;
    }

    auto chat_clear_whisper(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!PyArg_ParseTuple(args, "s", &name)) {
            return nullptr;
        }

        get_python_chat().clear_whisper(name);

        Py_RETURN_NONE;
    }

    auto chat_init_whisper(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *interface_value;
        if (!PyArg_ParseTuple(args, "O", &interface_value)) {
            return nullptr;
        }

        get_python_chat().init_whisper(interface_value);
        Py_RETURN_NONE;
    }

    auto chat_get_link_from_hyperlink(PyObject *self, PyObject *args) -> PyObject * {
        char *sz_hyperlink;
        if (!PyArg_ParseTuple(args, "s", &sz_hyperlink)) {
            return nullptr;
        }

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

    void init_social_chat_module() {
        static PyMethodDef s_methods[] =
            {
                {"set_chat_color", chat_set_chat_color, METH_VARARGS},
                {"clear", chat_clear, METH_VARARGS},
                {"close", chat_close, METH_VARARGS},

                {"create_chat_set", chat_create_chat_set, METH_VARARGS},
                {"update", chat_update, METH_VARARGS},
                {"render", chat_render, METH_VARARGS},

                {"set_board_state", chat_set_board_state, METH_VARARGS},
                {"set_position", chat_set_position, METH_VARARGS},
                {"set_height", chat_set_height, METH_VARARGS},
                {"toggle_chat_mode", chat_toggle_chat_mode, METH_VARARGS},
                {"enable_chat_mode", chat_enable_chat_mode, METH_VARARGS},
                {"disable_chat_mode", chat_disable_chat_mode, METH_VARARGS},
                {"set_end_pos", chat_set_end_pos, METH_VARARGS},

                {"get_line_count", chat_get_line_count, METH_VARARGS},
                {"get_visible_line_count", chat_get_visible_line_count, METH_VARARGS},
                {"get_line_step", chat_get_line_step, METH_VARARGS},

                {"append_chat", chat_append_chat, METH_VARARGS},
                {"append_chat_with_delay", chat_append_chat_with_delay, METH_VARARGS},
                {"arrange_showing_chat", chat_arrange_showing_chat, METH_VARARGS},

                {"create_whisper", chat_create_whisper, METH_VARARGS},
                {"append_whisper", chat_append_whisper, METH_VARARGS},
                {"render_whisper", chat_render_whisper, METH_VARARGS},
                {"set_whisper_box_size", chat_set_whisper_box_size, METH_VARARGS},
                {"set_whisper_position", chat_set_whisper_position, METH_VARARGS},
                {"clear_whisper", chat_clear_whisper, METH_VARARGS},
                {"init_whisper", chat_init_whisper, METH_VARARGS},

                {"get_link_from_hyperlink", chat_get_link_from_hyperlink, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module = init_python_module("metin2.social.chat", s_methods);

        PyModule_AddIntConstant(module, "CHAT_TYPE_TALKING", CHAT_TYPE_TALKING);
        PyModule_AddIntConstant(module, "CHAT_TYPE_INFO", CHAT_TYPE_INFO);
        PyModule_AddIntConstant(module, "CHAT_TYPE_NOTICE", CHAT_TYPE_NOTICE);
        PyModule_AddIntConstant(module, "CHAT_TYPE_PARTY", CHAT_TYPE_PARTY);
        PyModule_AddIntConstant(module, "CHAT_TYPE_GUILD", CHAT_TYPE_GUILD);
        PyModule_AddIntConstant(module, "CHAT_TYPE_COMMAND", CHAT_TYPE_COMMAND);
        PyModule_AddIntConstant(module, "CHAT_TYPE_SHOUT", CHAT_TYPE_SHOUT);
        PyModule_AddIntConstant(module, "CHAT_TYPE_WHISPER", CHAT_TYPE_WHISPER);
        PyModule_AddIntConstant(module, "WHISPER_TYPE_CHAT", python_chat::WHISPER_TYPE_CHAT);
        PyModule_AddIntConstant(module, "WHISPER_TYPE_SYSTEM", python_chat::WHISPER_TYPE_SYSTEM);
        PyModule_AddIntConstant(module, "WHISPER_TYPE_GM", python_chat::WHISPER_TYPE_GM);

        PyModule_AddIntConstant(module, "BOARD_STATE_VIEW", python_chat::BOARD_STATE_VIEW);
        PyModule_AddIntConstant(module, "BOARD_STATE_EDIT", python_chat::BOARD_STATE_EDIT);
        PyModule_AddIntConstant(module, "BOARD_STATE_LOG", python_chat::BOARD_STATE_LOG);

        PyModule_AddIntConstant(module, "CHAT_SET_CHAT_WINDOW", 0);
        PyModule_AddIntConstant(module, "CHAT_SET_LOG_WINDOW", 1);
    }
}
