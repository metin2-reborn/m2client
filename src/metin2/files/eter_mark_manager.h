#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace metin2::inline core {
    class mapped_file;
}

namespace metin2::inline files {

    // Guild marks and symbols are downloaded from the server at run time, so they live in the user's
    // profile (~/.metin2/mark) instead of the client directory, and never go through the file
    // manager. The game keeps calling them "mark/<file>"; path() maps such a name to the profile.
    class eter_mark_manager {
      public:
        static auto is_mark_path(std::string_view name) -> bool;

        // Absolute path for a "mark/..." name; creates the parent directory so it can be written.
        auto path(std::string_view name) -> std::string;

        auto get(mapped_file &mapped_file, const char *filename, const void **data) -> bool;
        auto is_exist(const char *filename) -> bool;

      private:
        auto root() -> const std::filesystem::path &;

        std::filesystem::path root_;
    };

    auto get_mark_manager() -> eter_mark_manager &;
}
