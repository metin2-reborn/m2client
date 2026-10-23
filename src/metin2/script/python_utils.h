#pragma once

#include "python_header.h"

#include <string>

namespace metin2::inline script {
    // PyArg_ParseTuple "O&" converter for the pointer-sized integer handles the modules hand to the
    // scripts (windows, image and text instances). A zero handle raises instead of being
    // dereferenced later.
    template <class T>
    auto py_handle_converter(PyObject *obj, void *out) -> int {
        if (PyFloat_Check(obj)) {
            PyErr_SetString(PyExc_TypeError, "handle must be an integer");
            return 0;
        }

        const Py_ssize_t handle = PyInt_AsSsize_t(obj);
        if (handle == -1 && PyErr_Occurred()) {
            return 0;
        }

        if (handle == 0) {
            PyErr_SetString(PyExc_ValueError, "null handle");
            return 0;
        }

        *static_cast<T **>(out) = reinterpret_cast<T *>(handle);
        return 1;
    }

    // Every call consumes poArgs (a new reference from Py_BuildValue), whether or not it succeeds.
    auto py_call_class_member_func(PyObject *class_value, const char *func, PyObject *args) -> bool;
    auto py_call_class_member_func(PyObject *class_value, const char *func, PyObject *args, bool *pis_ret) -> bool;
    auto py_call_class_member_func(PyObject *class_value, const char *func, PyObject *args, long *pl_ret_value) -> bool;
    auto py_call_class_member_func_by_py_string(PyObject *class_value, PyObject *func_name, PyObject *args) -> bool;

    // Collects the text of a Python exception while it is printed and sends it on (to the server).
    class python_exception_sender_interface {
      public:
        virtual ~python_exception_sender_interface() = default;

        void clear() { exception_string_ = ""; }
        void register_exception_string(const char *string) { exception_string_ += string; }

        virtual void send() = 0;

      protected:
        std::string exception_string_;
    };

    void set_exception_sender(python_exception_sender_interface *exception_sender_value);
    auto get_exception_sender() -> python_exception_sender_interface *;
}
