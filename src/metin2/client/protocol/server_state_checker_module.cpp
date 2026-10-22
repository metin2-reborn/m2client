#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include "server_state_checker.h"

namespace metin2::inline client::inline protocol {

    auto server_state_checker_create(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *wnd;
        if (!py_tuple_get_object(args, 0, &wnd))
            return py_bad_argument();

        get_server_state_checker().create(wnd);
        return py_build_none();
    }

    auto server_state_checker_update(PyObject *self, PyObject *args) -> PyObject * {
        get_server_state_checker().update();
        return py_build_none();
    }

    auto server_state_checker_request(PyObject *self, PyObject *args) -> PyObject * {
        get_server_state_checker().request();
        return py_build_none();
    }

    auto server_state_checker_add_channel(PyObject *self, PyObject *args) -> PyObject * {
        int server_index;
        if (!py_tuple_get_integer(args, 0, &server_index))
            return py_build_exception();

        char *addr;
        if (!py_tuple_get_string(args, 1, &addr))
            return py_build_exception();

        int port;
        if (!py_tuple_get_integer(args, 2, &port))
            return py_build_exception();

        get_server_state_checker().add_channel(server_index, addr, port);
        return py_build_none();
    }

    auto server_state_checker_initialize(PyObject *self, PyObject *args) -> PyObject * {
        get_server_state_checker().initialize();
        return py_build_none();
    }

    void init_net_server_state_module() {
        static PyMethodDef s_methods[] =
            {
                {"Create", server_state_checker_create, METH_VARARGS},
                {"Update", server_state_checker_update, METH_VARARGS},
                {"Request", server_state_checker_request, METH_VARARGS},
                {"AddChannel", server_state_checker_add_channel, METH_VARARGS},
                {"Initialize", server_state_checker_initialize, METH_VARARGS},
                {nullptr, nullptr, 0, nullptr},
        };

        init_python_module("metin2.net.server_state", s_methods);
    }
}
