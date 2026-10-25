#include "map_outdoor.h"

#include "area_terrain.h"

#include <metin2/world/maps/map_config.h>

#include <metin2/core/parse.h>
#include <metin2/core/timer.h>
#include <metin2/core/util.h>
#include <metin2/core/utils.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/log/log.h>

#include <format>

namespace metin2::inline world::inline terrain {

    auto map_outdoor::load(float x, float y, float z) -> bool {
        destroy();

        std::string file_name = map_file_name(map_data_directory(), "map.toml");

        if (!load_setting(file_name.c_str())) {
            metin2::log::error("Cannot load map settings {}", file_name.c_str());
        }

        create_terrain_patch_proxy_list();
        build_quad_tree();
        load_water_texture();
        create_character_shadow_texture();

        old_read_x_ = -1;

        update(x, y, z);

        std::string local_env_data_name = map_file_name(map_data_directory(), settings_env_data_name_);
        if (get_file_manager().is_exist(local_env_data_name.c_str())) {
            env_data_name_ = local_env_data_name;
        } else {
            const std::string &environment_root = "environment:";
            const std::string &map_name = get_name();

            env_data_name_ = environment_root + settings_env_data_name_;

            if (0 == env_data_name_.compare(environment_root)) {
                const std::string &append_name = map_name.substr(map_name.size() - 2, 2);
                env_data_name_ = environment_root + append_name + ".toml";
            }
        }

        return true;
    }

    auto map_outdoor::environment_data_name() -> std::string & {
        return env_data_name_;
    }

    auto map_outdoor::is_terrain_loaded(uint16_t x, uint16_t y) -> bool {
        for (uint32_t i = 0; i < terrain_vector_.size(); ++i) {
            terrain_area *terrain = terrain_vector_[i];
            uint16_t us_coord_x, us_coord_y;
            terrain->coordinate(&us_coord_x, &us_coord_y);

            if (us_coord_x == x && us_coord_y == y) {
                return true;
            }
        }

        return false;
    }

    auto map_outdoor::is_area_loaded(uint16_t x, uint16_t y) -> bool {
        for (uint32_t i = 0; i < area_vector_.size(); ++i) {
            area *area = area_vector_[i];
            uint16_t us_coord_x, us_coord_y;
            area->coordinate(&us_coord_x, &us_coord_y);

            if (us_coord_x == x && us_coord_y == y) {
                return true;
            }
        }

        return false;
    }

    void map_outdoor::assign_terrain_ptr() {
        short reference_coord_min_x, reference_coord_max_x, reference_coord_min_y, reference_coord_max_y;
        reference_coord_min_x = std::max(cur_coordinate_.terrain_coord_x - LOAD_SIZE_WIDTH, 0);
        reference_coord_max_x = std::min(cur_coordinate_.terrain_coord_x + LOAD_SIZE_WIDTH, terrain_count_x_ - 1);
        reference_coord_min_y = std::max(cur_coordinate_.terrain_coord_y - LOAD_SIZE_WIDTH, 0);
        reference_coord_max_y = std::min(cur_coordinate_.terrain_coord_y + LOAD_SIZE_WIDTH, terrain_count_y_ - 1);

        uint32_t i;
        for (i = 0; i < AROUND_AREA_NUM; ++i) {
            area_value_[i] = nullptr;
            terrain_[i] = nullptr;
        }

        for (i = 0; i < terrain_vector_.size(); ++i) {
            terrain_area *terrain = terrain_vector_[i];
            uint16_t us_coord_x, us_coord_y;
            terrain->coordinate(&us_coord_x, &us_coord_y);

            if (us_coord_x >= reference_coord_min_x &&
                us_coord_x <= reference_coord_max_x &&
                us_coord_y >= reference_coord_min_y &&
                us_coord_y <= reference_coord_max_y) {
                terrain_[(us_coord_y - cur_coordinate_.terrain_coord_y + LOAD_SIZE_WIDTH) * 3 +
                         (us_coord_x - cur_coordinate_.terrain_coord_x + LOAD_SIZE_WIDTH)] = terrain;
            }
        }

        for (i = 0; i < area_vector_.size(); ++i) {
            area *area = area_vector_[i];
            uint16_t us_coord_x, us_coord_y;
            area->coordinate(&us_coord_x, &us_coord_y);

            if (us_coord_x >= reference_coord_min_x &&
                us_coord_x <= reference_coord_max_x &&
                us_coord_y >= reference_coord_min_y &&
                us_coord_y <= reference_coord_max_y) {
                area_value_[(us_coord_y - cur_coordinate_.terrain_coord_y + LOAD_SIZE_WIDTH) * 3 +
                            (us_coord_x - cur_coordinate_.terrain_coord_x + LOAD_SIZE_WIDTH)] = area;
            }
        }
    }

    auto map_outdoor::load_area(uint16_t area_coord_x, uint16_t area_coord_y, uint16_t cell_coord_x, uint16_t cell_coord_y) -> bool {
        if (is_area_loaded(area_coord_x, area_coord_y)) {
            return true;
        }

        uint32_t id = uint32_t(area_coord_x) * 1000 + uint32_t(area_coord_y);
        const std::string area_path_name = map_file_name(map_data_directory(), std::format("{:06}/", id));

        area *area = area::new_value();
        area->set_map_out_door(this);

        area->set_coordinate(area_coord_x, area_coord_y);
        if (!area->load(area_path_name.c_str())) {
            metin2::log::error("Cannot load area ({}, {}): shadow map missing", area_coord_x, area_coord_y);
        }

        area_vector_.push_back(area);

        area->enable_portal(enable_portal_);

        return true;
    }

    auto map_outdoor::load_terrain(uint16_t terrain_coord_x, uint16_t terrain_coord_y, uint16_t cell_coord_x, uint16_t cell_coord_y) -> bool {
        if (is_terrain_loaded(terrain_coord_x, terrain_coord_y)) {
            return true;
        }

        uint32_t start_time = el_timer_get_m_sec();

        uint32_t id = uint32_t(terrain_coord_x) * 1000 + uint32_t(terrain_coord_y);
        const std::string area_folder = map_file_name(map_data_directory(), std::format("{:06}/", id));
        // Every area has an area.toml; without it the area is not part of the map.
        if (!get_file_manager().is_exist((area_folder + "area.toml").c_str())) {
            metin2::log::error("Cannot load terrain {}: no area.toml", area_folder);
            return false;
        }

        terrain_area *terrain = terrain_area::new_value();

        terrain->clear();
        terrain->set_map_out_door(this);

        terrain->set_coordinate(terrain_coord_x, terrain_coord_y);

        terrain->copy_setting_from_global_setting();

        const std::string raw_height_fieldname = area_folder + "height.raw";
        const std::string splat_name = area_folder + "tile.raw";
        const std::string attr_map_name = area_folder + "attr.atr";
        const std::string water_map_name = area_folder + "water.wtr";
        const std::string shadow_tex_name = area_folder + "shadowmap.dds";
        const std::string shadow_map_name = area_folder + "shadowmap.raw";
        const std::string mini_map_tex_name = area_folder + "minimap.dds";

        if (!terrain->load_water_map(water_map_name.c_str())) {
            metin2::log::error("Cannot load terrain ({}, {}): water map missing", terrain_coord_x, terrain_coord_y);
        }

        if (!terrain->load_height_map(raw_height_fieldname.c_str())) {
            metin2::log::error("Cannot load terrain ({}, {}): height map missing", terrain_coord_x, terrain_coord_y);
        }

        if (!terrain->load_attr_map(attr_map_name.c_str())) {
            metin2::log::error("Cannot load terrain ({}, {}): attribute map missing", terrain_coord_x, terrain_coord_y);
        }

        if (!terrain->raw_load_tile_map(splat_name.c_str())) {
            metin2::log::error("Cannot load terrain ({}, {}): tile map missing", terrain_coord_x, terrain_coord_y);
        }

        terrain->load_shadow_texture(shadow_tex_name.c_str());

        if (!terrain->load_shadow_map(shadow_map_name.c_str())) {
            metin2::log::error("Cannot load terrain ({}, {}): shadow map missing", terrain_coord_x, terrain_coord_y);
        }

        terrain->load_mini_map_texture(mini_map_tex_name.c_str());
        terrain->calculate_terrain_patch();

        terrain->set_ready();

        metin2::log::debug("Terrain loaded in {} ms", el_timer_get_m_sec() - start_time);

        terrain_vector_.push_back(terrain);

        return true;
    }

    auto map_outdoor::load_setting(const char *file_name) -> bool {
        map_config config;
        if (!read_map_config(file_name, config)) {
            return false;
        }
        if (!config.has_terrain) {
            metin2::log::error("Map config {}: no [terrain] table", file_name);
            return false;
        }

        setting_terrain_visible_ = config.terrain_visible;
        view_radius_ = config.view_radius;
        height_scale_ = config.height_scale;

        set_terrain_count(config.size_x, config.size_y);

        terrain_tex_coord_base_ = 1.0f / (float)(terrain_impl::PATCH_XSIZE * terrain_impl::CELLSCALE);

        if (config.has_base_position) {
            set_base_xy(config.base_x, config.base_y);
        }

        if (!texture_set_value_.load(config.texture_set.c_str(), terrain_tex_coord_base_)) {
            metin2::log::error("Map config {}: cannot load texture set {}", file_name, config.texture_set);
            return false;
        }

        terrain_area::set_texture_set(&texture_set_value_);

        settings_env_data_name_ = config.environment;

        water_tex_coord_base_ = 1.0f / (float)(terrain_impl::CELLSCALE * 4);

        splat_alpha_ = math::matrix_scaling(+terrain_tex_coord_base_ * 2.0f * (float)(terrain_impl::PATCH_XSIZE) / (float)(terrain_impl::SPLATALPHA_RAW_XSIZE - 2), -terrain_tex_coord_base_ * 2.0f * (float)(terrain_impl::PATCH_YSIZE) / (float)(terrain_impl::SPLATALPHA_RAW_XSIZE - 2), 0.0f);
        splat_alpha_._41 = terrain_tex_coord_base_ * 4.6f;
        splat_alpha_._42 = terrain_tex_coord_base_ * 4.6f;

        static_shadow_ = math::matrix_scaling(+terrain_tex_coord_base_ * ((float)terrain_impl::PATCH_XSIZE / static_cast<float>(terrain_impl::XSIZE)), -terrain_tex_coord_base_ * ((float)terrain_impl::PATCH_YSIZE / static_cast<float>(terrain_impl::XSIZE)), 0.0f);
        static_shadow_._41 = 0.0f;
        static_shadow_._42 = 0.0f;

        dynamic_shadow_scale_ = math::matrix_scaling(1.0f / 2550.0f, -1.0f / 2550.0f, 1.0f);
        dynamic_shadow_scale_._41 = 0.5f;
        dynamic_shadow_scale_._42 = 0.5f;

        building_transparent_ = math::matrix_scaling(1.0f / ((float)width_), -1.0f / ((float)height_), 1.0f);
        building_transparent_._41 = 0.5f;
        building_transparent_._42 = 0.5f;
        return true;
    }
}
