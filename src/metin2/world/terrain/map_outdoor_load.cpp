#include "map_outdoor.h"

#include "area_terrain.h"

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

        std::string file_name = map_file_name(map_data_directory(), "Setting.txt");

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
                env_data_name_ = environment_root + append_name + ".msenv";
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
        const std::string filename = area_folder + "AreaProperty.txt";

        token_vector_map token_vector_map;

        if (!load_multiple_text_data(filename.c_str(), token_vector_map)) {
            metin2::log::error("Cannot load terrain: area property cannot be read");
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("scripttype")) {
            metin2::log::error("Cannot load terrain: area property has no scripttype key");
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("areaname")) {
            metin2::log::error("Cannot load terrain: area property has no areaname key");
            return false;
        }

        const std::string &type = token_vector_map["scripttype"][0];
        const std::string &area_name = token_vector_map["areaname"][0];

        if (type != "AreaProperty") {
            metin2::log::error("Cannot load terrain: area property script type is not AreaProperty");
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
        terrain->set_name(area_name.c_str());
        terrain->calculate_terrain_patch();

        terrain->set_ready();

        metin2::log::debug("Terrain loaded in {} ms", el_timer_get_m_sec() - start_time);

        terrain_vector_.push_back(terrain);

        return true;
    }

    auto map_outdoor::load_setting(const char *file_name) -> bool {
        token_vector_map token_vector_map;

        if (!load_multiple_text_data(file_name, token_vector_map)) {
            metin2::log::error("Cannot load map settings {}", file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("scripttype")) {
            metin2::log::error("Map settings {}: no scripttype key", file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("viewradius")) {
            metin2::log::error("Map settings {}: no viewradius key", file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("cellscale")) {
            metin2::log::error("Map settings {}: no cellscale key", file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("heightscale")) {
            metin2::log::error("Map settings {}: no heightscale key", file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("mapsize")) {
            metin2::log::error("Map settings {}: no mapsize key", file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("textureset")) {
            metin2::log::error("Map settings {}: no textureset key", file_name);
            return false;
        }

        if (token_vector_map.end() != token_vector_map.find("terrainvisible")) {
            setting_terrain_visible_ = (bool)(metin2::to_number<int>(token_vector_map["terrainvisible"][0].c_str()) != 0);
        } else {
            setting_terrain_visible_ = true;
        }

        const std::string &type = token_vector_map["scripttype"][0];
        const std::string &view_radius = token_vector_map["viewradius"][0];
        const std::string &height_scale = token_vector_map["heightscale"][0];
        const std::string &map_size_x = token_vector_map["mapsize"][0];
        const std::string &map_size_y = token_vector_map["mapsize"][1];

        std::string texture_set;
        token_vector &vec_token = token_vector_map["textureset"];
        if (!vec_token.empty()) {
            texture_set = vec_token[0];
        }

        if (type != "MapSetting") {
            metin2::log::error("Map settings {}: unknown script type", file_name);
            return false;
        }

        view_radius_ = metin2::to_number<long>(view_radius.c_str());

        if (0 >= view_radius_) {
            metin2::log::error("Map settings {}: view radius must be greater than 0", file_name);
            return false;
        }

        height_scale_ = metin2::to_number<double>(height_scale.c_str());

        set_terrain_count(metin2::to_number<int>(map_size_x.c_str()), metin2::to_number<int>(map_size_y.c_str()));

        terrain_tex_coord_base_ = 1.0f / (float)(terrain_impl::PATCH_XSIZE * terrain_impl::CELLSCALE);

        if (token_vector_map.end() != token_vector_map.find("baseposition")) {
            const std::string &map_base_x = token_vector_map["baseposition"][0];
            const std::string &map_base_y = token_vector_map["baseposition"][1];
            set_base_xy((uint32_t)metin2::to_number<long>(map_base_x.c_str()), (uint32_t)metin2::to_number<long>(map_base_y.c_str()));
        }

        if (!texture_set_value_.load(texture_set.c_str(), terrain_tex_coord_base_)) {
            metin2::log::error("Map settings {}: cannot load texture set {}", file_name, texture_set);
            return false;
        }

        terrain_area::set_texture_set(&texture_set_value_);

        if (token_vector_map.end() != token_vector_map.find("environment")) {
            const token_vector &environment_vector = token_vector_map["environment"];
            if (!environment_vector.empty()) {
                settings_env_data_name_ = environment_vector[0];
            } else {
                metin2::log::error("Map settings {}: cannot load environment data", file_name);
            }
        }

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
