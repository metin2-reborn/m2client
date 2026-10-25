#include "eter_file_manager.h"

#include <metin2/log/log.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <set>
#include <span>

namespace metin2::inline files {

    namespace {
        constexpr std::string_view kDefaultArchive = "system";
        constexpr std::string_view kDataDirectory = "data/";

        // Folders of system:ymir work that became archives of their own.
        constexpr std::array<std::string_view, 8> kMovedToArchive = {"effect", "environment", "item", "special", "zone", "guild", "tree", "ui"};
        constexpr std::array<std::string_view, 6> kMovedToCharacter = {"monster", "monster2", "npc", "npc2", "pc", "pc2"};
        constexpr std::string_view kCharacterArchive = "character";
        // system:ymir work/uiloading became ui:loading.
        constexpr std::string_view kUiLoadingFolder = "uiloading";
        // Top-level folders of the system archive that became archives of their own.
        constexpr std::array<std::string_view, 3> kTopLevelMoved = {"season1", "season2", "icon"};
        // Script formats that became TOML.
        constexpr std::array<std::string_view, 4> kScriptExtensions = {".msa", ".mse", ".msm", ".sub"};
        // Archives whose textures were converted from DDS to PNG.
        constexpr std::array<std::string_view, 6> kPngArchives = {"effect", "environment", "item", "special", "zone", "character"};

        auto is_one_of(std::string_view name, std::span<const std::string_view> names) -> bool {
            return std::ranges::any_of(names, [name](std::string_view n) -> bool { return assets::case_insensitive_equal{}(name, n); });
        }

        auto ends_with_no_case(std::string_view s, std::string_view suffix) -> bool {
            return s.size() >= suffix.size() && assets::case_insensitive_equal{}(s.substr(s.size() - suffix.size()), suffix);
        }

        auto is_script(std::string_view path) -> bool {
            return std::ranges::any_of(kScriptExtensions, [path](std::string_view extension) -> bool { return ends_with_no_case(path, extension); });
        }

        void replace_extension(std::string &path, std::string_view extension) {
            path.replace(path.rfind('.'), std::string::npos, extension);
        }

        // The disk spelling of data/<name><suffix>. The archive folders are lower case, but a name
        // may spell the archive in any case ("Sound:..."), like the rest of the path.
        auto disk_name(const std::string &name, std::string_view suffix) -> std::string {
            std::string wanted = name + std::string(suffix);
            std::error_code ec;
            if (std::filesystem::exists(std::string(kDataDirectory) + wanted, ec)) {
                return wanted;
            }
            for (auto it = std::filesystem::directory_iterator(kDataDirectory, ec); !ec && it != std::filesystem::directory_iterator(); it.increment(ec)) {
                const std::string entry = it->path().filename().native();
                if (assets::case_insensitive_equal{}(entry, wanted)) {
                    return entry;
                }
            }
            return wanted;
        }
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

        // Names from before the system split: "ymir work/pc/x" is character:pc/x, "ymir work/effect/x"
        // is effect:x and "season1/x" is season1:x. The models still carry such names for their textures.
        constexpr std::string_view legacy_root = "ymir work/";
        if (assets::case_insensitive_equal{}(result.archive_value, kDefaultArchive) && assets::starts_with_no_case(p, legacy_root)) {
            const std::size_t slash = p.find('/', legacy_root.size());
            const std::string_view folder = std::string_view(p).substr(legacy_root.size(), slash == std::string::npos ? std::string_view::npos : slash - legacy_root.size());
            if (slash != std::string::npos && is_one_of(folder, kMovedToArchive)) {
                result.archive_value = folder;
                p.erase(0, slash + 1);
            } else if (slash != std::string::npos && assets::case_insensitive_equal{}(folder, kUiLoadingFolder)) {
                result.archive_value = "ui";
                p.replace(0, slash, "loading");
            } else if (slash != std::string::npos && is_one_of(folder, kMovedToCharacter)) {
                result.archive_value = kCharacterArchive;
                p.erase(0, legacy_root.size());
            }
        } else if (assets::case_insensitive_equal{}(result.archive_value, kDefaultArchive)) {
            const std::size_t slash = p.find('/');
            if (slash != std::string::npos && is_one_of(std::string_view(p).substr(0, slash), kTopLevelMoved)) {
                result.archive_value = p.substr(0, slash);
                p.erase(0, slash + 1);
            }
        }

        // The text scripts became TOML, the textures of the ymir work archives PNG and the icons PNG.
        if (is_script(p)) {
            replace_extension(p, ".toml");
        } else if (ends_with_no_case(p, ".dds") && is_one_of(result.archive_value, kPngArchives)) {
            replace_extension(p, ".png");
        } else if (ends_with_no_case(p, ".tga") && assets::case_insensitive_equal{}(result.archive_value, "icon")) {
            replace_extension(p, ".png");
        }

        return result;
    }

    auto eter_file_manager::canonical_name(std::string_view name) -> std::string {
        auto path = parse_path(name);
        std::string result;
        if (assets::case_insensitive_equal{}(path.archive_value, kDefaultArchive)) {
            // System names keep their spelling (drive prefix included), so the resource names they give stay the same.
            result = name;
            if (is_script(result)) {
                replace_extension(result, ".toml");
            }
        } else {
            result = path.archive_value + ":" + path.path;
        }

        for (char &c : result) {
            c = c == '\\' ? '/' : ascii_tolower(c);
        }
        return result;
    }

    auto eter_file_manager::sound_script_name(std::string_view name) -> std::string {
        auto path = parse_path(name);
        std::string folder;
        if (is_one_of(path.archive_value, kMovedToArchive)) {
            folder = path.archive_value + "/";
        } else if (!assets::case_insensitive_equal{}(path.archive_value, kCharacterArchive)) {
            return {};
        }

        std::string result = "sound:" + folder + path.path;
        const std::size_t dot = result.rfind('.');
        if (dot != std::string::npos && result.find('/', dot) == std::string::npos) {
            result.erase(dot);
        }
        result += ".toml";
        for (char &c : result) {
            c = ascii_tolower(c);
        }
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
        // (About 60 ms for the 53k files of data/system/; std::filesystem::relative would stat each.)
        std::error_code ec;
        const std::string root = std::string(kDataDirectory) + disk_name(archive_name, "");
        for (auto it = std::filesystem::recursive_directory_iterator(root, ec); !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            if (!it->is_regular_file(ec))
                continue;
            const std::string &disk_path = it->path().native();
            archive_value.loose.emplace(disk_path.substr(root.size() + 1), disk_path);
        }

        std::string zip_path = std::string(kDataDirectory) + disk_name(archive_name, ".zip");
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
