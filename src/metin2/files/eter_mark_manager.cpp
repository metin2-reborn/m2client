#include "eter_mark_manager.h"

#include "asset_name.h"

#include <metin2/config/state_dir.h>
#include <metin2/core/mapped_file.h>
#include <metin2/log/log.h>

#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace metin2::inline files {

    namespace {
        constexpr std::string_view kMarkPrefix = "mark/";

        auto normalize(std::string_view name) -> std::string {
            std::string s(name);
            for (char &c : s)
                if (c == '\\')
                    c = '/';
            return s;
        }
    }

    auto get_mark_manager() -> eter_mark_manager & {
        static eter_mark_manager manager;
        return manager;
    }

    auto eter_mark_manager::is_mark_path(std::string_view name) -> bool {
        return assets::starts_with_no_case(normalize(name.substr(0, kMarkPrefix.size())), kMarkPrefix);
    }

    auto eter_mark_manager::root() -> const std::filesystem::path & {
        if (root_.empty())
            root_ = metin2::state::path("mark", true);
        return root_;
    }

    auto eter_mark_manager::path(std::string_view name) -> std::string {
        std::string relative = normalize(name);
        if (is_mark_path(relative))
            relative.erase(0, kMarkPrefix.size());

        std::filesystem::path path = root() / relative;
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        return path.string();
    }

    auto eter_mark_manager::get(mapped_file &mapped_file, const char *filename, const void **data) -> bool {
        const std::string full_path = path(filename);
        return is_exist(filename) && mapped_file.create(full_path.c_str(), data, 0, 0);
    }

    auto eter_mark_manager::is_exist(const char *filename) -> bool {
        std::error_code error;
        return std::filesystem::exists(path(filename), error);
    }
}
