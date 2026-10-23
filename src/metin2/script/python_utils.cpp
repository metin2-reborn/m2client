#include "python_utils.h"

namespace metin2::inline script {

    static python_exception_sender_interface *exception_sender = nullptr;

    void set_exception_sender(python_exception_sender_interface *exception_sender_value) {
        exception_sender = exception_sender_value;
    }

    auto get_exception_sender() -> python_exception_sender_interface * {
        return exception_sender;
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
