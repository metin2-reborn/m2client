#include "grp_particles.h"
#include <metin2/files/asset_toml.h>
#include <metin2/math/color.h>
#include <metin2/math/vector.h>
#include "resource_manager.h"

#include <algorithm>
#include <array>

namespace metin2::inline graphics {

    namespace {
        constexpr std::array<std::string_view, 2> blend_names = {"alpha", "add"};

        // A [min, max] range; min may not exceed max.
        auto read_range(asset_table &values, std::string_view key, float &min, float &max, bool required) -> bool {
            math::vector2 range = {min, max};
            const bool ok = required ? values.required(key, range) : values.optional(key, range);
            if (!ok) {
                return false;
            }
            if (range.x > range.y) {
                values.error(key, "must be [min, max] with min not above max");
                return false;
            }
            min = range.x;
            max = range.y;
            return true;
        }

        auto read_pair(asset_table &values, std::string_view key, float &x, float &y) -> bool {
            math::vector2 pair = {x, y};
            if (!values.optional(key, pair)) {
                return false;
            }
            x = pair.x;
            y = pair.y;
            return true;
        }

        auto read_color(asset_table &values, std::string_view key, float (&out)[4], bool required) -> bool {
            math::color color(out[0], out[1], out[2], out[3]);
            const bool ok = required ? values.required(key, color) : values.optional(key, color);
            if (!ok) {
                return false;
            }
            out[0] = color.r;
            out[1] = color.g;
            out[2] = color.b;
            out[3] = color.a;
            return true;
        }

        auto not_negative(asset_table &values, std::string_view key, float value) -> bool {
            if (value < 0.0f) {
                values.error(key, "must not be negative");
                return false;
            }
            return true;
        }
    }

    auto particle_settings::load(std::string_view text, std::string_view name, particle_settings &settings) -> bool {
        const auto table = parse_asset_toml(text, name, "Particles");
        if (!table) {
            return false;
        }

        asset_table root(*table, name);
        root.table("asset");
        const toml::table *particles = root.table("particles");
        if (!particles) {
            root.error("particles", "is missing");
            return false;
        }

        particle_settings read;
        asset_table values(*particles, name);
        bool ok = values.required("image", read.image);
        ok &= values.optional_enum("blend", blend_names, read.blend);
        ok &= values.required("rate", read.rate) && not_negative(values, "rate", read.rate);
        ok &= read_range(values, "lifetime", read.lifetime_min, read.lifetime_max, true);
        if (read.lifetime_min <= 0.0f) {
            values.error("lifetime", "must be positive");
            ok = false;
        }
        ok &= read_pair(values, "origin", read.origin_x, read.origin_y);
        ok &= read_pair(values, "spread", read.spread_x, read.spread_y);
        ok &= values.optional("fill", read.fill);
        ok &= read_pair(values, "velocity", read.velocity_x, read.velocity_y);
        ok &= read_pair(values, "velocity_spread", read.velocity_spread_x, read.velocity_spread_y);
        ok &= read_pair(values, "acceleration", read.acceleration_x, read.acceleration_y);
        ok &= values.optional("drag", read.drag) && not_negative(values, "drag", read.drag);
        ok &= values.optional("turbulence", read.turbulence) && not_negative(values, "turbulence", read.turbulence);
        ok &= read_range(values, "size", read.size_min, read.size_max, true);
        if (read.size_min <= 0.0f) {
            values.error("size", "must be positive");
            ok = false;
        }
        ok &= values.optional("size_end", read.size_end) && not_negative(values, "size_end", read.size_end);
        if (read_color(values, "color", read.color, false)) {
            std::ranges::copy(read.color, std::begin(read.color_end));
        } else {
            ok = false;
        }
        ok &= read_color(values, "color_end", read.color_end, false);
        ok &= values.optional("fade_in", read.fade_in) && not_negative(values, "fade_in", read.fade_in);
        ok &= values.optional("fade_out", read.fade_out) && not_negative(values, "fade_out", read.fade_out);
        ok &= read_range(values, "rotation", read.rotation_min, read.rotation_max, false);
        ok &= read_range(values, "spin", read.spin_min, read.spin_max, false);
        ok &= values.optional("prewarm", read.prewarm);
        ok &= values.finish();
        if (!root.finish() || !ok) {
            return false;
        }
        settings = read;
        return true;
    }

    auto graphic_particles::type() -> type_type {
        static type_type s_type = string_to_type("CGraphicParticles");
        return s_type;
    }

    graphic_particles::graphic_particles(const char *file_name) : resource(file_name) {
    }

    graphic_particles::~graphic_particles() = default;

    auto graphic_particles::on_load(int size, const void *buf) -> bool {
        if (!buf) {
            return false;
        }
        if (!particle_settings::load(std::string_view(static_cast<const char *>(buf), size), file_name_string(), settings_)) {
            return false;
        }

        resource *image = get_resource_manager().resource_pointer(settings_.image.c_str());
        if (!image || !image->is_type(graphic_image::type())) {
            return false;
        }
        image_ = image;
        if (image_->is_empty()) {
            image_.clear();
            return false;
        }
        loaded_ = true;
        return true;
    }

    void graphic_particles::on_clear() {
        settings_ = {};
        image_.clear();
        loaded_ = false;
    }

    auto graphic_particles::on_is_empty() const -> bool {
        return !loaded_;
    }

    auto graphic_particles::on_is_type(type_type type) -> bool {
        if (graphic_particles::type() == type) {
            return true;
        }
        return resource::on_is_type(type);
    }
}
