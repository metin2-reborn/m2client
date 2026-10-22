#include "python_debug_module.h"

#include <metin2/log/log.h>
#include "python_module.h"
#include "python_utils.h"

namespace metin2::inline script {

    static auto dbg_log_box(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        char *caption;
        if (!py_tuple_get_string(args, 0, &msg))
            return py_build_exception();
        if (!py_tuple_get_string(args, 1, &caption))
            metin2::log::error("{}", msg);
        else
            metin2::log::error("{}", msg);
        return py_build_none();
    }

    static auto dbg_trace(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!py_tuple_get_string(args, 0, &msg))
            return py_build_exception();

        metin2::log::debug("{}", msg);
        return py_build_none();
    }

    static auto dbg_tracen(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!py_tuple_get_string(args, 0, &msg))
            return py_build_exception();

        metin2::log::debug("{}", msg);
        return py_build_none();
    }

    static auto dbg_trace_error(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!py_tuple_get_string(args, 0, &msg))
            return py_build_exception();

        metin2::log::error("{}", msg);
        return py_build_none();
    }

    static auto dbg_register_exception_string(PyObject *self, PyObject *args) -> PyObject * {
        char *msg;
        if (!py_tuple_get_string(args, 0, &msg))
            return py_build_exception();

        if (python_exception_sender_interface *sender = get_exception_sender())
            sender->register_exception_string(msg);

        return py_build_none();
    }

    void init_log_module() {
        static PyMethodDef s_methods[] = {
            {"LogBox", dbg_log_box, METH_VARARGS},
            {"Trace", dbg_trace, METH_VARARGS},
            {"Tracen", dbg_tracen, METH_VARARGS},
            {"TraceError", dbg_trace_error, METH_VARARGS},
            {"RegisterExceptionString", dbg_register_exception_string, METH_VARARGS},
            {nullptr, nullptr},
        };

        init_python_module("metin2.log", s_methods);
    }
}
