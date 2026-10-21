#include "state_dir.h"

#include <cstdlib>
#include <sys/stat.h>

namespace metin2::state {
    void make_directories(const std::string &path) {
        for (std::size_t slash = path.find('/', 1); slash != std::string::npos; slash = path.find('/', slash + 1))
            mkdir(path.substr(0, slash).c_str(), 0755);
        mkdir(path.c_str(), 0755);
    }

    auto directory() -> const std::string & {
        static const std::string value = [] -> std::string {
            const char *home = std::getenv("HOME");
            std::string dir = std::string(home && *home ? home : ".") + "/.metin2";
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
