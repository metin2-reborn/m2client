#include "python_debug_module.h"

#include <metin2/log/log.h>
#include "python_module.h"
#include "python_utils.h"

namespace metin2::inline script {

    static auto dbg_log_box(PyObject *self, PyObject *args) -> PyObject * {
        char *msg, *caption = nullptr;
        if (!PyArg_ParseTuple(args, "s|s", &msg, &caption)) {
            return nullptr;
        }

        metin2::log::error("{}", msg);
        Py_RETURN_NONE;
    }

    static auto dbg_trace(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!PyArg_ParseTuple(args, "s", &msg)) {
            return nullptr;
        }

        metin2::log::debug("{}", msg);
        Py_RETURN_NONE;
    }

    static auto dbg_tracen(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!PyArg_ParseTuple(args, "s", &msg)) {
            return nullptr;
        }

        metin2::log::debug("{}", msg);
        Py_RETURN_NONE;
    }

    static auto dbg_trace_error(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!PyArg_ParseTuple(args, "s", &msg)) {
            return nullptr;
        }

        metin2::log::error("{}", msg);
        Py_RETURN_NONE;
    }

    static auto dbg_warn(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!PyArg_ParseTuple(args, "s", &msg)) {
            return nullptr;
        }

        metin2::log::warn("{}", msg);
        Py_RETURN_NONE;
    }

    static auto dbg_register_exception_string(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!PyArg_ParseTuple(args, "s", &msg)) {
            return nullptr;
        }

        if (python_exception_sender_interface *sender = get_exception_sender())
            sender->register_exception_string(msg);

        Py_RETURN_NONE;
    }

    void init_log_module() {
        static PyMethodDef s_methods[] = {
            {"log_box", dbg_log_box, METH_VARARGS},
            {"trace", dbg_trace, METH_VARARGS},
            {"tracen", dbg_tracen, METH_VARARGS},
            {"trace_error", dbg_trace_error, METH_VARARGS},
            {"warn", dbg_warn, METH_VARARGS},
            {"register_exception_string", dbg_register_exception_string, METH_VARARGS},
            {nullptr, nullptr},
        };

        init_python_module("metin2.log", s_methods);
    }
}
