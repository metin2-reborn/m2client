#include <metin2/client/characters/python_character_manager.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline app {
    auto effect_register_indexed_fly_data(PyObject *self, PyObject *args) -> PyObject * {
        int index, type;
        if (!py_tuple_get_integer(args, 0, &index) || !py_tuple_get_integer(args, 1, &type)) {
            return py_build_exception();
        }

        char *fly_data_name;
        if (!py_tuple_get_string(args, 2, &fly_data_name)) {
            return py_build_exception();
        }

        get_flying_manager().register_indexed_fly_data(index, type, fly_data_name);

        return py_build_none();
    }

    void initeffect() {
        static PyMethodDef methods[] = {
            {"RegisterIndexedFlyData", effect_register_indexed_fly_data, METH_VARARGS},

            {nullptr, nullptr, 0, nullptr},
        };

        auto *module_value = Py_InitModule("effect", methods);

        PyModule_AddIntConstant(module_value, "INDEX_FLY_TYPE_NORMAL", flying_manager::INDEX_FLY_TYPE_NORMAL);
        PyModule_AddIntConstant(module_value, "INDEX_FLY_TYPE_FIRE_CRACKER", flying_manager::INDEX_FLY_TYPE_FIRE_CRACKER);
        PyModule_AddIntConstant(module_value, "INDEX_FLY_TYPE_AUTO_FIRE", flying_manager::INDEX_FLY_TYPE_AUTO_FIRE);

        enum {
            FLY_NONE,
            FLY_EXP,
            FLY_HP_MEDIUM,
            FLY_HP_BIG,
            FLY_SP_SMALL,
            FLY_SP_MEDIUM,
            FLY_SP_BIG,
            FLY_FIREWORK1,
            FLY_FIREWORK2,
            FLY_FIREWORK3,
            FLY_FIREWORK4,
            FLY_FIREWORK5,
            FLY_FIREWORK6,
            FLY_FIREWORK_XMAS,
            FLY_CHAIN_LIGHTNING,
            FLY_HP_SMALL,
            FLY_SKILL_MUYEONG,
        };

        PyModule_AddIntConstant(module_value, "FLY_NONE", FLY_NONE);
        PyModule_AddIntConstant(module_value, "FLY_EXP", FLY_EXP);
        PyModule_AddIntConstant(module_value, "FLY_HP_MEDIUM", FLY_HP_MEDIUM);
        PyModule_AddIntConstant(module_value, "FLY_HP_BIG", FLY_HP_BIG);
        PyModule_AddIntConstant(module_value, "FLY_SP_SMALL", FLY_SP_SMALL);
        PyModule_AddIntConstant(module_value, "FLY_SP_MEDIUM", FLY_SP_MEDIUM);
        PyModule_AddIntConstant(module_value, "FLY_SP_BIG", FLY_SP_BIG);
        PyModule_AddIntConstant(module_value, "FLY_FIREWORK1", FLY_FIREWORK1);
        PyModule_AddIntConstant(module_value, "FLY_FIREWORK2", FLY_FIREWORK2);
        PyModule_AddIntConstant(module_value, "FLY_FIREWORK3", FLY_FIREWORK3);
        PyModule_AddIntConstant(module_value, "FLY_FIREWORK4", FLY_FIREWORK4);
        PyModule_AddIntConstant(module_value, "FLY_FIREWORK5", FLY_FIREWORK5);
        PyModule_AddIntConstant(module_value, "FLY_FIREWORK6", FLY_FIREWORK6);
        PyModule_AddIntConstant(module_value, "FLY_FIREWORK_XMAS", FLY_FIREWORK_XMAS);
        PyModule_AddIntConstant(module_value, "FLY_CHAIN_LIGHTNING", FLY_CHAIN_LIGHTNING);
        PyModule_AddIntConstant(module_value, "FLY_HP_SMALL", FLY_HP_SMALL);
        PyModule_AddIntConstant(module_value, "FLY_SKILL_MUYEONG", FLY_SKILL_MUYEONG);
    }
}
