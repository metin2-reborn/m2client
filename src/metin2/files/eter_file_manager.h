#pragma once

#include "asset_name.h"
#include "zip_archive.h"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <metin2/core/mapped_file.h>

namespace metin2::inline files {

    // Resolves asset names to files. A name is "<Archive>:<path>", or just "<path>" for the System
    // archive. A loose file data/<Archive>/<path> wins over the entry in data/<Archive>.zip. Names are
    // case-insensitive, may use backslashes, and a drive prefix ("d:/ymir work/...") is ignored.
    class eter_file_manager {
        // The loose files of an archive: path inside the archive (any case) -> path on disk.
        using loose_file_map = std::unordered_map<
            std::string, std::string,
            assets::case_insensitive_hash,
            assets::case_insensitive_equal>;

        // An archive as found on disk: the zip (if any) and the files of the loose-file folder,
        // listed once when the archive is first used.
        struct archive {
            std::string name_value;
            loose_file_map loose;
            std::unique_ptr<zip_archive> zip;

            auto get(mapped_file &mapped_file, const std::string &path, const void **data) const -> bool;
            auto is_exist(const std::string &path) const -> bool;
        };

        using archive_map = std::unordered_map<
            std::string, std::unique_ptr<archive>,
            assets::case_insensitive_hash,
            assets::case_insensitive_equal>;

        struct eter_asset_path {
            std::string archive_value;
            std::string path;
        };

      public:
        auto get(mapped_file &mapped_file, const char *filename, const void **data) -> bool;
        auto is_exist(const char *filename) -> bool;

        // Calls fn with the path of every file of the archive whose path starts with prefix: zip
        // entries and loose files under data/<Archive>/, each name once.
        void for_each(std::string_view archive_value, std::string_view prefix, const std::function<void(const std::string &)> &fn);

        static auto parse_path(std::string_view path) -> eter_asset_path;

      private:
        auto find_archive(const std::string &archive_name) -> const archive *;
        auto load_archive(const std::string &archive_name) -> std::optional<archive>;

        std::mutex mutex_value_; // guards archives_ (files are read from the loader thread as well)
        archive_map archives_;
    };

    auto get_file_manager() -> eter_file_manager &;
}
