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

    // Resolves asset names to files. A name is "<archive>:<path>", or just "<path>" for the system
    // archive. A loose file data/<archive>/<path> wins over the entry in data/<archive>.zip. Names are
    // case-insensitive, may use backslashes, and a drive prefix ("d:/ymir work/...") is ignored. Names
    // from before the ymir work split still resolve (see canonical_name).
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

        // The current name of an asset, lower case with '/': names into the folders that left
        // system:ymir work become names of their archive ("d:/ymir work/pc/x.dds" is
        // "character:pc/x.png"), and .msa/.mse scripts are .toml. Other system names keep their
        // spelling. Two names of one file give the same canonical name.
        static auto canonical_name(std::string_view name) -> std::string;

        // The sound script (.mss) of a motion or effect script: "character:pc/warrior/wait.toml" has
        // "sound/pc/warrior/wait.mss", "effect:etc/x.toml" has "sound/effect/etc/x.mss". Empty for
        // scripts outside the split archives, which have no sound scripts.
        static auto sound_script_name(std::string_view name) -> std::string;

      private:
        auto find_archive(const std::string &archive_name) -> const archive *;
        auto load_archive(const std::string &archive_name) -> std::optional<archive>;

        std::mutex mutex_value_; // guards archives_ (files are read from the loader thread as well)
        archive_map archives_;
    };

    auto get_file_manager() -> eter_file_manager &;
}
