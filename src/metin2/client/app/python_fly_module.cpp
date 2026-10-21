#include <metin2/script/python_header.h>
#include <metin2/world/flying/flying_object_manager.h>

namespace metin2::inline client::inline app {
    void initfly() {
        static PyMethodDef s_methods[] =
            {

                {nullptr, nullptr, 0, nullptr},
        };

        Py_InitModule("fly", s_methods);
    }
}
