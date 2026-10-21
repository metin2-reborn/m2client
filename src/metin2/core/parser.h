#pragma once

#include <list>
#include <string>

namespace metin2::inline core {

    namespace event_script {
        struct argumet {
            argumet(const std::string &name, const std::string &value_value) {
                name_value = name;
                value = value_value;
            }
            argumet(const argumet &c_arg) {
                name_value = c_arg.name_value;
                value = c_arg.value;
            }
            void operator=(const argumet &c_arg) {
                name_value = c_arg.name_value;
                value = c_arg.value;
            }
            std::string name_value;
            std::string value;
        };
        using arg = argumet;

        using arg_list = std::list<arg>;

        struct cmd {
            std::string name_value;
            arg_list arg_list_value;

            cmd() = default;
            cmd(const cmd &c_cmd) {
                name_value = c_cmd.name_value;
                arg_list_value = c_cmd.arg_list_value;
            }
            void operator=(const cmd &c_cmd) {
                name_value = c_cmd.name_value;
                arg_list_value = c_cmd.arg_list_value;
            }
        };

        class group {
          public:
            group();
            ~group();

          public:
            auto create(const std::string &source) -> bool;

            // Removes and returns the next command; false when none is left.
            auto get_cmd(cmd &cmd) -> bool;

            // Returns the next command without removing it.
            auto read_cmd(cmd &cmd) -> bool;

          private:
            void set_error(const char *str);
            auto get_arg(const char *c_atr_base, int arg_len, arg_list &arg_list) -> bool;

            std::string error_;
            std::list<cmd> cmd_list_;
        };
    }
}
