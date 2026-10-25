#include "map_util.h"
#include <cfloat>
#include <metin2/core/mapped_file.h>
#include <metin2/files/asset_toml.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/gpu/types.h>

#include <iterator>
#include <utility>

namespace metin2::inline world::inline maps {

    void environment_init(environment_data &env_data) {
        for (int i = 0; i < ENV_DIRLIGHT_NUM; ++i) {
            env_data.dir_lights_enable[i] = false;
            env_data.dir_lights[i].type = gpu::light_type::DIRECTIONAL;
            env_data.dir_lights[i].direction = math::vector3(0.5f, 0.5f, -0.5f);
            env_data.dir_lights[i].position = math::vector3(0.0f, 0.0f, 0.0f);
            env_data.dir_lights[i].specular = math::color(1.0f, 1.0f, 1.0f, 1.0f);
            env_data.dir_lights[i].diffuse = math::color(1.0f, 1.0f, 1.0f, 1.0f);
            env_data.dir_lights[i].ambient = math::color(0.5f, 0.5f, 0.5f, 1.0f);
            env_data.dir_lights[i].range = 0.0f;
            env_data.dir_lights[i].falloff = 1.0f;
            env_data.dir_lights[i].theta = 0.0f;
            env_data.dir_lights[i].phi = 0.0f;
            env_data.dir_lights[i].attenuation0 = 0.0f;
            env_data.dir_lights[i].attenuation1 = 1.0f;
            env_data.dir_lights[i].attenuation2 = 0.0f;
        }

        env_data.material_value.diffuse = math::color(0.8f, 0.8f, 0.8f, 1.0f);
        env_data.material_value.ambient = math::color(0.8f, 0.8f, 0.8f, 1.0f);
        env_data.material_value.emissive = math::color(0.8f, 0.8f, 0.8f, 1.0f);
        env_data.material_value.specular = math::color(0.0f, 0.0f, 0.0f, 1.0f);
        env_data.material_value.power = 0.0f;

        env_data.fog_enable = false;
        env_data.density_fog = false;
        env_data.fog_near_distance = 25600.0f * 0.5f;
        env_data.fog_far_distance = 25600.0f * 0.7f;
        env_data.fog_color = math::color(0.5f, 0.5f, 0.5f, 1.0f);

        env_data.filtering_enable = false;
        env_data.filtering_color = math::color(0.3f, 0.1f, 0.1f, 0.0f);
        env_data.filtering_alpha_src = gpu::blend::ONE;
        env_data.filtering_alpha_dest = gpu::blend::ONE;

        env_data.wind_strength = 0.2f;
        env_data.wind_random = 0.0f;

        env_data.sky_box_scale = math::vector3(3500.0f, 3500.0f, 3500.0f);
        env_data.sky_box_gradient_level_upper = 0;
        env_data.sky_box_gradient_level_lower = 0;
        env_data.sky_box_texture_render_mode = false;

        env_data.cloud_scale = math::vector2(200000.0f, 200000.0f);
        env_data.cloud_height = 30000.0f;
        env_data.cloud_texture_scale = math::vector2(4.0f, 4.0f);
        env_data.cloud_speed = math::vector2(0.001f, 0.001f);
        env_data.cloud_texture_file_name = "";
        env_data.cloud_gradient_color.first_color = .0f;
        env_data.cloud_gradient_color.second_color = .0f;

        env_data.sky_box_gradient_color_vector.clear();

        env_data.lens_flare_enable = false;
        env_data.lens_flare_brightness_color = math::color(1.0f, 1.0f, 1.0f, 1.0f);
        env_data.lens_flare_max_brightness = 1.0f;

        env_data.main_flare_enable = false;
        env_data.main_flare_texture_file_name = "";
        env_data.main_flare_size = 0.2f;

        env_data.reserve = false;
    }

    namespace {
        // A { first = [r, g, b, a], second = [r, g, b, a] } pair of sky colours.
        auto read_gradient_color(asset_table &owner, std::string_view key, const toml::node &node, gradient_color &color) -> bool {
            const toml::table *pair = node.as_table();
            if (!pair) {
                owner.error(key, "must be { first = [r, g, b, a], second = [r, g, b, a] } tables");
                return false;
            }
            asset_table values(*pair, owner.file_name());
            math::color first;
            math::color second;
            bool ok = values.required("first", first);
            ok &= values.required("second", second);
            color.first_color = {first.r, first.g, first.b, first.a};
            color.second_color = {second.r, second.g, second.b, second.a};
            return values.finish() && ok;
        }
    }

    auto environment_parse(environment_data &env_data, std::string_view text, std::string_view name) -> bool {
        const auto table = parse_asset_toml(text, name, "Environment");
        if (!table) {
            return false;
        }

        asset_table root(*table, name);
        root.table("asset");
        bool ok = root.optional("reserved", env_data.reserve);

        if (const toml::table *light_table = root.table("directional_light")) {
            asset_table light(*light_table, name);
            math::vector3 direction = env_data.dir_lights[ENV_DIRLIGHT_BACKGROUND].direction;
            ok &= light.optional("direction", direction);
            constexpr std::pair<std::string_view, int> groups[] = {
                {"background", ENV_DIRLIGHT_BACKGROUND},
                {"character",  ENV_DIRLIGHT_CHARACTER }
            };
            for (const auto &[key, index] : groups) {
                if (const toml::table *group = light.table(key)) {
                    asset_table values(*group, name);
                    env_data.dir_lights[index].direction = direction;
                    ok &= values.optional("enable", env_data.dir_lights_enable[index]);
                    ok &= values.optional("diffuse", env_data.dir_lights[index].diffuse);
                    ok &= values.optional("ambient", env_data.dir_lights[index].ambient);
                    ok &= values.finish();
                }
            }
            ok &= light.finish();
        }

        if (const toml::table *material_table = root.table("material")) {
            asset_table values(*material_table, name);
            ok &= values.optional("diffuse", env_data.material_value.diffuse);
            ok &= values.optional("ambient", env_data.material_value.ambient);
            ok &= values.optional("emissive", env_data.material_value.emissive);
            ok &= values.finish();
        }

        if (const toml::table *fog_table = root.table("fog")) {
            asset_table values(*fog_table, name);
            ok &= values.optional("enable", env_data.fog_enable);
            ok &= values.optional("density", env_data.density_fog);
            ok &= values.optional("near", env_data.fog_near_distance);
            ok &= values.optional("far", env_data.fog_far_distance);
            ok &= values.optional("color", env_data.fog_color);
            ok &= values.finish();
        }

        if (const toml::table *filter_table = root.table("filter")) {
            asset_table values(*filter_table, name);
            ok &= values.optional("enable", env_data.filtering_enable);
            ok &= values.optional("color", env_data.filtering_color);
            ok &= values.optional("alpha_src", env_data.filtering_alpha_src);
            ok &= values.optional("alpha_dest", env_data.filtering_alpha_dest);
            ok &= values.finish();
        }

        if (const toml::table *sky_table = root.table("sky_box")) {
            asset_table values(*sky_table, name);
            ok &= values.optional("texture_render_mode", env_data.sky_box_texture_render_mode);
            ok &= values.optional("scale", env_data.sky_box_scale);
            ok &= values.optional("gradient_upper", env_data.sky_box_gradient_level_upper);
            ok &= values.optional("gradient_lower", env_data.sky_box_gradient_level_lower);
            if (const toml::table *faces_table = values.table("faces")) {
                asset_table faces(*faces_table, name);
                constexpr std::string_view face_names[] = {"front", "back", "left", "right", "top", "bottom"};
                for (size_t i = 0; i < std::size(face_names); ++i) {
                    ok &= faces.optional(face_names[i], env_data.sky_box_face_file_name[i]);
                }
                ok &= faces.finish();
            }
            ok &= values.optional("cloud_scale", env_data.cloud_scale);
            ok &= values.optional("cloud_height", env_data.cloud_height);
            ok &= values.optional("cloud_texture_scale", env_data.cloud_texture_scale);
            ok &= values.optional("cloud_speed", env_data.cloud_speed);
            ok &= values.optional("cloud_texture", env_data.cloud_texture_file_name);
            if (const toml::node *cloud_color = sky_table->get("cloud_color")) {
                values.table("cloud_color");
                ok &= read_gradient_color(values, "cloud_color", *cloud_color, env_data.cloud_gradient_color);
            }
            if (const toml::array *gradient = values.array("gradient")) {
                const size_t count = env_data.sky_box_gradient_level_upper + env_data.sky_box_gradient_level_lower;
                if (gradient->size() != count) {
                    values.error("gradient", "must have gradient_upper + gradient_lower colour pairs");
                    ok = false;
                } else {
                    env_data.sky_box_gradient_color_vector.assign(count, gradient_color{});
                    for (size_t i = 0; i < count; ++i) {
                        ok &= read_gradient_color(values, "gradient", (*gradient)[i], env_data.sky_box_gradient_color_vector[i]);
                    }
                }
            }
            ok &= values.finish();
        }

        if (const toml::table *flare_table = root.table("lens_flare")) {
            asset_table values(*flare_table, name);
            ok &= values.optional("enable", env_data.lens_flare_enable);
            ok &= values.optional("brightness_color", env_data.lens_flare_brightness_color);
            ok &= values.optional("max_brightness", env_data.lens_flare_max_brightness);
            ok &= values.optional("main_flare_enable", env_data.main_flare_enable);
            ok &= values.optional("main_flare_texture", env_data.main_flare_texture_file_name);
            ok &= values.optional("main_flare_size", env_data.main_flare_size);
            ok &= values.finish();
        }

        return root.finish() && ok;
    }

    auto environment_load(environment_data &env_data, const char *env_file_name) -> bool {
        const std::string name = eter_file_manager::canonical_name(env_file_name);
        mapped_file file;
        const void *data = nullptr;
        if (!get_file_manager().get(file, name.c_str(), &data)) {
            return false;
        }
        return environment_parse(env_data, std::string_view(static_cast<const char *>(data), file.size()), name);
    }

    auto get_linear_interpolation(float begin, float end, float cur_rate) -> float {
        return (end - begin) * cur_rate + begin;
    }

    ease_out_interpolation::ease_out_interpolation() {
        initialize();
    }

    ease_out_interpolation::~ease_out_interpolation() = default;

    void ease_out_interpolation::initialize() {
        remaining_time_ = 0.0f;
        value_ = 0.0f;
        speed_ = 0.0f;
        acceleration_ = 0.0f;
        start_value_ = 0.0f;
        last_value_ = 0.0f;
    }

    auto ease_out_interpolation::setup(float start, float end, float time) -> bool {
        if (fabs(time) < FLT_EPSILON) {
            time = 0.01f;
        }

        value_ = start;
        start_value_ = start;
        last_value_ = start;

        speed_ = (2.0f * (end - start)) / time;
        acceleration_ = 2.0f * (end - start) / (time * time) - 2.0f * speed_ / time;
        remaining_time_ = time;

        return true;
    }

    void ease_out_interpolation::interpolate(float elapsed_time) {
        last_value_ = value_;

        remaining_time_ -= elapsed_time;
        speed_ += acceleration_ * elapsed_time;
        value_ += speed_ * elapsed_time;

        if (!is_playing()) {
            value_ = 0.0f;
            last_value_ = 0.0f;
        }
    }

    auto ease_out_interpolation::is_playing() -> bool {
        return remaining_time_ > 0.0f;
    }

    auto ease_out_interpolation::value() -> float {
        return value_;
    }

    auto ease_out_interpolation::changing_value() -> float {
        return value_ - last_value_;
    }
}
