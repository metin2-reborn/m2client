#include "python_utils.h"

#include <cstdarg>
#include <cstdio>

namespace metin2::inline script {

    static python_exception_sender_interface *exception_sender = nullptr;

    void set_exception_sender(python_exception_sender_interface *exception_sender_value) {
        exception_sender = exception_sender_value;
    }

    auto get_exception_sender() -> python_exception_sender_interface * {
        return exception_sender;
    }

    auto py_bad_argument() -> PyObject * {
        PyErr_BadArgument();
        return nullptr;
    }

    auto py_build_exception(const char *err, ...) -> PyObject * {
        if (!err)
            PyErr_Clear();
        else {
            char err_buf[512 + 1];
            va_list args;
            va_start(args, err);
            vsnprintf(err_buf, sizeof(err_buf), err, args);
            va_end(args);

            PyErr_SetString(PyExc_RuntimeError, err_buf);
        }

        return py_build_none();
    }

    auto py_build_none() -> PyObject * {
        Py_INCREF(Py_None);
        return Py_None;
    }

    static auto get_tuple_item(PyObject *args, int pos) -> PyObject * {
        if (pos >= PyTuple_Size(args))
            return nullptr;

        return PyTuple_GetItem(args, pos);
    }

    auto py_tuple_get_object(PyObject *args, int pos, PyObject **ret) -> bool {
        PyObject *item = get_tuple_item(args, pos);
        if (!item)
            return false;

        *ret = item;
        return true;
    }

    auto py_tuple_get_float(PyObject *args, int pos, float *ret) -> bool {
        PyObject *item = get_tuple_item(args, pos);
        if (!item)
            return false;

        *ret = float(PyFloat_AsDouble(item));
        return true;
    }

    auto py_tuple_get_byte(PyObject *args, int pos, uint8_t *ret) -> bool {
        int val = 0;
        bool result = py_tuple_get_integer(args, pos, &val);
        *ret = uint8_t(val);
        return result;
    }

    auto py_tuple_get_integer(PyObject *args, int pos, uint8_t *ret) -> bool {
        return py_tuple_get_byte(args, pos, ret);
    }

    auto py_tuple_get_integer(PyObject *args, int pos, uint16_t *ret) -> bool {
        int val = 0;
        bool result = py_tuple_get_integer(args, pos, &val);
        *ret = uint16_t(val);
        return result;
    }

    auto py_tuple_get_integer(PyObject *args, int pos, int *ret) -> bool {
        PyObject *item = get_tuple_item(args, pos);
        if (!item)
            return false;

        *ret = int(PyLong_AsLong(item));
        return true;
    }

    auto py_tuple_get_integer(PyObject *args, int pos, intptr_t *ret) -> bool {
        PyObject *item = get_tuple_item(args, pos);
        if (!item)
            return false;

        *ret = intptr_t(PyInt_AsSsize_t(item));
        return true;
    }

    auto py_tuple_get_string(PyObject *args, int pos, char **ret) -> bool {
        PyObject *item = get_tuple_item(args, pos);
        if (!item)
            return false;

        if (!PyString_Check(item))
            return false;

        *ret = PyString_AsString(item);
        return true;
    }

    auto py_tuple_get_boolean(PyObject *args, int pos, bool *ret) -> bool {
        PyObject *item = get_tuple_item(args, pos);
        if (!item)
            return false;

        *ret = PyLong_AsLong(item) != 0;
        return true;
    }

    // Calls poFunc (a new reference, released here) with poArgs (released here) and hands back the
    // result as a new reference. A failed call prints the Python traceback and forwards it to the
    // registered exception sender.
    static auto call_member(PyObject *func, PyObject *args, PyObject **ppo_ret) -> bool {
        if (!func) {
            PyErr_Clear();
            Py_XDECREF(args);
            return false;
        }

        if (!PyCallable_Check(func)) {
            Py_DECREF(func);
            Py_XDECREF(args);
            return false;
        }

        PyObject *ret = PyObject_CallObject(func, args); // New reference

        if (!ret) {
            if (exception_sender)
                exception_sender->clear();

            PyErr_Print();

            if (exception_sender)
                exception_sender->send();

            Py_DECREF(func);
            Py_XDECREF(args);
            return false;
        }

        *ppo_ret = ret;

        Py_DECREF(func);
        Py_XDECREF(args);
        return true;
    }

    static auto call_member_by_name(PyObject *class_value, const char *func, PyObject *args, PyObject **ppo_ret) -> bool {
        if (!class_value) {
            Py_XDECREF(args);
            return false;
        }

        return call_member(PyObject_GetAttrString(class_value, func), args, ppo_ret); // New reference
    }

    auto py_call_class_member_func(PyObject *class_value, const char *func, PyObject *args) -> bool {
        PyObject *ret;

        if (!call_member_by_name(class_value, func, args, &ret))
            return false;

        Py_DECREF(ret);
        return true;
    }

    auto py_call_class_member_func_by_py_string(PyObject *class_value, PyObject *func_name, PyObject *args) -> bool {
        PyObject *ret;

        if (!class_value) {
            Py_XDECREF(args);
            return false;
        }

        if (!call_member(PyObject_GetAttr(class_value, func_name), args, &ret)) // New reference
            return false;

        Py_DECREF(ret);
        return true;
    }

    auto py_call_class_member_func(PyObject *class_value, const char *func, PyObject *args, bool *pis_ret) -> bool {
        PyObject *ret;

        if (!call_member_by_name(class_value, func, args, &ret))
            return false;

        if (PyNumber_Check(ret))
            *pis_ret = (PyLong_AsLong(ret) != 0);
        else
            *pis_ret = true;

        Py_DECREF(ret);
        return true;
    }

    auto py_call_class_member_func(PyObject *class_value, const char *func, PyObject *args, long *pl_ret_value) -> bool {
        PyObject *ret;

        if (!call_member_by_name(class_value, func, args, &ret))
            return false;

        bool is_number = PyNumber_Check(ret);
        if (is_number)
            *pl_ret_value = PyLong_AsLong(ret);

        Py_DECREF(ret);
        return is_number;
    }
}
