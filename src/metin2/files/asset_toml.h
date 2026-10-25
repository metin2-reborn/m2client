#pragma once

#include <cstdint>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>

#include <metin2/math/color.h>
#include <metin2/math/vector.h>

#include <toml++/toml.hpp>

namespace metin2::inline files {

    // Parses a TOML asset file and checks its kind, the type in its [asset] table ("Motion",
    // "Effect", "TextureSet"). Errors are logged with the file name and line; nullopt when the text
    // is not TOML or not an asset of that type.
    auto parse_asset_toml(std::string_view text, std::string_view name, std::string_view type) -> std::optional<toml::table>;

    // The type in the [asset] table of a TOML asset file; nullopt when the text is not TOML or has none.
    auto asset_type(std::string_view text) -> std::optional<std::string>;

    // Reads the values of one table of an asset file. A missing optional value keeps the default the
    // caller put in it; a missing required value or a value of the wrong type is logged with the file
    // name and line and makes finish() false. finish() also warns about the keys nothing read.
    class asset_table {
      public:
        asset_table(const toml::table &table, std::string_view file_name);

        auto line() const -> uint32_t;
        auto file_name() const -> std::string_view { return file_name_; }

        auto required(std::string_view key, float &value) -> bool;
        auto required(std::string_view key, int &value) -> bool;
        auto required(std::string_view key, uint8_t &value) -> bool;
        auto required(std::string_view key, uint32_t &value) -> bool;
        auto required(std::string_view key, bool &value) -> bool;
        auto required(std::string_view key, std::string &value) -> bool;
        auto required(std::string_view key, math::vector3 &value) -> bool;
        auto required(std::string_view key, math::color &value) -> bool;

        auto optional(std::string_view key, float &value) -> bool;
        auto optional(std::string_view key, int &value) -> bool;
        auto optional(std::string_view key, uint8_t &value) -> bool;
        auto optional(std::string_view key, uint32_t &value) -> bool;
        auto optional(std::string_view key, bool &value) -> bool;
        auto optional(std::string_view key, std::string &value) -> bool;
        auto optional(std::string_view key, math::vector3 &value) -> bool;
        auto optional(std::string_view key, math::color &value) -> bool;

        // A name out of names, read as its index.
        auto required_enum(std::string_view key, std::span<const std::string_view> names, int &value) -> bool;
        auto optional_enum(std::string_view key, std::span<const std::string_view> names, int &value) -> bool;

        // Raw access for values with a shape of their own; nullptr when the key is missing (and an
        // error when it has another type).
        auto array(std::string_view key) -> const toml::array *;
        auto table(std::string_view key) -> const toml::table *;

        // Logs an error about key (or the table itself when key is empty) and makes finish() false.
        void error(std::string_view key, std::string_view message);

        auto finish() -> bool;

      private:
        auto find(std::string_view key) -> const toml::node *;
        template <typename T>
        auto read(std::string_view key, T &value, bool required) -> bool;

        const toml::table &table_;
        std::string_view file_name_;
        std::set<std::string, std::less<>> read_;
        bool ok_ = true;
    };

    // The numbers of a TOML array of exactly values.size() numbers; false otherwise.
    auto read_numbers(const toml::node &node, std::span<float> values) -> bool;
}
