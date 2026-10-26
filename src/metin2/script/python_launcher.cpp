#include "python_launcher.h"

#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

#include <metin2/core/mapped_file.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>

namespace metin2::inline script {

    python_launcher::python_launcher() : module_(nullptr), dic_(nullptr) {
        // The standard library is the game directory's lib/ (the working directory is the game
        // directory), nothing else: no site module, no PYTHON* environment, no Python install
        // searched for, no .pyc written into the read-only game directory. The home only keeps
        // Py_Initialize from looking for an install; sys.path is set afterwards.
        Py_NoSiteFlag = 1;
        Py_IgnoreEnvironmentFlag = 1;
        Py_DontWriteBytecodeFlag = 1;
        static char home[] = ".";
        Py_SetPythonHome(home);
        Py_Initialize();

        std::error_code error;
        std::string lib = std::filesystem::absolute("lib", error).string();
        if (error)
            lib = "lib";
        PySys_SetPath(lib.data());
    }

    python_launcher::~python_launcher() {
        clear();
    }

    void python_launcher::clear() {
        Py_Finalize();
    }

    // Logs the pending Python exception (the traceback itself was printed by Python).
    static void traceback() {
        PyObject *exc;
        PyObject *v;
        PyObject *tb;

        PyErr_Fetch(&exc, &v, &tb);

        std::string str;
        if (v && PyString_Check(v)) {
            const char *err_str = PyString_AS_STRING(v);
            str.append("Error: ");
            str.append(err_str);

            metin2::log::debug("{}", err_str);
        }
        Py_XDECREF(exc);
        Py_XDECREF(v);
        Py_XDECREF(tb);
        metin2::log::error("Uncaught Python exception\n\n{}", str.c_str());
    }

    auto python_launcher::create(const char *program_name) -> bool {
        Py_SetProgramName(const_cast<char *>(program_name));
        module_ = PyImport_AddModule("__main__");

        if (!module_)
            return false;

        dic_ = PyModule_GetDict(module_);

        PyObject *builtins = PyImport_ImportModule("__builtin__");
        PyModule_AddIntConstant(builtins, "TRUE", 1);
        PyModule_AddIntConstant(builtins, "FALSE", 0);
        PyDict_SetItemString(dic_, "__builtins__", builtins);
        Py_DECREF(builtins);

        if (!run_line("import __main__"))
            return false;

        if (!run_line("import sys"))
            return false;

        return true;
    }

    auto python_launcher::run_memory_text_file(const char *file_name, uint32_t file_size, const void *c_pv_file_data) -> bool {
        const char *c_pc_file_data = static_cast<const char *>(c_pv_file_data);

        std::string conv_file_data;
        conv_file_data.reserve(file_size + 64);
        conv_file_data += "exec(compile('''";

        // Drop the carriage returns of CRLF sources.
        for (uint32_t i = 0; i < file_size; ++i) {
            if (c_pc_file_data[i] != '\r')
                conv_file_data += c_pc_file_data[i];
        }

        conv_file_data += "''', '";
        conv_file_data += file_name;
        conv_file_data += "', 'exec'))";

        return run_line(conv_file_data.c_str());
    }

    auto python_launcher::run_file(const char *file_name) -> bool {
        mapped_file file;
        const void *data;
        if (!get_file_manager().get(file, file_name, &data))
            return false;

        uint32_t buf_size = file.size();
        if (buf_size == 0)
            return false;

        return run_memory_text_file(file_name, buf_size, data);
    }

    auto python_launcher::run_line(const char *src) -> bool {
        PyObject *v = PyRun_String(src, Py_file_input, dic_, dic_);

        if (!v) {
            traceback();
            return false;
        }

        Py_DECREF(v);
        return true;
    }
}
