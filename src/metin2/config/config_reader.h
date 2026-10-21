#pragma once

#include <metin2/log/log.h>

#include <toml++/toml.hpp>

#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace metin2::config {
    // metin2.toml is edited by players: a key with a bad value is reported and keeps its default,
    // the rest of the file still applies.
    class reader {
      public:
        reader(const toml::table &root, std::string file) : root_(root), file_(std::move(file)) {}

        template <typename T>
        void read(std::string_view section, std::string_view key, T &out) {
            known_.insert(std::string(section) + "." + std::string(key));
            const toml::node *node = root_[section][key].node();
            if (!node)
                return;

            if constexpr (std::is_same_v<T, bool>) {
                if (const auto value = node->value_exact<bool>())
                    out = *value;
                else
                    bad(*node, section, key, "true or false");
            } else if constexpr (std::is_integral_v<T>) {
                const auto value = node->value_exact<int64_t>();
                if (value && std::in_range<T>(*value))
                    out = static_cast<T>(*value);
                else
                    bad(*node, section, key, "a whole number in range");
            } else if constexpr (std::is_floating_point_v<T>) {
                if (const auto value = node->value<double>())
                    out = static_cast<T>(*value);
                else
                    bad(*node, section, key, "a number");
            } else {
                if (const auto value = node->value_exact<std::string>())
                    out = *value;
                else
                    bad(*node, section, key, "a string");
            }
        }

        void report_unknown_keys() const {
            for (const auto &[section, node] : root_) {
                const toml::table *table = node.as_table();
                if (!table) {
                    metin2::log::warn("{}:{}: unknown key {}", file_, node.source().begin.line, section.str());
                    continue;
                }
                for (const auto &[key, value] : *table) {
                    if (!known_.contains(std::string(section.str()) + "." + std::string(key.str())))
                        metin2::log::warn("{}:{}: unknown key {}.{}", file_, value.source().begin.line, section.str(), key.str());
                }
            }
        }

      private:
        void bad(const toml::node &node, std::string_view section, std::string_view key, std::string_view expected) const {
            metin2::log::warn("{}:{}: {}.{} must be {}; the default is used", file_, node.source().begin.line, section, key, expected);
        }

        const toml::table &root_;
        std::string file_;
        std::set<std::string> known_;
    };
}
