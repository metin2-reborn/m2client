#include "font_family.h"

#include <metin2/files/asset_toml.h>
#include <metin2/log/log.h>

#include <algorithm>
#include <cstdlib>

namespace metin2::inline graphics {

    namespace {
        auto read_weights(asset_table &style, std::string_view name, std::vector<font_family::weight_file> &out) -> bool {
            const toml::array *weights = style.array("weights");
            if (!weights) {
                style.error("weights", "is missing");
                return false;
            }
            bool ok = true;
            for (const toml::node &node : *weights) {
                const toml::table *entry = node.as_table();
                if (!entry) {
                    style.error("weights", "must be an array of tables");
                    ok = false;
                    continue;
                }
                asset_table values(*entry, name);
                font_family::weight_file file;
                values.required("weight", file.weight);
                values.required("path", file.path);
                if (!values.finish()) {
                    ok = false;
                    continue;
                }
                if (file.weight < 1 || file.weight > 1000) {
                    values.error("weight", "must be between 1 and 1000");
                    ok = false;
                    continue;
                }
                if (file.path.empty()) {
                    values.error("path", "must not be empty");
                    ok = false;
                    continue;
                }
                if (std::ranges::any_of(out, [&](const font_family::weight_file &other) -> bool { return other.weight == file.weight; })) {
                    values.error("weight", "is listed twice in this style");
                    ok = false;
                    continue;
                }
                out.push_back(std::move(file));
            }
            if (out.empty() && ok) {
                style.error("weights", "has no entries");
                ok = false;
            }
            std::ranges::sort(out, {}, &font_family::weight_file::weight);
            return ok && style.finish();
        }
    }

    auto font_family::type() -> type_type {
        static type_type s_type = string_to_type("CFontFamily");
        return s_type;
    }

    auto font_family::parse(std::string_view text, std::string_view name, table &out) -> bool {
        out = table();
        const auto parsed = parse_asset_toml(text, name, "Font");
        if (!parsed) {
            return false;
        }

        asset_table root(*parsed, name);
        root.table("asset");
        bool ok = true;
        if (const toml::table *normal = root.table("normal")) {
            asset_table style(*normal, name);
            ok = read_weights(style, name, out.normal) && ok;
        } else {
            root.error("normal", "is missing");
            ok = false;
        }
        if (const toml::table *italic = root.table("italic")) {
            asset_table style(*italic, name);
            ok = read_weights(style, name, out.italic) && ok;
        }
        return root.finish() && ok;
    }

    auto font_family::file_for(const table &files, bool italic, int weight) -> const std::string & {
        static const std::string none;
        const std::vector<weight_file> &style = italic && !files.italic.empty() ? files.italic : files.normal;
        const weight_file *best = nullptr;
        for (const weight_file &file : style) {
            // The closest weight; of two equally close the heavier, which the sort puts later.
            if (!best || std::abs(file.weight - weight) <= std::abs(best->weight - weight)) {
                best = &file;
            }
        }
        return best ? best->path : none;
    }

    font_family::font_family(const char *file_name) : resource(file_name) {
    }

    auto font_family::on_load(int size, const void *buf) -> bool {
        if (!buf || size <= 0) {
            metin2::log::error("Font {} not found", file_name());
            return false;
        }
        return parse(std::string_view(static_cast<const char *>(buf), static_cast<size_t>(size)), file_name(), files_);
    }

    void font_family::on_clear() {
        files_ = table();
    }

    auto font_family::on_is_empty() const -> bool {
        return files_.normal.empty();
    }

    auto font_family::on_is_type(type_type type) -> bool {
        if (font_family::type() == type) {
            return true;
        }
        return resource::on_is_type(type);
    }
}
