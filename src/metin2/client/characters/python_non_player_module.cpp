#include "python_non_player.h"

#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include "instance_base.h"
#include "python_character_manager.h"

namespace metin2::inline client::inline characters {

    auto nonplayer_get_level_by_vid(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!PyArg_ParseTuple(args, "i", &virtual_id)) {
            return nullptr;
        }

        instance_base *instance = get_python_character_manager().instance_ptr(virtual_id);

        if (!instance)
            return Py_BuildValue("i", -1);

        const python_non_player::mob_table *mob_table = get_python_non_player().table(instance->virtual_number());

        if (!mob_table)
            return Py_BuildValue("i", -1);

        float average_level = mob_table->level_value;
        average_level = floor(average_level + 0.5f);
        return Py_BuildValue("i", int(average_level));
    }

    auto nonplayer_get_grade_by_vid(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!PyArg_ParseTuple(args, "i", &virtual_id)) {
            return nullptr;
        }

        instance_base *instance = get_python_character_manager().instance_ptr(virtual_id);

        if (!instance)
            return Py_BuildValue("i", -1);

        const python_non_player::mob_table *mob_table = get_python_non_player().table(instance->virtual_number());

        if (!mob_table)
            return Py_BuildValue("i", -1);

        return Py_BuildValue("i", mob_table->rank);
    }

    auto nonplayer_get_monster_name(PyObject *self, PyObject *args) -> PyObject * {
        int v_num;
        if (!PyArg_ParseTuple(args, "i", &v_num)) {
            return nullptr;
        }

        python_non_player &non_player = get_python_non_player();
        return Py_BuildValue("s", non_player.monster_name(v_num));
    }

    void init_characters_nonplayer_module() {
        static PyMethodDef s_methods[] =
            {
                {"get_level_by_vid", nonplayer_get_level_by_vid, METH_VARARGS},
                {"get_grade_by_vid", nonplayer_get_grade_by_vid, METH_VARARGS},
                {"get_monster_name", nonplayer_get_monster_name, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module = init_python_module("metin2.characters.nonplayer", s_methods);

        PyModule_AddIntConstant(module, "ON_CLICK_EVENT_NONE", python_non_player::ON_CLICK_EVENT_NONE);
        PyModule_AddIntConstant(module, "ON_CLICK_EVENT_BATTLE", python_non_player::ON_CLICK_EVENT_BATTLE);
        PyModule_AddIntConstant(module, "ON_CLICK_EVENT_SHOP", python_non_player::ON_CLICK_EVENT_SHOP);
        PyModule_AddIntConstant(module, "ON_CLICK_EVENT_TALK", python_non_player::ON_CLICK_EVENT_TALK);
        PyModule_AddIntConstant(module, "ON_CLICK_EVENT_VEHICLE", python_non_player::ON_CLICK_EVENT_VEHICLE);

        PyModule_AddIntConstant(module, "PAWN", 0);
        PyModule_AddIntConstant(module, "S_PAWN", 1);
        PyModule_AddIntConstant(module, "KNIGHT", 2);
        PyModule_AddIntConstant(module, "S_KNIGHT", 3);
        PyModule_AddIntConstant(module, "BOSS", 4);
        PyModule_AddIntConstant(module, "KING", 5);
    }
}
