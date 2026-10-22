#pragma once

#include "python_header.h"

namespace metin2::inline script {

    // Registers a module under its dotted name ("metin2.graphics.text") and links it into its parent
    // packages, creating empty containers ("metin2", "metin2.graphics") the first time they are
    // needed. Python 2.7 keys sys.modules by the full name and looks there before asking
    // sys.meta_path, so `import metin2.graphics.text`, `from metin2 import graphics` and attribute
    // access all resolve without involving the game-script loader. Parent and child may be
    // registered in any order: Py_InitModule fills in a container that a child created earlier.
    // Returns a borrowed reference, like Py_InitModule.
    auto init_python_module(const char *dotted_name, PyMethodDef *methods) -> PyObject *;
}
