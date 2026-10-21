#include "python_messenger.h"
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline social {

    void python_messenger::remove_friend(const char *key) {
        friend_name_map_value_.erase(key);
    }

    void python_messenger::on_friend_login(const char *key) {
        friend_name_map_value_.insert(key);

        if (messenger_handler_)
            py_call_class_member_func(messenger_handler_, "OnLogin", Py_BuildValue("(is)", MESSENGER_GRUOP_INDEX_FRIEND, key));
    }

    void python_messenger::on_friend_logout(const char *key) {
        friend_name_map_value_.insert(key);

        if (messenger_handler_)
            py_call_class_member_func(messenger_handler_, "OnLogout", Py_BuildValue("(is)", MESSENGER_GRUOP_INDEX_FRIEND, key));
    }

    void python_messenger::set_mobile(const char *key, uint8_t state) {
        friend_name_map_value_.insert(key);

        if (messenger_handler_)
            py_call_class_member_func(messenger_handler_, "OnMobile", Py_BuildValue("(isi)", MESSENGER_GRUOP_INDEX_FRIEND, key, state));
    }

    auto python_messenger::is_friend_by_key(const char *key) -> bool {
        return friend_name_map_value_.end() != friend_name_map_value_.find(key);
    }

    auto python_messenger::is_friend_by_name(const char *name) -> bool {
        return is_friend_by_key(name);
    }

    void python_messenger::append_guild_member(const char *name) {
        if (guild_member_state_map_value_.end() != guild_member_state_map_value_.find(name))
            return;

        logout_guild_member(name);
    }

    void python_messenger::remove_guild_member(const char *name) {
        guild_member_state_map_value_.erase(name);

        if (messenger_handler_)
            py_call_class_member_func(messenger_handler_, "OnRemoveList", Py_BuildValue("(is)", MESSENGER_GRUOP_INDEX_GUILD, name));
    }

    void python_messenger::remove_all_guild_member() {
        guild_member_state_map_value_.clear();

        if (messenger_handler_)
            py_call_class_member_func(messenger_handler_, "OnRemoveAllList", Py_BuildValue("(i)", MESSENGER_GRUOP_INDEX_GUILD));
    }

    void python_messenger::login_guild_member(const char *name) {
        guild_member_state_map_value_[name] = 1;
        if (messenger_handler_)
            py_call_class_member_func(messenger_handler_, "OnLogin", Py_BuildValue("(is)", MESSENGER_GRUOP_INDEX_GUILD, name));
    }

    void python_messenger::logout_guild_member(const char *name) {
        guild_member_state_map_value_[name] = 0;
        if (messenger_handler_)
            py_call_class_member_func(messenger_handler_, "OnLogout", Py_BuildValue("(is)", MESSENGER_GRUOP_INDEX_GUILD, name));
    }

    void python_messenger::refresh_guild_member() {
        for (auto itor = guild_member_state_map_value_.begin(); itor != guild_member_state_map_value_.end(); ++itor) {
            if (itor->second)
                py_call_class_member_func(messenger_handler_, "OnLogin", Py_BuildValue("(is)", MESSENGER_GRUOP_INDEX_GUILD, (itor->first).c_str()));
            else
                py_call_class_member_func(messenger_handler_, "OnLogout", Py_BuildValue("(is)", MESSENGER_GRUOP_INDEX_GUILD, (itor->first).c_str()));
        }
    }

    void python_messenger::destroy() {
        friend_name_map_value_.clear();
        guild_member_state_map_value_.clear();
    }

    void python_messenger::set_messenger_handler(PyObject *handler) {
        messenger_handler_ = handler;
    }

    python_messenger::python_messenger()
        : messenger_handler_(nullptr) {
    }

    python_messenger::~python_messenger() = default;

    auto messenger_remove_friend(PyObject *self, PyObject *args) -> PyObject * {
        char *key;
        if (!py_tuple_get_string(args, 0, &key))
            return py_build_exception();

        get_python_messenger().remove_friend(key);
        return py_build_none();
    }

    auto messenger_is_friend_by_name(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        return Py_BuildValue("i", get_python_messenger().is_friend_by_name(name));
    }

    auto messenger_destroy(PyObject *self, PyObject *args) -> PyObject * {
        get_python_messenger().destroy();
        return py_build_none();
    }

    auto messenger_refresh_guild_member(PyObject *self, PyObject *args) -> PyObject * {
        get_python_messenger().refresh_guild_member();
        return py_build_none();
    }

    auto messenger_set_messenger_handler(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *event_handler;
        if (!py_tuple_get_object(args, 0, &event_handler))
            return py_build_exception();

        get_python_messenger().set_messenger_handler(event_handler);
        return py_build_none();
    }

    void init_messenger() {
        static PyMethodDef s_methods[] =
            {
                {"RemoveFriend", messenger_remove_friend, METH_VARARGS},
                {"IsFriendByName", messenger_is_friend_by_name, METH_VARARGS},
                {"Destroy", messenger_destroy, METH_VARARGS},
                {"RefreshGuildMember", messenger_refresh_guild_member, METH_VARARGS},
                {"SetMessengerHandler", messenger_set_messenger_handler, METH_VARARGS},
                {nullptr, nullptr, 0, nullptr},
        };

        Py_InitModule("messenger", s_methods);
    }
}
