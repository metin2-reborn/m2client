#pragma once

#include "asset_name.h"

#include <metin2/core/mapped_file.h>

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace metin2::inline files {

    // Read-only zip archive (stored and deflate entries, zip64 aware). The archive is memory-mapped
    // once; stored entries are handed out without copying, deflated ones are inflated per request.
    class zip_archive {
      public:
        zip_archive() = default;
        ~zip_archive();
        zip_archive(const zip_archive &) = delete;
        auto operator=(const zip_archive &) -> zip_archive & = delete;

        auto open(const std::string &path) -> bool;

        auto get(mapped_file &mapped_file, std::string_view name, const void **data) const -> bool;
        auto contains(std::string_view name) const -> bool;

        // Calls fn for every file whose name starts with prefix (case-insensitive).
        void for_each(std::string_view prefix, const std::function<void(const std::string &)> &fn) const;

      private:
        struct entry {
            std::uint64_t local_header = 0;
            std::uint64_t compressed_size = 0;
            std::uint64_t size = 0;
            std::uint32_t crc = 0;
            std::uint16_t method = 0;
            std::uint16_t flags_value = 0;
        };

        auto read_central_directory() -> bool;

        std::string path_;
        core::mapped_file map_;
        const std::uint8_t *data_value_ = nullptr;
        std::size_t size_ = 0;
        std::unordered_map<std::string, entry, assets::case_insensitive_hash, assets::case_insensitive_equal> entries_;
    };
}
