#include "file_dir.h"

#include <cstring>
#include <string>

#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>

namespace metin2::inline core {

    auto dir::create(const char *c_sz_filter, const char *c_sz_path, bool checked_extension) -> bool {
        std::string st_path = c_sz_path;

        if (!st_path.empty()) {
            char end = st_path[st_path.length() - 1];

            if (end != '/' && end != '\\') {
                st_path += '/';
            }
        }

        DIR *dir = opendir(st_path.empty() ? "." : st_path.c_str());
        if (!dir) {
            return true; // like FindFirstFile failing: nothing to visit
        }

        bool result = true;
        while (const dirent *entry = readdir(dir)) {
            const char *name = entry->d_name;

            if (!name[0] || name[0] == '.') {
                continue;
            }

            struct stat st;
            if (stat((st_path + name).c_str(), &st) != 0) {
                continue;
            }

            if (S_ISDIR(st.st_mode)) {
                if (!on_folder(c_sz_filter, st_path.c_str(), name)) {
                    result = false;
                    break;
                }
            } else {
                const char *extension = strchr(name, '.');
                if (!extension) {
                    continue;
                }

                // TODO: checked_extension should always be on, once every caller passes the filter as bare
                // extensions ("wav;gr2").
                if (checked_extension) {
                    std::string str_filter = c_sz_filter;
                    size_t pos = str_filter.find_first_of(';', 0);
                    if (pos != std::string::npos && pos > 0) {
                        std::string first_filter = str_filter.substr(0, pos);
                        std::string second_filter = str_filter.substr(pos + 1);
                        if (0 != first_filter.compare(extension + 1) && 0 != second_filter.compare(extension + 1)) {
                            continue;
                        }
                    } else {
                        if (0 != strcasecmp(extension + 1, c_sz_filter)) {
                            continue;
                        }
                    }
                }

                if (!on_file(st_path.c_str(), name)) {
                    result = false;
                    break;
                }
            }
        }

        closedir(dir);
        return result;
    }
}
