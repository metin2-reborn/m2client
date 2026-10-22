#include "python_non_player.h"

#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include "instance_base.h"
#include "python_character_manager.h"

namespace metin2::inline client::inline characters {

    auto nonplayer_get_level_by_vid(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

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
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

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
        if (!py_tuple_get_integer(args, 0, &v_num))
            return py_build_exception();

        python_non_player &non_player = get_python_non_player();
        return Py_BuildValue("s", non_player.monster_name(v_num));
    }

    void init_characters_nonplayer_module() {
        static PyMethodDef s_methods[] =
            {
                {"GetLevelByVID", nonplayer_get_level_by_vid, METH_VARARGS},
                {"GetGradeByVID", nonplayer_get_grade_by_vid, METH_VARARGS},
                {"GetMonsterName", nonplayer_get_monster_name, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = init_python_module("metin2.characters.nonplayer", s_methods);

        PyModule_AddIntConstant(module_value, "ON_CLICK_EVENT_NONE", python_non_player::ON_CLICK_EVENT_NONE);
        PyModule_AddIntConstant(module_value, "ON_CLICK_EVENT_BATTLE", python_non_player::ON_CLICK_EVENT_BATTLE);
        PyModule_AddIntConstant(module_value, "ON_CLICK_EVENT_SHOP", python_non_player::ON_CLICK_EVENT_SHOP);
        PyModule_AddIntConstant(module_value, "ON_CLICK_EVENT_TALK", python_non_player::ON_CLICK_EVENT_TALK);
        PyModule_AddIntConstant(module_value, "ON_CLICK_EVENT_VEHICLE", python_non_player::ON_CLICK_EVENT_VEHICLE);

        PyModule_AddIntConstant(module_value, "PAWN", 0);
        PyModule_AddIntConstant(module_value, "S_PAWN", 1);
        PyModule_AddIntConstant(module_value, "KNIGHT", 2);
        PyModule_AddIntConstant(module_value, "S_KNIGHT", 3);
        PyModule_AddIntConstant(module_value, "BOSS", 4);
        PyModule_AddIntConstant(module_value, "KING", 5);
    }
}
