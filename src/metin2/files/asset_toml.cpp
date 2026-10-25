#include "asset_toml.h"

#include <metin2/log/log.h>

#include <limits>

namespace metin2::inline files {

    namespace {
        template <typename T>
        auto convert_integer(const toml::node &node, T &value) -> bool {
            const auto v = node.value_exact<int64_t>();
            if (!v || *v < std::numeric_limits<T>::min() || *v > std::numeric_limits<T>::max()) {
                return false;
            }
            value = static_cast<T>(*v);
            return true;
        }

        auto convert(const toml::node &node, float &value) -> bool {
            if (!node.is_number()) {
                return false;
            }
            value = static_cast<float>(*node.value<double>());
            return true;
        }
        auto convert(const toml::node &node, int &value) -> bool { return convert_integer(node, value); }
        auto convert(const toml::node &node, uint8_t &value) -> bool { return convert_integer(node, value); }
        auto convert(const toml::node &node, uint32_t &value) -> bool { return convert_integer(node, value); }
        auto convert(const toml::node &node, bool &value) -> bool {
            const auto v = node.value_exact<bool>();
            if (!v) {
                return false;
            }
            value = *v;
            return true;
        }
        auto convert(const toml::node &node, std::string &value) -> bool {
            const auto v = node.value_exact<std::string>();
            if (!v) {
                return false;
            }
            value = *v;
            return true;
        }
        auto convert(const toml::node &node, math::vector3 &value) -> bool {
            float v[3];
            if (!read_numbers(node, v)) {
                return false;
            }
            value = math::vector3(v[0], v[1], v[2]);
            return true;
        }
        auto convert(const toml::node &node, math::color &value) -> bool {
            float v[4];
            if (!read_numbers(node, v)) {
                return false;
            }
            value = math::color(v[0], v[1], v[2], v[3]);
            return true;
        }

        template <typename T>
        constexpr auto describe() -> std::string_view {
            if constexpr (std::is_same_v<T, float>) {
                return "a number";
            } else if constexpr (std::is_same_v<T, bool>) {
                return "true or false";
            } else if constexpr (std::is_same_v<T, std::string>) {
                return "a string";
            } else if constexpr (std::is_same_v<T, math::vector3>) {
                return "three numbers";
            } else if constexpr (std::is_same_v<T, math::color>) {
                return "four numbers";
            } else if constexpr (std::is_same_v<T, uint8_t>) {
                return "a whole number from 0 to 255";
            } else if constexpr (std::is_same_v<T, uint32_t>) {
                return "a whole number of at least 0";
            } else {
                return "a whole number";
            }
        }
    }

    auto parse_asset_toml(std::string_view text, std::string_view name, std::string_view type) -> std::optional<toml::table> {
        toml::parse_result parsed = toml::parse(text, name);
        if (!parsed) {
            const toml::parse_error &error = parsed.error();
            metin2::log::error("{}:{}: {}", name, error.source().begin.line, error.description());
            return std::nullopt;
        }

        toml::table table = std::move(parsed).table();
        const auto asset_type = table["asset"]["type"].value_exact<std::string>();
        if (!asset_type) {
            metin2::log::error("{}: [asset] type is missing; expected \"{}\"", name, type);
            return std::nullopt;
        }
        if (*asset_type != type) {
            metin2::log::error("{}: [asset] type is \"{}\"; expected \"{}\"", name, *asset_type, type);
            return std::nullopt;
        }
        return table;
    }

    auto read_numbers(const toml::node &node, std::span<float> values) -> bool {
        const toml::array *array = node.as_array();
        if (!array || array->size() != values.size()) {
            return false;
        }
        for (size_t i = 0; i < values.size(); ++i) {
            if (!(*array)[i].is_number()) {
                return false;
            }
            values[i] = static_cast<float>(*(*array)[i].value<double>());
        }
        return true;
    }

    asset_table::asset_table(const toml::table &table, std::string_view file_name) : table_(table), file_name_(file_name) {}

    auto asset_table::line() const -> uint32_t {
        return table_.source().begin.line;
    }

    auto asset_table::find(std::string_view key) -> const toml::node * {
        read_.emplace(key);
        return table_.get(key);
    }

    void asset_table::error(std::string_view key, std::string_view message) {
        const toml::node *node = key.empty() ? nullptr : table_.get(key);
        const uint32_t at = node ? node->source().begin.line : line();
        if (key.empty()) {
            metin2::log::error("{}:{}: {}", file_name_, at, message);
        } else {
            metin2::log::error("{}:{}: {} {}", file_name_, at, key, message);
        }
        ok_ = false;
    }

    template <typename T>
    auto asset_table::read(std::string_view key, T &value, bool required) -> bool {
        const toml::node *node = find(key);
        if (!node) {
            if (required) {
                error(key, "is missing");
                return false;
            }
            return true;
        }
        if (!convert(*node, value)) {
            error(key, std::string("must be ") + std::string(describe<T>()));
            return false;
        }
        return true;
    }

    auto asset_table::required(std::string_view key, float &value) -> bool { return read(key, value, true); }
    auto asset_table::required(std::string_view key, int &value) -> bool { return read(key, value, true); }
    auto asset_table::required(std::string_view key, uint8_t &value) -> bool { return read(key, value, true); }
    auto asset_table::required(std::string_view key, uint32_t &value) -> bool { return read(key, value, true); }
    auto asset_table::required(std::string_view key, bool &value) -> bool { return read(key, value, true); }
    auto asset_table::required(std::string_view key, std::string &value) -> bool { return read(key, value, true); }
    auto asset_table::required(std::string_view key, math::vector3 &value) -> bool { return read(key, value, true); }
    auto asset_table::required(std::string_view key, math::color &value) -> bool { return read(key, value, true); }

    auto asset_table::optional(std::string_view key, float &value) -> bool { return read(key, value, false); }
    auto asset_table::optional(std::string_view key, int &value) -> bool { return read(key, value, false); }
    auto asset_table::optional(std::string_view key, uint8_t &value) -> bool { return read(key, value, false); }
    auto asset_table::optional(std::string_view key, uint32_t &value) -> bool { return read(key, value, false); }
    auto asset_table::optional(std::string_view key, bool &value) -> bool { return read(key, value, false); }
    auto asset_table::optional(std::string_view key, std::string &value) -> bool { return read(key, value, false); }
    auto asset_table::optional(std::string_view key, math::vector3 &value) -> bool { return read(key, value, false); }
    auto asset_table::optional(std::string_view key, math::color &value) -> bool { return read(key, value, false); }

    auto asset_table::optional_enum(std::string_view key, std::span<const std::string_view> names, int &value) -> bool {
        const toml::node *node = find(key);
        if (!node) {
            return true;
        }
        const auto name = node->value_exact<std::string>();
        for (size_t i = 0; name && i < names.size(); ++i) {
            if (!names[i].empty() && *name == names[i]) {
                value = static_cast<int>(i);
                return true;
            }
        }
        std::string message = "must be one of";
        for (size_t i = 0; i < names.size(); ++i) {
            if (!names[i].empty()) {
                message += i == 0 ? " \"" : ", \"";
                message += names[i];
                message += '"';
            }
        }
        error(key, message);
        return false;
    }

    auto asset_table::required_enum(std::string_view key, std::span<const std::string_view> names, int &value) -> bool {
        if (!table_.get(key)) {
            read_.emplace(key);
            error(key, "is missing");
            return false;
        }
        return optional_enum(key, names, value);
    }

    auto asset_table::array(std::string_view key) -> const toml::array * {
        const toml::node *node = find(key);
        if (node && !node->is_array()) {
            error(key, "must be an array");
            return nullptr;
        }
        return node ? node->as_array() : nullptr;
    }

    auto asset_table::table(std::string_view key) -> const toml::table * {
        const toml::node *node = find(key);
        if (node && !node->is_table()) {
            error(key, "must be a table");
            return nullptr;
        }
        return node ? node->as_table() : nullptr;
    }

    auto asset_table::finish() -> bool {
        for (const auto &[key, node] : table_) {
            if (!read_.contains(key.str())) {
                metin2::log::warn("{}:{}: unknown key {}", file_name_, node.source().begin.line, key.str());
            }
        }
        return ok_;
    }
}
