#include "python_safe_box.h"
#include <metin2/log/log.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline trade {

    void python_safe_box::open_safe_box(int size) {
        money_ = 0;
        item_instance_vector_value_.clear();
        item_instance_vector_value_.resize(SAFEBOX_SLOT_X_COUNT * size);

        for (uint32_t i = 0; i < item_instance_vector_value_.size(); ++i) {
            packet_item &instance = item_instance_vector_value_[i];
            memset(&instance, 0, sizeof(instance));
        }
    }

    void python_safe_box::set_item_data(uint32_t slot_index, const packet_item &item_instance) {
        if (slot_index >= item_instance_vector_value_.size()) {
            metin2::log::error("Safe box slot {} out of range", slot_index);
            return;
        }

        item_instance_vector_value_[slot_index] = item_instance;
    }

    void python_safe_box::del_item_data(uint32_t slot_index) {
        if (slot_index >= item_instance_vector_value_.size()) {
            metin2::log::error("Safe box slot {} out of range", slot_index);
            return;
        }

        packet_item &instance = item_instance_vector_value_[slot_index];
        memset(&instance, 0, sizeof(instance));
    }

    auto python_safe_box::money() -> uint32_t {
        return money_;
    }

    auto python_safe_box::slot_item_id(uint32_t slot_index, uint32_t *item_id) -> bool {
        if (slot_index >= item_instance_vector_value_.size()) {
            metin2::log::error("Safe box slot {} out of range", slot_index);
            return false;
        }

        *item_id = item_instance_vector_value_[slot_index].vnum;

        return true;
    }

    auto python_safe_box::item_data_ptr(uint32_t slot_index, packet_item **instance) -> bool {
        if (slot_index >= item_instance_vector_value_.size()) {
            metin2::log::error("Safe box slot {} out of range", slot_index);
            return false;
        }

        *instance = &item_instance_vector_value_[slot_index];

        return true;
    }

    void python_safe_box::open_mall(int size) {
        mall_item_instance_vector_.clear();
        mall_item_instance_vector_.resize(SAFEBOX_SLOT_X_COUNT * size);

        for (uint32_t i = 0; i < mall_item_instance_vector_.size(); ++i) {
            packet_item &instance = mall_item_instance_vector_[i];
            memset(&instance, 0, sizeof(instance));
        }
    }

    void python_safe_box::set_mall_item_data(uint32_t slot_index, const packet_item &item_data) {
        if (slot_index >= mall_item_instance_vector_.size()) {
            metin2::log::error("Mall slot {} out of range", slot_index);
            return;
        }

        mall_item_instance_vector_[slot_index] = item_data;
    }

    void python_safe_box::del_mall_item_data(uint32_t slot_index) {
        if (slot_index >= mall_item_instance_vector_.size()) {
            metin2::log::error("Mall slot {} out of range", slot_index);
            return;
        }

        packet_item &instance = mall_item_instance_vector_[slot_index];
        memset(&instance, 0, sizeof(instance));
    }

    auto python_safe_box::mall_item_data_ptr(uint32_t slot_index, packet_item **instance) -> bool {
        if (slot_index >= mall_item_instance_vector_.size()) {
            metin2::log::error("Mall slot {} out of range", slot_index);
            return false;
        }

        *instance = &mall_item_instance_vector_[slot_index];

        return true;
    }

    auto python_safe_box::slot_mall_item_id(uint32_t slot_index, uint32_t *item_id) -> bool {
        if (slot_index >= mall_item_instance_vector_.size()) {
            metin2::log::error("Mall slot {} out of range", slot_index);
            return false;
        }

        *item_id = mall_item_instance_vector_[slot_index].vnum;

        return true;
    }

    auto python_safe_box::mall_size() -> uint32_t {
        return mall_item_instance_vector_.size();
    }

    python_safe_box::python_safe_box() {
        money_ = 0;
    }

    python_safe_box::~python_safe_box() = default;

    auto safebox_get_item_id(PyObject *self, PyObject *args) -> PyObject * {
        int ipos;
        if (!py_tuple_get_integer(args, 0, &ipos))
            return py_bad_argument();

        packet_item *instance;
        if (!get_python_safe_box().item_data_ptr(ipos, &instance))
            return py_build_exception();

        return Py_BuildValue("i", instance->vnum);
    }

    auto safebox_get_item_count(PyObject *self, PyObject *args) -> PyObject * {
        int ipos;
        if (!py_tuple_get_integer(args, 0, &ipos))
            return py_bad_argument();

        packet_item *instance;
        if (!get_python_safe_box().item_data_ptr(ipos, &instance))
            return py_build_exception();

        return Py_BuildValue("i", instance->count);
    }

    auto safebox_get_item_flags(PyObject *self, PyObject *args) -> PyObject * {
        int ipos;
        if (!py_tuple_get_integer(args, 0, &ipos))
            return py_bad_argument();

        packet_item *instance;
        if (!get_python_safe_box().item_data_ptr(ipos, &instance))
            return py_build_exception();

        return Py_BuildValue("i", instance->flags_value);
    }

    auto safebox_get_item_metin_socket(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_bad_argument();
        int socket_index;
        if (!py_tuple_get_integer(args, 1, &socket_index))
            return py_bad_argument();

        if (socket_index >= ITEM_SOCKET_SLOT_MAX_NUM)
            return py_build_exception();

        packet_item *item_data;
        if (!get_python_safe_box().item_data_ptr(slot_index, &item_data))
            return py_build_exception();

        return Py_BuildValue("i", item_data->sockets[socket_index]);
    }

    auto safebox_get_item_attribute(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();
        int attr_slot_index;
        if (!py_tuple_get_integer(args, 1, &attr_slot_index))
            return py_build_exception();

        if (attr_slot_index >= 0 && attr_slot_index < ITEM_ATTRIBUTE_SLOT_MAX_NUM) {
            packet_item *item_data;
            if (get_python_safe_box().item_data_ptr(slot_index, &item_data))
                return Py_BuildValue("ii", item_data->attr[attr_slot_index].type_value, item_data->attr[attr_slot_index].value);
        }

        return Py_BuildValue("ii", 0, 0);
    }

    auto safebox_get_money(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_safe_box().money());
    }

    auto safebox_get_mall_item_id(PyObject *self, PyObject *args) -> PyObject * {
        int ipos;
        if (!py_tuple_get_integer(args, 0, &ipos))
            return py_bad_argument();

        packet_item *instance;
        if (!get_python_safe_box().mall_item_data_ptr(ipos, &instance))
            return py_build_exception();

        return Py_BuildValue("i", instance->vnum);
    }

    auto safebox_get_mall_item_count(PyObject *self, PyObject *args) -> PyObject * {
        int ipos;
        if (!py_tuple_get_integer(args, 0, &ipos))
            return py_bad_argument();

        packet_item *instance;
        if (!get_python_safe_box().mall_item_data_ptr(ipos, &instance))
            return py_build_exception();

        return Py_BuildValue("i", instance->count);
    }

    auto safebox_get_mall_item_metin_socket(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_bad_argument();
        int socket_index;
        if (!py_tuple_get_integer(args, 1, &socket_index))
            return py_bad_argument();

        if (socket_index >= ITEM_SOCKET_SLOT_MAX_NUM)
            return py_build_exception();

        packet_item *item_data;
        if (!get_python_safe_box().mall_item_data_ptr(slot_index, &item_data))
            return py_build_exception();

        return Py_BuildValue("i", item_data->sockets[socket_index]);
    }

    auto safebox_get_mall_item_attribute(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();
        int attr_slot_index;
        if (!py_tuple_get_integer(args, 1, &attr_slot_index))
            return py_build_exception();

        if (attr_slot_index >= 0 && attr_slot_index < ITEM_ATTRIBUTE_SLOT_MAX_NUM) {
            packet_item *item_data;
            if (get_python_safe_box().mall_item_data_ptr(slot_index, &item_data))
                return Py_BuildValue("ii", item_data->attr[attr_slot_index].type_value, item_data->attr[attr_slot_index].value);
        }

        return Py_BuildValue("ii", 0, 0);
    }

    auto safebox_get_mall_size(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_safe_box().mall_size());
    }

    void init_trade_safebox_module() {
        static PyMethodDef s_methods[] =
            {
                {"GetItemID", safebox_get_item_id, METH_VARARGS},
                {"GetItemCount", safebox_get_item_count, METH_VARARGS},
                {"GetItemFlags", safebox_get_item_flags, METH_VARARGS},
                {"GetItemMetinSocket", safebox_get_item_metin_socket, METH_VARARGS},
                {"GetItemAttribute", safebox_get_item_attribute, METH_VARARGS},
                {"GetMoney", safebox_get_money, METH_VARARGS},

                {"GetMallItemID", safebox_get_mall_item_id, METH_VARARGS},
                {"GetMallItemCount", safebox_get_mall_item_count, METH_VARARGS},
                {"GetMallItemMetinSocket", safebox_get_mall_item_metin_socket, METH_VARARGS},
                {"GetMallItemAttribute", safebox_get_mall_item_attribute, METH_VARARGS},
                {"GetMallSize", safebox_get_mall_size, METH_VARARGS},
                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = init_python_module("metin2.trade.safebox", s_methods);
        PyModule_AddIntConstant(module_value, "SAFEBOX_SLOT_X_COUNT", python_safe_box::SAFEBOX_SLOT_X_COUNT);
        PyModule_AddIntConstant(module_value, "SAFEBOX_SLOT_Y_COUNT", python_safe_box::SAFEBOX_SLOT_Y_COUNT);
        PyModule_AddIntConstant(module_value, "SAFEBOX_PAGE_SIZE", python_safe_box::SAFEBOX_PAGE_SIZE);
    }
}
