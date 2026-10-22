#include "python_module.h"

#include <string>
#include <string_view>

namespace metin2::inline script {

    auto init_python_module(const char *dotted_name, PyMethodDef *methods) -> PyObject * {
        PyObject *module = Py_InitModule(dotted_name, methods);
        if (!module)
            return nullptr;

        std::string_view name = dotted_name;
        PyObject *child = module;
        for (auto dot = name.rfind('.'); dot != std::string_view::npos; dot = name.rfind('.')) {
            const std::string leaf(name.substr(dot + 1));
            name = name.substr(0, dot);
            const std::string parent_name(name);

            // Borrowed; created (and put into sys.modules) when it does not exist yet.
            PyObject *parent = PyImport_AddModule(parent_name.c_str());
            if (!parent)
                return nullptr;
            if (!PyObject_HasAttrString(parent, "__path__")) {
                // An empty __path__ marks the container as a package without giving the stock
                // path importer anywhere on disk to look.
                PyObject *path = PyList_New(0);
                if (!path || PyModule_AddObject(parent, "__path__", path) < 0)
                    return nullptr;
            }
            Py_INCREF(child); // PyModule_AddObject steals the reference
            if (PyModule_AddObject(parent, leaf.c_str(), child) < 0)
                return nullptr;
            child = parent;
        }
        return module;
    }
}
