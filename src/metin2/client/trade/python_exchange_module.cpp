#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include "python_exchange.h"

namespace metin2::inline client::inline trade {

    auto exchange_init_trading(PyObject *self, PyObject *args) -> PyObject * {
        get_python_exchange().end();
        return py_build_none();
    }

    auto exchange_get_elk_from_self(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_exchange().elk_from_self());
    }

    auto exchange_get_elk_from_target(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_exchange().elk_from_target());
    }

    auto exchange_get_accept_from_self(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_exchange().accept_from_self());
    }

    auto exchange_get_accept_from_target(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_exchange().accept_from_target());
    }

    auto exchange_get_item_vnum_from_self(PyObject *self, PyObject *args) -> PyObject * {
        int pos;

        if (!py_tuple_get_integer(args, 0, &pos))
            return py_build_exception();

        return Py_BuildValue("i", get_python_exchange().item_vnum_from_self((char)pos));
    }

    auto exchange_get_item_vnum_from_target(PyObject *target, PyObject *args) -> PyObject * {
        int pos;

        if (!py_tuple_get_integer(args, 0, &pos))
            return py_build_exception();

        return Py_BuildValue("i", get_python_exchange().item_vnum_from_target((char)pos));
    }

    auto exchange_get_item_count_from_self(PyObject *self, PyObject *args) -> PyObject * {
        int pos;

        if (!py_tuple_get_integer(args, 0, &pos))
            return py_build_exception();

        return Py_BuildValue("i", get_python_exchange().item_count_from_self((char)pos));
    }

    auto exchange_get_item_count_from_target(PyObject *target, PyObject *args) -> PyObject * {
        int pos;

        if (!py_tuple_get_integer(args, 0, &pos))
            return py_build_exception();

        return Py_BuildValue("i", get_python_exchange().item_count_from_target((char)pos));
    }

    auto exchange_get_name_from_target(PyObject *target, PyObject *args) -> PyObject * {
        return Py_BuildValue("s", get_python_exchange().name_from_target());
    }

    auto exchange_get_item_metin_socket_from_target(PyObject *target, PyObject *args) -> PyObject * {
        int pos;
        if (!py_tuple_get_integer(args, 0, &pos))
            return py_build_exception();
        int metin_socket_pos;
        if (!py_tuple_get_integer(args, 1, &metin_socket_pos))
            return py_build_exception();
        return Py_BuildValue("i", get_python_exchange().item_metin_socket_from_target(pos, metin_socket_pos));
    }

    auto exchange_get_item_metin_socket_from_self(PyObject *target, PyObject *args) -> PyObject * {
        int pos;
        if (!py_tuple_get_integer(args, 0, &pos))
            return py_build_exception();
        int metin_socket_pos;
        if (!py_tuple_get_integer(args, 1, &metin_socket_pos))
            return py_build_exception();
        return Py_BuildValue("i", get_python_exchange().item_metin_socket_from_self(pos, metin_socket_pos));
    }

    auto exchange_get_item_attribute_from_target(PyObject *target, PyObject *args) -> PyObject * {
        int pos;
        if (!py_tuple_get_integer(args, 0, &pos))
            return py_build_exception();
        int attr_slot_pos;
        if (!py_tuple_get_integer(args, 1, &attr_slot_pos))
            return py_build_exception();

        uint8_t type;
        short value;
        get_python_exchange().item_attribute_from_target(pos, attr_slot_pos, &type, &value);

        return Py_BuildValue("ii", type, value);
    }

    auto exchange_get_item_attribute_from_self(PyObject *target, PyObject *args) -> PyObject * {
        int pos;
        if (!py_tuple_get_integer(args, 0, &pos))
            return py_build_exception();
        int attr_slot_pos;
        if (!py_tuple_get_integer(args, 1, &attr_slot_pos))
            return py_build_exception();

        uint8_t type;
        short value;
        get_python_exchange().item_attribute_from_self(pos, attr_slot_pos, &type, &value);

        return Py_BuildValue("ii", type, value);
    }

    void init_trade_exchange_module() {
        static PyMethodDef s_methods[] =
            {
                {"InitTrading", exchange_init_trading, METH_VARARGS},

                {"GetElkFromSelf", exchange_get_elk_from_self, METH_VARARGS},
                {"GetElkFromTarget", exchange_get_elk_from_target, METH_VARARGS},

                {"GetItemVnumFromSelf", exchange_get_item_vnum_from_self, METH_VARARGS},
                {"GetItemVnumFromTarget", exchange_get_item_vnum_from_target, METH_VARARGS},

                {"GetItemCountFromSelf", exchange_get_item_count_from_self, METH_VARARGS},
                {"GetItemCountFromTarget", exchange_get_item_count_from_target, METH_VARARGS},

                {"GetAcceptFromSelf", exchange_get_accept_from_self, METH_VARARGS},
                {"GetAcceptFromTarget", exchange_get_accept_from_target, METH_VARARGS},

                {"GetNameFromTarget", exchange_get_name_from_target, METH_VARARGS},

                {"GetItemMetinSocketFromTarget", exchange_get_item_metin_socket_from_target, METH_VARARGS},
                {"GetItemMetinSocketFromSelf", exchange_get_item_metin_socket_from_self, METH_VARARGS},

                {"GetItemAttributeFromTarget", exchange_get_item_attribute_from_target, METH_VARARGS},
                {"GetItemAttributeFromSelf", exchange_get_item_attribute_from_self, METH_VARARGS},

                {nullptr, nullptr},
        };

        PyObject *module_value = init_python_module("metin2.trade.exchange", s_methods);
        PyModule_AddIntConstant(module_value, "EXCHANGE_ITEM_MAX_NUM", python_exchange::EXCHANGE_ITEM_MAX_NUM);
    }
}
