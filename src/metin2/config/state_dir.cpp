#include "state_dir.h"

#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace metin2::state {
    void make_directories(const std::string &path) {
        std::error_code error;
        std::filesystem::create_directories(path, error);
    }

    auto directory() -> const std::string & {
        static const std::string value = [] -> std::string {
#ifdef _WIN32
            // %APPDATA%\metin2, as Windows applications keep their settings there.
            const char *base = std::getenv("APPDATA");
            std::string dir = std::string(base && *base ? base : ".") + "/metin2";
#else
            const char *home = std::getenv("HOME");
            std::string dir = std::string(home && *home ? home : ".") + "/.metin2";
#endif
            make_directories(dir);
            return dir;
        }();
        return value;
    }

    auto path(std::string_view name, bool create_directory) -> std::string {
        std::string result = directory() + "/" + std::string(name);
        if (create_directory)
            make_directories(result);
        return result;
    }
}
