
#include <metin2/files/eter_file_manager.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline app {
    auto pack_exist(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;

        if (!py_tuple_get_string(args, 0, &file_name))
            return py_build_exception();

        return Py_BuildValue("i", get_file_manager().is_exist(file_name) ? 1 : 0);
    }

    auto pack_get(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;

        if (!py_tuple_get_string(args, 0, &file_name))
            return py_build_exception();

        // Python may only read .py, .pyc and .txt files from the packs.
        const char *ext = strrchr(file_name, '.');
        if (ext) {
            if ((strcasecmp(ext, ".py") == 0) ||
                (strcasecmp(ext, ".pyc") == 0) ||
                (strcasecmp(ext, ".txt") == 0)) {
                mapped_file file;
                const void *data = nullptr;

                if (get_file_manager().get(file, file_name, &data))
                    return Py_BuildValue("s#", data, file.size());
            }
        }

        return py_build_exception();
    }

    void initpack() {
        static PyMethodDef s_methods[] =
            {
                {"Exist", pack_exist, METH_VARARGS},
                {"Get", pack_get, METH_VARARGS},
                {nullptr, nullptr},
        };

        Py_InitModule("pack", s_methods);
    }
}
