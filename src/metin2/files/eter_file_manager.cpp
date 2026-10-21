#include "eter_file_manager.h"

#include <metin2/log/log.h>

#include <cctype>
#include <filesystem>
#include <set>

namespace metin2::inline files {

    namespace {
        constexpr std::string_view kDefaultArchive = "System";
        constexpr std::string_view kDataDirectory = "data/";
    }

    auto get_file_manager() -> eter_file_manager & {
        static eter_file_manager manager;
        return manager;
    }

    auto eter_file_manager::parse_path(std::string_view path) -> eter_asset_path {
        std::string name(path);
        for (char &c : name)
            if (c == '\\')
                c = '/';

        // A drive prefix ("d:/ymir work/...") is not an archive name.
        if (name.size() >= 3 && std::isalpha(static_cast<unsigned char>(name[0])) && name[1] == ':' && name[2] == '/')
            name.erase(0, 3);

        eter_asset_path result;
        auto colon = name.find(':');
        if (colon == std::string::npos) {
            result.archive_value = kDefaultArchive;
            result.path = std::move(name);
        } else {
            result.archive_value = name.substr(0, colon);
            result.path = name.substr(colon + 1);
        }

        std::string &p = result.path;
        std::size_t start = 0;
        while (true) {
            if (p.compare(start, 2, "./") == 0)
                start += 2;
            else if (start < p.size() && p[start] == '/')
                start += 1;
            else
                break;
        }
        p.erase(0, start);

        std::size_t dup;
        while ((dup = p.find("//")) != std::string::npos)
            p.erase(dup, 1);

        return result;
    }

    auto eter_file_manager::archive::get(mapped_file &mapped_file, const std::string &path, const void **data) const -> bool {
        if (auto it = loose.find(path); it != loose.end() && mapped_file.create(it->second.c_str(), data, 0, 0))
            return true;

        return zip && zip->get(mapped_file, path, data);
    }

    auto eter_file_manager::archive::is_exist(const std::string &path) const -> bool {
        return loose.contains(path) || (zip && zip->contains(path));
    }

    auto eter_file_manager::load_archive(const std::string &archive_name) -> std::optional<archive> {
        archive archive_value;
        archive_value.name_value = archive_name;

        // The game names files with inconsistent case ("UIScript/PopupDialog.py" for
        // uiscript/popupdialog.py), so the folder is listed once and looked up without case.
        // (About 60 ms for the 53k files of data/System/; std::filesystem::relative would stat each.)
        std::error_code ec;
        const std::string root = std::string(kDataDirectory) + archive_name;
        for (auto it = std::filesystem::recursive_directory_iterator(root, ec); !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            if (!it->is_regular_file(ec))
                continue;
            const std::string &disk_path = it->path().native();
            archive_value.loose.emplace(disk_path.substr(root.size() + 1), disk_path);
        }

        std::string zip_path = std::string(kDataDirectory) + archive_name + ".zip";
        if (std::filesystem::is_regular_file(zip_path, ec)) {
            auto zip = std::make_unique<zip_archive>();
            if (zip->open(zip_path))
                archive_value.zip = std::move(zip);
        }

        if (!archive_value.zip && archive_value.loose.empty())
            return std::nullopt;

        return archive_value;
    }

    auto eter_file_manager::find_archive(const std::string &archive_name) -> const archive * {
        std::lock_guard<std::mutex> lock(mutex_value_);

        auto it = archives_.find(archive_name);
        if (it == archives_.end()) {
            auto loaded = load_archive(archive_name);
            if (!loaded)
                metin2::log::error("Archive {} not found (neither data/{}.zip nor data/{}/)", archive_name.c_str(), archive_name.c_str(), archive_name.c_str());

            // A missing archive is remembered as nullptr, so it is reported and probed only once.
            it = archives_.emplace(archive_name, loaded ? std::make_unique<archive>(std::move(*loaded)) : nullptr).first;
        }

        return it->second.get();
    }

    auto eter_file_manager::get(mapped_file &mapped_file, const char *filename, const void **data) -> bool {
        if (!filename || !*filename)
            return false;

        auto path = parse_path(filename);
        const archive *archive_value = find_archive(path.archive_value);
        return archive_value && archive_value->get(mapped_file, path.path, data);
    }

    auto eter_file_manager::is_exist(const char *filename) -> bool {
        if (!filename || !*filename)
            return false;

        auto path = parse_path(filename);
        const archive *archive_value = find_archive(path.archive_value);
        return archive_value && archive_value->is_exist(path.path);
    }

    void eter_file_manager::for_each(std::string_view archive_name, std::string_view prefix, const std::function<void(const std::string &)> &fn) {
        const archive *archive_value = find_archive(std::string(archive_name));
        if (!archive_value)
            return;

        std::set<std::string, std::less<>> seen;
        auto emit = [&](const std::string &name) -> void {
            std::string lower = name;
            for (char &c : lower)
                c = ascii_tolower(c);
            if (seen.insert(std::move(lower)).second)
                fn(name);
        };

        for (const auto &[name, disk_path] : archive_value->loose)
            if (assets::starts_with_no_case(name, prefix))
                emit(name);

        if (archive_value->zip)
            archive_value->zip->for_each(prefix, emit);
    }
}
