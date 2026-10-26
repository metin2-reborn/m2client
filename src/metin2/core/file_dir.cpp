#include "file_dir.h"

#include "string_case.h"

#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace metin2::inline core {

    auto dir::create(const char *c_sz_filter, const char *c_sz_path, bool checked_extension) -> bool {
        std::string st_path = c_sz_path;

        if (!st_path.empty()) {
            char end = st_path[st_path.length() - 1];

            if (end != '/' && end != '\\') {
                st_path += '/';
            }
        }

        std::error_code error;
        std::filesystem::directory_iterator entries(st_path.empty() ? "." : st_path, error);
        if (error) {
            return true; // like FindFirstFile failing: nothing to visit
        }

        for (const std::filesystem::directory_entry &entry : entries) {
            const std::string name = entry.path().filename().string();

            if (name.empty() || name[0] == '.') {
                continue;
            }

            if (entry.is_directory(error)) {
                if (!on_folder(c_sz_filter, st_path.c_str(), name.c_str())) {
                    return false;
                }
            } else {
                const std::string::size_type dot = name.find('.');
                if (dot == std::string::npos) {
                    continue;
                }
                const std::string_view extension(name.c_str() + dot + 1);

                // TODO: checked_extension should always be on, once every caller passes the filter as bare
                // extensions ("wav;gr2").
                if (checked_extension) {
                    const std::string_view filter = c_sz_filter;
                    const std::string_view::size_type pos = filter.find(';');
                    if (pos != std::string_view::npos && pos > 0) {
                        if (filter.substr(0, pos) != extension && filter.substr(pos + 1) != extension) {
                            continue;
                        }
                    } else if (!iequals(extension, filter)) {
                        continue;
                    }
                }

                if (!on_file(st_path.c_str(), name.c_str())) {
                    return false;
                }
            }
        }

        return true;
    }
}
