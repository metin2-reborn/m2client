#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include "server_state_checker.h"

namespace metin2::inline client::inline protocol {

    auto server_state_checker_create(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *wnd;
        if (!PyArg_ParseTuple(args, "O", &wnd)) {
            return nullptr;
        }

        get_server_state_checker().create(wnd);
        Py_RETURN_NONE;
    }

    auto server_state_checker_update(PyObject *self, PyObject *args) -> PyObject * {
        get_server_state_checker().update();
        Py_RETURN_NONE;
    }

    auto server_state_checker_request(PyObject *self, PyObject *args) -> PyObject * {
        get_server_state_checker().request();
        Py_RETURN_NONE;
    }

    auto server_state_checker_add_channel(PyObject *self, PyObject *args) -> PyObject * {
        int server_index;
        char *addr;
        int port;

        if (!PyArg_ParseTuple(args, "isi", &server_index, &addr, &port)) {
            return nullptr;
        }

        get_server_state_checker().add_channel(server_index, addr, port);
        Py_RETURN_NONE;
    }

    // The scripts pass their handler object; the checker reports through the network stream instead.
    auto server_state_checker_initialize(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handler = nullptr;
        if (!PyArg_ParseTuple(args, "|O", &handler)) {
            return nullptr;
        }

        get_server_state_checker().initialize();
        Py_RETURN_NONE;
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
