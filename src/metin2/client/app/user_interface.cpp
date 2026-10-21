#include <metin2/config/state_dir.h>
#include <metin2/core/lzo.h>
#include <metin2/core/utils.h>
#include <metin2/dev/automation.h>
#include <metin2/graphics/camera.h>
#include <metin2/log/log.h>
#include "python_application.h"
#include "python_exception_sender.h"
#include "resource_ids.h"

#include <metin2/core/util.h>
#include <metin2/files/eter_file_manager.h>

#include "check_latest_files.h"

#include <unistd.h>

#include <cstdlib>
#include <memory>
#include <metin2/script/python_debug_module.h>
#include <metin2/script/python_launcher.h>
#include <metin2/ui/python_modules.h>
#include "client_modules.h"
#include "locale.h"

namespace metin2::inline client::inline app {
    bool is_test_server_mode = false;

    static const char *python_library_file_names[] =
        {
            "UserDict.pyc",
            "__future__.pyc",
            "copy_reg.pyc",
            "linecache.pyc",
            "ntpath.pyc",
            "os.pyc",
            "site.pyc",
            "stat.pyc",
            "string.pyc",
            "traceback.pyc",
            "types.pyc",
            "\n",
    };

    auto check_python_library_filenames() -> bool {
        for (int i = 0; *python_library_file_names[i] != '\n'; ++i) {
            std::string filename = "lib/";
            filename += python_library_file_names[i];

            if (access(filename.c_str(), F_OK) != 0)
                return false;
        }

        return true;
    }

    // The strings of the Windows resource script's English string table. The Windows build could
    // override them per locale from metin2client.dat, which the game data does not contain.
    auto application_string_table_get_stringz(uint32_t id) -> const char * {
        switch (id) {
        case IDS_APP_NAME:
            return "Metin 2";
        case IDS_POSSESSIVE_MORPHENE:
            return "'s";
        case IDS_WARN_BAD_DRIVER:
            return "Please update your graphics driver.";
        case IDS_WARN_NO_TNL:
            return "Your graphics card has no hardware 3D T&L acceleration;\nthe game may run slowly or not at all.";
        }
        return "";
    }

    auto setup() -> int;

    // Assets come from the zip archives in data/ (CEterFileManager); these only set the loaders up.
    void file_system_initialize() {
        text_file_loader::set_cache_mode();
    }

    auto run_main_script(python_launcher &py_launcher) -> bool {
        initpack();
        initdbg();
        initime();
        initgrp();
        initgrp_image();
        initgrp_text();
        initwnd_mgr();
        initapp();
        initsystem();
        initchr();
        initchrmgr();
        init_player();
        init_item();
        init_non_player();
        init_trade();
        init_chat();
        init_text_tail();
        initnet();
        init_mini_map();
        init_event();
        initeffect();
        initfly();
        initsnd();
        initshop();
        initskill();
        initquest();
        init_background();
        init_messenger();
        initsafebox();
        initguild();
        init_server_state_checker();

#if M2_DEV_TOOLS
        // Lets the automation harness run Python inside the client (pyexec command).
        python_launcher *launcher = &py_launcher;
        dev::set_py_exec([launcher](const char *code, std::string *err) -> bool {
            bool ok = launcher->run_line(code);
            if (!ok && err) *err = "python error (see ~/.metin2/log/syserr.log)";
            return ok;
        });
#endif

        {
            std::string register_debug_flag;

            register_debug_flag = "__DEBUG__ = 1";

            if (!py_launcher.run_line(register_debug_flag.c_str())) {
                metin2::log::error("Cannot set the Python debug flag");
                return false;
            }
        }

        // system.py expects this global.
        if (!py_launcher.run_line("__COMMAND_LINE__ = \"\"")) {
            metin2::log::error("Cannot set the Python command line");
            return false;
        }
        {
            if (!py_launcher.run_file("system.py")) {
                metin2::log::error("Cannot run system.py");
                return false;
            }
        }

        return true;
    }

    auto run_client() -> bool {
        if (!setup())
            return false;

        metin2::log::init(metin2::state::path("log"));

        get_camera_manager();
        get_clzo();
        file_system_initialize();

        python_application &app = get_python_application();

        bool ret = false;
        {
            python_launcher &py_launcher = get_python_launcher();
            python_exception_sender py_exception_sender;
            set_exception_sender(&py_exception_sender);

            if (py_launcher.create()) {
                ret = run_main_script(py_launcher); // does not return until the game ends
            }

            app.clear();

            py_launcher.clear();
        }

        app.destroy();

        return ret;
    }

    void error_python_library_is_not_exist() {
        metin2::log::error("The Python library file does not exist");
    }

    auto run() -> int {
        if (!check_python_library_filenames()) {
            error_python_library_is_not_exist();
            return 0;
        }

        run_client();
        return 0;
    }

    auto setup() -> int {
        granny_log_callback callback;
        callback.Function = nullptr;
        callback.UserData = nullptr;
        GrannySetLogCallback(&callback);
        return 1;
    }
}
