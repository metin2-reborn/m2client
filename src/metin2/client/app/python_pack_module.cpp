#include <metin2/files/eter_file_manager.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline app {
    auto pack_exist(PyObject *self, PyObject *args) -> PyObject * {
        char *path;
        if (!PyArg_ParseTuple(args, "s", &path)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_file_manager().is_exist(path) ? 1 : 0);
    }

    auto pack_get(PyObject *self, PyObject *args) -> PyObject * {
        char *path;
        if (!PyArg_ParseTuple(args, "s", &path)) {
            return nullptr;
        }

        const char *ext = strrchr(path, '.');
        if (ext) {
            if ((strcasecmp(ext, ".py") == 0) ||
                (strcasecmp(ext, ".pyc") == 0) ||
                (strcasecmp(ext, ".txt") == 0) ||
                (strcasecmp(ext, ".xml") == 0)) {
                mapped_file file;

                const void *data = nullptr;
                if (get_file_manager().get(file, path, &data)) {
                    return Py_BuildValue("s#", data, file.size());
                }

                PyErr_SetString(PyExc_IOError, "File not found");
                return nullptr;
            }
        }

        PyErr_SetString(PyExc_ValueError, "Scripts may only read .py, .pyc, .txt and .xml files");
        return nullptr;
    }

    void init_files_module() {
        static PyMethodDef methods[] = {
            {"exist", pack_exist, METH_VARARGS},
            {"get", pack_get, METH_VARARGS},
            {nullptr, nullptr},
        };

        init_python_module("metin2.files", methods);
    }
}
