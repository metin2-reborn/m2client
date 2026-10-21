#include "map_util.h"
#include <cfloat>
#include <metin2/core/parse.h>
#include <metin2/core/stl.h>
#include <metin2/core/text_file_loader.h>
#include <metin2/gpu/types.h>

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

    auto environment_load(environment_data &env_data, const char *env_file_name) -> bool {
        text_file_loader text_loader;

        if (!text_loader.load(env_file_name))
            return false;

        text_loader.set_top();

        text_loader.token_boolean("reserved", &env_data.reserve);

        if (text_loader.set_child_node("directionallight")) {
            math::vector3 dir;
            text_loader.token_direction("direction", &dir);

            if (text_loader.set_child_node("background")) {
                env_data.dir_lights[ENV_DIRLIGHT_BACKGROUND].direction = dir;
                text_loader.token_boolean("enable", &env_data.dir_lights_enable[ENV_DIRLIGHT_BACKGROUND]);
                text_loader.token_color("diffuse", &env_data.dir_lights[ENV_DIRLIGHT_BACKGROUND].diffuse);
                text_loader.token_color("ambient", &env_data.dir_lights[ENV_DIRLIGHT_BACKGROUND].ambient);
                text_loader.set_parent_node();
            }

            if (text_loader.set_child_node("character")) {
                env_data.dir_lights[ENV_DIRLIGHT_CHARACTER].direction = dir;
                text_loader.token_boolean("enable", &env_data.dir_lights_enable[ENV_DIRLIGHT_CHARACTER]);
                text_loader.token_color("diffuse", &env_data.dir_lights[ENV_DIRLIGHT_CHARACTER].diffuse);
                text_loader.token_color("ambient", &env_data.dir_lights[ENV_DIRLIGHT_CHARACTER].ambient);
                text_loader.set_parent_node();
            }

            text_loader.set_parent_node();
        }

        if (text_loader.set_child_node("material")) {
            text_loader.token_color("diffuse", &env_data.material_value.diffuse);
            text_loader.token_color("ambient", &env_data.material_value.ambient);
            text_loader.token_color("emissive", &env_data.material_value.emissive);
            text_loader.set_parent_node();
        }

        if (text_loader.set_child_node("fog")) {
            text_loader.token_boolean("enable", &env_data.fog_enable);
            text_loader.token_boolean("isdensity", &env_data.density_fog);
            text_loader.token_float("neardistance", &env_data.fog_near_distance);
            text_loader.token_float("fardistance", &env_data.fog_far_distance);
            text_loader.token_color("color", &env_data.fog_color);
            text_loader.set_parent_node();
        }

        if (text_loader.set_child_node("filter")) {
            text_loader.token_boolean("enable", &env_data.filtering_enable);
            text_loader.token_color("color", &env_data.filtering_color);
            text_loader.token_byte("alphasrc", &env_data.filtering_alpha_src);
            text_loader.token_byte("alphadest", &env_data.filtering_alpha_dest);
            text_loader.set_parent_node();
        }

        if (text_loader.set_child_node("skybox")) {
            text_loader.token_boolean("btexturerendermode", &env_data.sky_box_texture_render_mode);
            text_loader.token_vector3("scale", &env_data.sky_box_scale);
            text_loader.token_byte("gradientlevelupper", &env_data.sky_box_gradient_level_upper);
            text_loader.token_byte("gradientlevellower", &env_data.sky_box_gradient_level_lower);

            text_loader.token_string("frontfacefilename", &env_data.sky_box_face_file_name[0]);
            text_loader.token_string("backfacefilename", &env_data.sky_box_face_file_name[1]);
            text_loader.token_string("leftfacefilename", &env_data.sky_box_face_file_name[2]);
            text_loader.token_string("rightfacefilename", &env_data.sky_box_face_file_name[3]);
            text_loader.token_string("topfacefilename", &env_data.sky_box_face_file_name[4]);
            text_loader.token_string("bottomfacefilename", &env_data.sky_box_face_file_name[5]);

            text_loader.token_vector2("cloudscale", &env_data.cloud_scale);
            text_loader.token_float("cloudheight", &env_data.cloud_height);
            text_loader.token_vector2("cloudtexturescale", &env_data.cloud_texture_scale);
            text_loader.token_vector2("cloudspeed", &env_data.cloud_speed);
            text_loader.token_string("cloudtexturefilename", &env_data.cloud_texture_file_name);

            token_vector *token_vector_cloud_color;
            if (text_loader.get_token_vector("cloudcolor", &token_vector_cloud_color))
                if (0 == token_vector_cloud_color->size() % 8) {
                    env_data.cloud_gradient_color.first_color.r = metin2::to_number<double>(token_vector_cloud_color->at(0).c_str());
                    env_data.cloud_gradient_color.first_color.g = metin2::to_number<double>(token_vector_cloud_color->at(1).c_str());
                    env_data.cloud_gradient_color.first_color.b = metin2::to_number<double>(token_vector_cloud_color->at(2).c_str());
                    env_data.cloud_gradient_color.first_color.a = metin2::to_number<double>(token_vector_cloud_color->at(3).c_str());

                    env_data.cloud_gradient_color.second_color.r = metin2::to_number<double>(token_vector_cloud_color->at(4).c_str());
                    env_data.cloud_gradient_color.second_color.g = metin2::to_number<double>(token_vector_cloud_color->at(5).c_str());
                    env_data.cloud_gradient_color.second_color.b = metin2::to_number<double>(token_vector_cloud_color->at(6).c_str());
                    env_data.cloud_gradient_color.second_color.a = metin2::to_number<double>(token_vector_cloud_color->at(7).c_str());
                }

            uint8_t gradient_count = env_data.sky_box_gradient_level_upper + env_data.sky_box_gradient_level_lower;
            token_vector *token_vector;
            if (text_loader.get_token_vector("gradient", &token_vector))
                if (0 == token_vector->size() % 8)
                    if (gradient_count == token_vector->size() / 8) {
                        env_data.sky_box_gradient_color_vector.clear();
                        env_data.sky_box_gradient_color_vector.resize(gradient_count);
                        for (uint32_t i = 0; i < gradient_count; ++i) {
                            env_data.sky_box_gradient_color_vector[i].first_color.r = metin2::to_number<double>(token_vector->at(i * 8 + 0).c_str());
                            env_data.sky_box_gradient_color_vector[i].first_color.g = metin2::to_number<double>(token_vector->at(i * 8 + 1).c_str());
                            env_data.sky_box_gradient_color_vector[i].first_color.b = metin2::to_number<double>(token_vector->at(i * 8 + 2).c_str());
                            env_data.sky_box_gradient_color_vector[i].first_color.a = metin2::to_number<double>(token_vector->at(i * 8 + 3).c_str());

                            env_data.sky_box_gradient_color_vector[i].second_color.r = metin2::to_number<double>(token_vector->at(i * 8 + 4).c_str());
                            env_data.sky_box_gradient_color_vector[i].second_color.g = metin2::to_number<double>(token_vector->at(i * 8 + 5).c_str());
                            env_data.sky_box_gradient_color_vector[i].second_color.b = metin2::to_number<double>(token_vector->at(i * 8 + 6).c_str());
                            env_data.sky_box_gradient_color_vector[i].second_color.a = metin2::to_number<double>(token_vector->at(i * 8 + 7).c_str());
                        }
                    }

            text_loader.set_parent_node();
        }

        if (text_loader.set_child_node("lensflare")) {
            text_loader.token_boolean("enable", &env_data.lens_flare_enable);
            text_loader.token_color("brightnesscolor", &env_data.lens_flare_brightness_color);
            text_loader.token_float("maxbrightness", &env_data.lens_flare_max_brightness);
            text_loader.token_boolean("mainflareenable", &env_data.main_flare_enable);
            text_loader.token_string("mainflaretexturefilename", &env_data.main_flare_texture_file_name);
            text_loader.token_float("mainflaresize", &env_data.main_flare_size);

            text_loader.set_parent_node();
        }
        return true;
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
