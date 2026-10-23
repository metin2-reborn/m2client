#include "python_quest.h"
#include <metin2/core/timer.h>
#include <metin2/graphics/grp_image.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline quest {

    void python_quest::register_quest_instance(const quest_instance &c_r_quest_instance) {
        delete_quest_instance(c_r_quest_instance.index_value);
        quest_instance_container_value_.push_back(c_r_quest_instance);

        quest_instance &r_quest_instance = *quest_instance_container_value_.rbegin();
        r_quest_instance.start_time = int(get_timer().current_second());
    }

    struct quest_instance_compare_fn {
        uint32_t search_index;
        quest_instance_compare_fn(uint32_t index) : search_index(index) {}
        auto operator()(const python_quest::quest_instance &quest_instance) -> bool {
            return search_index == quest_instance.index_value;
        }
    };

    void python_quest::delete_quest_instance(uint32_t index) {
        auto itor = std::find_if(quest_instance_container_value_.begin(), quest_instance_container_value_.end(), quest_instance_compare_fn(index));
        if (itor == quest_instance_container_value_.end())
            return;

        quest_instance_container_value_.erase(itor);
    }

    auto python_quest::is_quest(uint32_t index) -> bool {
        auto itor = std::find_if(quest_instance_container_value_.begin(), quest_instance_container_value_.end(), quest_instance_compare_fn(index));
        return itor != quest_instance_container_value_.end();
    }

    void python_quest::make_quest(uint32_t index) {
        delete_quest_instance(index);
        quest_instance_container_value_.push_back(quest_instance());

        quest_instance &quest_instance_value = *quest_instance_container_value_.rbegin();
        quest_instance_value.index_value = index;
        quest_instance_value.start_time = int(get_timer().current_second());
    }

    void python_quest::set_quest_title(uint32_t index, const char *title) {
        quest_instance *quest_instance_value;
        if (!quest_instance_ptr(index, &quest_instance_value))
            return;

        quest_instance_value->title = title;
    }

    void python_quest::set_quest_clock_name(uint32_t index, const char *clock_name) {
        quest_instance *quest_instance_value;
        if (!quest_instance_ptr(index, &quest_instance_value))
            return;

        quest_instance_value->clock_name = clock_name;
    }

    void python_quest::set_quest_counter_name(uint32_t index, const char *counter_name) {
        quest_instance *quest_instance_value;
        if (!quest_instance_ptr(index, &quest_instance_value))
            return;

        quest_instance_value->counter_name = counter_name;
    }

    void python_quest::set_quest_clock_value(uint32_t index, int clock_value) {
        quest_instance *quest_instance_value;
        if (!quest_instance_ptr(index, &quest_instance_value))
            return;

        quest_instance_value->clock_value = clock_value;
        quest_instance_value->start_time = int(get_timer().current_second());
    }

    void python_quest::set_quest_counter_value(uint32_t index, int counter_value) {
        quest_instance *quest_instance_value;
        if (!quest_instance_ptr(index, &quest_instance_value))
            return;

        quest_instance_value->counter_value = counter_value;
    }

    void python_quest::set_quest_icon_file_name(uint32_t index, const char *icon_file_name) {
        quest_instance *quest_instance_value;
        if (!quest_instance_ptr(index, &quest_instance_value))
            return;

        quest_instance_value->icon_file_name = icon_file_name;
    }

    auto python_quest::quest_count() -> int {
        return quest_instance_container_value_.size();
    }

    auto python_quest::get_quest_instance_ptr(uint32_t array_index, quest_instance **quest_instance_value) -> bool {
        if (array_index >= quest_instance_container_value_.size())
            return false;

        *quest_instance_value = &quest_instance_container_value_[array_index];

        return true;
    }

    auto python_quest::quest_instance_ptr(uint32_t quest_index, quest_instance **quest_instance_value) -> bool {
        auto itor = std::find_if(quest_instance_container_value_.begin(), quest_instance_container_value_.end(), quest_instance_compare_fn(quest_index));
        if (itor == quest_instance_container_value_.end())
            return false;

        *quest_instance_value = &(*itor);

        return true;
    }

    void python_quest::initialize_impl() {
    }

    void python_quest::clear() {
        quest_instance_container_value_.clear();
    }

    python_quest::python_quest() {
        initialize_impl();
    }

    python_quest::~python_quest() {
        clear();
    }

    auto quest_get_quest_count(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_quest().quest_count());
    }

    auto quest_get_quest_data(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        python_quest::quest_instance *quest_instance;
        if (!get_python_quest().get_quest_instance_ptr(index, &quest_instance)) {
            PyErr_Format(PyExc_ValueError, "Unknown quest index %d", index);
            return nullptr;
        }

        graphic_image *image = nullptr;
        if (!quest_instance->icon_file_name.empty()) {
            std::string icon_file_name;
            icon_file_name = "d:/ymir work/ui/game/quest/questicon/";
            icon_file_name += quest_instance->icon_file_name.c_str();
            image = (graphic_image *)get_resource_manager().resource_pointer(icon_file_name.c_str());
        } else {
            {
                std::string icon_file_name = "season1/icon/scroll_open.tga";
                image = (graphic_image *)get_resource_manager().resource_pointer(icon_file_name.c_str());
            }
        }

        return Py_BuildValue("snsi", quest_instance->title.c_str(),
            (Py_ssize_t)image,
            quest_instance->counter_name.c_str(),
            quest_instance->counter_value);
    }

    auto quest_get_quest_index(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        python_quest::quest_instance *quest_instance;
        if (!get_python_quest().get_quest_instance_ptr(index, &quest_instance)) {
            PyErr_Format(PyExc_ValueError, "Unknown quest index %d", index);
            return nullptr;
        }

        return Py_BuildValue("i", quest_instance->index_value);
    }

    auto quest_get_quest_last_time(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        python_quest::quest_instance *quest_instance;
        if (!get_python_quest().get_quest_instance_ptr(index, &quest_instance)) {
            PyErr_Format(PyExc_ValueError, "Unknown quest index %d", index);
            return nullptr;
        }

        int last_time = 0;

        if (quest_instance->clock_value >= 0) {
            last_time = (quest_instance->start_time + quest_instance->clock_value) - int(get_timer().current_second());
        }

        return Py_BuildValue("si", quest_instance->clock_name.c_str(), last_time);
    }

    auto quest_clear(PyObject *self, PyObject *args) -> PyObject * {
        get_python_quest().clear();
        Py_RETURN_NONE;
    }

    void init_quest_module() {
        static PyMethodDef s_methods[] =
            {
                {"get_quest_count", quest_get_quest_count, METH_VARARGS},
                {"get_quest_data", quest_get_quest_data, METH_VARARGS},
                {"get_quest_index", quest_get_quest_index, METH_VARARGS},
                {"get_quest_last_time", quest_get_quest_last_time, METH_VARARGS},
                {"clear", quest_clear, METH_VARARGS},
                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module = init_python_module("metin2.quest", s_methods);
        PyModule_AddIntConstant(module, "QUEST_MAX_NUM", 5);
    }
}
