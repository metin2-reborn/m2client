#pragma once

#include <cstdint>
#include <string>

#include "python_header.h"

namespace metin2::inline script {

#define SET_EXCEPTION(x) PyErr_SetString(PyExc_RuntimeError, #x)

    auto py_tuple_get_string(PyObject *args, int pos, char **ret) -> bool;
    auto py_tuple_get_integer(PyObject *args, int pos, uint8_t *ret) -> bool;
    auto py_tuple_get_integer(PyObject *args, int pos, int *ret) -> bool;
    auto py_tuple_get_integer(PyObject *args, int pos, uint16_t *ret) -> bool;
    auto py_tuple_get_integer(PyObject *args, int pos, intptr_t *ret) -> bool; // pointer-sized handles
    auto py_tuple_get_byte(PyObject *args, int pos, uint8_t *ret) -> bool;
    auto py_tuple_get_float(PyObject *args, int pos, float *ret) -> bool;
    auto py_tuple_get_object(PyObject *args, int pos, PyObject **ret) -> bool;
    auto py_tuple_get_boolean(PyObject *args, int pos, bool *ret) -> bool;

    // Every call consumes poArgs (a new reference from Py_BuildValue), whether or not it succeeds.
    auto py_call_class_member_func(PyObject *class_value, const char *func, PyObject *args) -> bool;
    auto py_call_class_member_func(PyObject *class_value, const char *func, PyObject *args, bool *pis_ret) -> bool;
    auto py_call_class_member_func(PyObject *class_value, const char *func, PyObject *args, long *pl_ret_value) -> bool;
    auto py_call_class_member_func_by_py_string(PyObject *class_value, PyObject *func_name, PyObject *args) -> bool;

    auto py_build_exception(const char *err = nullptr, ...) -> PyObject *;
    auto py_bad_argument() -> PyObject *;
    auto py_build_none() -> PyObject *;

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
