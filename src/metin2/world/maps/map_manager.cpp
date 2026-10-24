#include <metin2/files/eter_file_manager.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>

#include <metin2/world/terrain/map_outdoor.h>
#include "map_manager.h"

#include <metin2/core/file_loader.h>
#include <metin2/core/timer.h>
#include <metin2/gpu/types.h>

#include <algorithm>

namespace metin2::inline world::inline maps {

    auto map_manager::is_map_outdoor() -> bool {
        if (map_)
            return true;

        return false;
    }

    auto map_manager::map_outdoor_ref() -> map_outdoor & {
        assert(nullptr != map_);
        return *map_;
    }

    map_manager::map_manager() : mc_pcur_environment_data_(nullptr) {
        map_ = nullptr;
        is_software_tiling_enable_reserved_ = false;
    }

    map_manager::~map_manager() {
        destroy();
    }

    auto map_manager::is_software_tiling_enable() -> bool {
        return terrain_patch::SOFTWARE_TRANSFORM_PATCH_ENABLE;
    }

    void map_manager::reserve_software_tiling_enable(bool is_enable) {
        is_software_tiling_enable_reserved_ = is_enable;
    }

    void map_manager::initialize() {
        mc_pcur_environment_data_ = nullptr;
        blend_target_ = nullptr;
        load_map_info_vector();
    }

    void map_manager::create() {
        assert(nullptr == map_ && "CMapManager::Create");
        if (map_) {
            clear();
            return;
        }

        terrain_patch::SOFTWARE_TRANSFORM_PATCH_ENABLE = is_software_tiling_enable_reserved_;

        map_ = (map_outdoor *)alloc_map();

        assert(nullptr != map_ && "CMapManager::Create MAP is NULL");
    }

    void map_manager::destroy() {
        stl_wipe_second(environment_data_map_value_);

        if (map_) {
            map_->clear();
            delete map_;
            map_ = nullptr;
        }
    }

    void map_manager::clear() {
        if (map_)
            map_->clear();
    }

    auto map_manager::alloc_map() -> map_base * {
        return new map_outdoor;
    }

    auto map_manager::load_map(const std::string &map_name, float x, float y, float z) -> bool {
        map_outdoor &map = map_outdoor_ref();

        map.leave();
        map.set_name(map_name);
        map.load_property();

        if (map_base::MAPTYPE_INDOOR == map.get_type()) {
            metin2::log::error("Cannot load indoor map");
            return false;
        } else if (map_base::MAPTYPE_OUTDOOR == map.get_type()) {
            if (!map.load(x, y, z)) {
                metin2::log::error("Cannot load outdoor map");
                return false;
            }

            register_environment_data(0, map.environment_data_name().c_str());

            set_environment_data(0);
        } else {
            metin2::log::error("Cannot load map: invalid map type");
            return false;
        }

        map.enter();
        return true;
    }

    auto map_manager::is_map_ready() -> bool {
        if (!map_)
            return false;

        return map_->is_ready();
    }

    auto map_manager::update_map(float fx, float fy, float fz) -> bool {
        if (!map_)
            return false;

        map_outdoor &map = map_outdoor_ref();
        return map.update(fx, -fy, fz);
    }

    void map_manager::update_around_ambience(float fx, float fy, float fz) {
        if (!map_)
            return;

        map_outdoor &map = map_outdoor_ref();
        map.update_around_ambience(fx, -fy, fz);
    }

    auto map_manager::height(float fx, float fy) -> float {
        if (!map_) {
            metin2::log::error("Height at ({:.6f}, {:.6f}) requested before the map was created", fx, fy);
            return 0.0f;
        }
        map_outdoor &map = map_outdoor_ref();
        return map.height(fx, fy);
    }

    auto map_manager::terrain_height(float fx, float fy) -> float {
        if (!map_) {
            metin2::log::error("Terrain height at ({:.6f}, {:.6f}) requested before the map was created", fx, fy);
            return 0.0f;
        }
        map_outdoor &map = map_outdoor_ref();
        return map.terrain_height(fx, fy);
    }

    auto map_manager::water_height(int x, int y, int32_t *pl_water_height) -> bool {
        if (!map_) {
            metin2::log::error("Water height at ({}, {}) requested before the map was created", x, y);
            return false;
        }

        map_outdoor &map = map_outdoor_ref();
        return map.water_height(x, y, pl_water_height);
    }

    namespace {
        constexpr float ENVIRONMENT_BLEND_SECONDS = 1.5f;

        auto lerp(float a, float b, float t) -> float {
            return a + (b - a) * t;
        }

        auto lerp(const math::color &a, const math::color &b, float t) -> math::color {
            return a + (b - a) * t;
        }

        void blend_light(const gpu::light &from, const gpu::light &to, float t, gpu::light &out) {
            out = to;
            out.diffuse = lerp(from.diffuse, to.diffuse, t);
            out.ambient = lerp(from.ambient, to.ambient, t);
            out.specular = lerp(from.specular, to.specular, t);
            const math::vector3 direction = from.direction + (to.direction - from.direction) * t;
            if (math::length_sq(direction) > 0.0f)
                out.direction = math::normalize(direction);
        }
    }

    void map_manager::update_environment_blend() {
        if (!blend_target_)
            return;

        const float t = std::clamp((get_timer().current_second() - blend_start_time_) / ENVIRONMENT_BLEND_SECONDS, 0.0f, 1.0f);
        const environment_data &to = *blend_target_;

        if (t >= 1.0f) {
            mc_pcur_environment_data_ = blend_target_;
            blend_target_ = nullptr;
            map_outdoor_ref().set_environment_data_ptr(mc_pcur_environment_data_);
            return;
        }

        environment_data &out = blended_environment_;
        for (int i = 0; i < ENV_DIRLIGHT_NUM; ++i)
            blend_light(blend_from_.dir_lights[i], to.dir_lights[i], t, out.dir_lights[i]);
        out.material_value.diffuse = lerp(blend_from_.material_value.diffuse, to.material_value.diffuse, t);
        out.material_value.ambient = lerp(blend_from_.material_value.ambient, to.material_value.ambient, t);
        out.material_value.specular = lerp(blend_from_.material_value.specular, to.material_value.specular, t);
        out.material_value.emissive = lerp(blend_from_.material_value.emissive, to.material_value.emissive, t);
        out.fog_color = lerp(blend_from_.fog_color, to.fog_color, t);
        // A reserved environment gets its fog distances from the view distance setting, which
        // writes them into the current environment itself.
        if (!to.reserve) {
            out.fog_near_distance = lerp(blend_from_.fog_near_distance, to.fog_near_distance, t);
            out.fog_far_distance = lerp(blend_from_.fog_far_distance, to.fog_far_distance, t);
        }
        out.filtering_color = lerp(blend_from_.filtering_color, to.filtering_color, t);
        // The filter copied its colour when the environment was set.
        map_outdoor_ref().set_environment_screen_filter();
    }

    void map_manager::begin_environment() {
        if (!map_)
            return;

        if (!mc_pcur_environment_data_)
            return;

        update_environment_blend();

        map_outdoor &map = map_outdoor_ref();

        STATEMANAGER.save_render_state(gpu::state::LIGHTING, true);

        STATEMANAGER.save_render_state(gpu::state::FOG_ENABLE, mc_pcur_environment_data_->fog_enable);

        STATEMANAGER.set_material(&mc_pcur_environment_data_->material_value);

        if (mc_pcur_environment_data_->dir_lights_enable[ENV_DIRLIGHT_BACKGROUND]) {
            STATEMANAGER.enable_light(0, true);

            map.apply_light(reinterpret_cast<uintptr_t>(mc_pcur_environment_data_), mc_pcur_environment_data_->dir_lights[ENV_DIRLIGHT_BACKGROUND]);
        } else
            STATEMANAGER.enable_light(0, false);

        if (mc_pcur_environment_data_->fog_enable) {
            uint32_t fog_color = mc_pcur_environment_data_->fog_color;
            STATEMANAGER.set_render_state(gpu::state::FOG_COLOR, fog_color);

            if (mc_pcur_environment_data_->density_fog) {
                float density = 0.00015f;
                STATEMANAGER.set_render_state(gpu::state::FOG_VERTEX_MODE, gpu::fog::EXP);
                STATEMANAGER.set_render_state(gpu::state::FOG_DENSITY, *((uint32_t *)&density));
            } else {
                speed_tree_forest_direct_x8 &forest = get_speed_tree_forest_direct_x8();
                forest.set_fog(
                    mc_pcur_environment_data_->get_fog_near_distance(),
                    mc_pcur_environment_data_->get_fog_far_distance());

                float fog_near = mc_pcur_environment_data_->get_fog_near_distance();
                float fog_far = mc_pcur_environment_data_->get_fog_far_distance();
                STATEMANAGER.set_render_state(gpu::state::FOG_VERTEX_MODE, gpu::fog::LINEAR);
                STATEMANAGER.set_render_state(gpu::state::RANGE_FOG_ENABLE, true);
                STATEMANAGER.set_render_state(gpu::state::FOG_START, *((uint32_t *)&fog_near));
                STATEMANAGER.set_render_state(gpu::state::FOG_END, *((uint32_t *)&fog_far));
            }
        }

        map.on_begin_environment();
    }

    void map_manager::end_environment() {
        if (!mc_pcur_environment_data_)
            return;

        STATEMANAGER.restore_render_state(gpu::state::LIGHTING);
        STATEMANAGER.restore_render_state(gpu::state::FOG_ENABLE);
    }

    void map_manager::set_environment_data(int env_data_index) {
        const environment_data *environmen_data;

        if (get_environment_data(env_data_index, &environmen_data))
            set_environment_data_ptr(environmen_data);
    }

    void map_manager::set_environment_data_ptr(const environment_data *environment_data) {
        if (!map_)
            return;

        if (!environment_data) {
            assert(!"null environment data");
            metin2::log::error("No environment data");
            return;
        }

        map_outdoor &map = map_outdoor_ref();

        if (environment_data == mc_pcur_environment_data_ || environment_data == blend_target_)
            return;

        // The first environment of a map is applied at once; a later change fades from what is on
        // screen now (which may itself be mid-blend).
        if (mc_pcur_environment_data_) {
            blend_from_ = *mc_pcur_environment_data_;
            blended_environment_ = *environment_data;
            blend_target_ = environment_data;
            blend_start_time_ = get_timer().current_second();
            mc_pcur_environment_data_ = &blended_environment_;
            map.set_environment_data_ptr(mc_pcur_environment_data_);
            update_environment_blend();
            return;
        }

        mc_pcur_environment_data_ = environment_data;

        map.set_environment_data_ptr(mc_pcur_environment_data_);
    }

    void map_manager::reset_environment_data_ptr(const environment_data *environment_data) {
        if (!map_)
            return;

        if (!environment_data) {
            assert(!"null environment data");
            metin2::log::error("No environment data");
            return;
        }

        map_outdoor &map = map_outdoor_ref();

        blend_target_ = nullptr;
        mc_pcur_environment_data_ = environment_data;
        map.reset_environment_data_ptr(mc_pcur_environment_data_);
    }

    auto map_manager::register_environment_data(uint32_t index, const char *file_name) -> bool {
        environment_data *environment_data = alloc_environment_data();

        if (!load_environment_data(file_name, environment_data)) {
            delete_environment_data(environment_data);
            return false;
        }

        auto f = environment_data_map_value_.find(index);
        if (environment_data_map_value_.end() == f) {
            environment_data_map_value_.insert(environment_data_map::value_type(index, environment_data));
        } else {
            delete f->second;
            f->second = environment_data;
        }
        return true;
    }

    auto map_manager::get_environment_data(uint32_t index, const environment_data **environment_data) -> bool {
        auto itor = environment_data_map_value_.find(index);

        if (environment_data_map_value_.end() == itor) {
            *environment_data = nullptr;
            return false;
        }

        *environment_data = itor->second;
        return true;
    }

    void map_manager::refresh_portal() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            area *area;
            if (!map.area_pointer(i, &area))
                continue;

            area->refresh_portal();
        }
    }

    void map_manager::clear_portal() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            area *area;
            if (!map.area_pointer(i, &area))
                continue;

            area->clear_portal();
        }
    }

    void map_manager::add_showing_portal_id(int id) {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            area *area;
            if (!map.area_pointer(i, &area))
                continue;

            area->add_showing_portal_id(id);
        }
    }

    auto map_manager::alloc_environment_data() -> environment_data * {
        auto *environment_data_value = new environment_data;
        environment_init(*environment_data_value);
        return environment_data_value;
    }

    void map_manager::delete_environment_data(environment_data *environment_data) {
        delete environment_data;
        environment_data = nullptr;
    }

    auto map_manager::load_environment_data(const char *file_name, environment_data *environment_data) -> bool {
        if (!environment_data)
            return false;

        return (bool)environment_load(*environment_data, file_name);
    }

    auto map_manager::shadow_map_color(float fx, float fy) -> uint32_t {
        if (!is_map_ready())
            return 0xFFFFFFFF;

        map_outdoor &map = map_outdoor_ref();
        return map.shadow_map_color(fx, fy);
    }

    auto map_manager::rendered_splat_num(int *patch, int *splat, float *splat_ratio) -> std::vector<int> & {
        if (!map_) {
            static std::vector<int> empty_vector;
            *patch = 0;
            *splat = 0;
            return empty_vector;
        }

        map_outdoor &map = map_outdoor_ref();
        return map.rendered_splat_num(patch, splat, splat_ratio);
    }

    auto map_manager::rendered_graphic_thing_instance_num(uint32_t *graphic_thing_instance_num, uint32_t *crc_num) -> area::crc_with_number_vector & {
        if (!map_) {
            static area::crc_with_number_vector empty_vector;
            *graphic_thing_instance_num = 0;
            *crc_num = 0;
            return empty_vector;
        }

        map_outdoor &map = map_outdoor_ref();
        return map.rendered_graphic_thing_instance_num(graphic_thing_instance_num, crc_num);
    }

    auto map_manager::get_normal(int ix, int iy, math::vector3 *normal) -> bool {
        if (!is_map_ready())
            return false;

        map_outdoor &map = map_outdoor_ref();
        return map.get_normal(ix, iy, normal);
    }

    auto map_manager::is_physical_collision(const math::vector3 &check_position) -> bool {
        if (!is_map_ready())
            return false;

        map_outdoor &map = map_outdoor_ref();
        return map.is_attr_on(check_position.x, -check_position.y, terrain_impl::ATTRIBUTE_BLOCK);
    }

    auto map_manager::is_attr_on(float x, float y, uint8_t attr) -> bool {
        if (!is_map_ready())
            return false;

        map_outdoor &map = map_outdoor_ref();
        return map.is_attr_on(x, y, attr);
    }

    auto map_manager::attr(float x, float y, uint8_t *attr) -> bool {
        if (!is_map_ready())
            return false;

        map_outdoor &map = map_outdoor_ref();
        return map.attr(x, y, attr);
    }

    auto map_manager::is_attr_on(int x, int y, uint8_t attr) -> bool {
        if (!is_map_ready())
            return false;

        map_outdoor &map = map_outdoor_ref();
        return map.is_attr_on(x, y, attr);
    }

    auto map_manager::attr(int x, int y, uint8_t *attr) -> bool {
        if (!is_map_ready())
            return false;

        map_outdoor &map = map_outdoor_ref();
        return map.attr(x, y, attr);
    }

    void map_manager::set_terrain_render_sort(map_outdoor::terrain_render_sort terrain_render_sort) {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.set_terrain_render_sort(terrain_render_sort);
    }

    void map_manager::set_transparent_tree(bool transparen_tree) {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.set_transparent_tree(transparen_tree);
    }

    auto map_manager::get_terrain_render_sort() -> map_outdoor::terrain_render_sort {
        if (!is_map_ready())
            return map_outdoor::DISTANCE_SORT;

        map_outdoor &map = map_outdoor_ref();
        return map.get_terrain_render_sort();
    }

    void map_manager::base_xy(uint32_t *base_x, uint32_t *base_y) {
        if (!is_map_ready()) {
            *base_x = 0;
            *base_y = 0;
        }

        map_outdoor &map = map_outdoor_ref();
        map.base_xy(base_x, base_y);
    }

    void map_manager::load_map_info_vector() {
        mapped_file file;
        const void *data;
        if (!get_file_manager().get(file, atlas_info_file_name_.c_str(), &data))
            if (!get_file_manager().get(file, "AtlasInfo.txt", &data))
                return;

        memory_text_file_loader text_file_loader;
        text_file_loader.bind(file.size(), data);

        char map_name[256];
        int x, y;
        int width_value, height;
        for (uint32_t line_index = 0; line_index < text_file_loader.line_count(); ++line_index) {
            const std::string &line = text_file_loader.line_string(line_index);
            sscanf(line.c_str(), "%s %d %d %d %d",
                map_name,
                &x, &y, &width_value, &height);

            if ('\0' == map_name[0])
                continue;

            map_info map_info_value;
            map_info_value.name_value = map_name;
            map_info_value.base_x = x;
            map_info_value.base_y = y;

            map_info_value.size_x = width_value;
            map_info_value.size_y = height;

            map_info_value.end_x = map_info_value.base_x + map_info_value.size_x * terrain_impl::TERRAIN_XSIZE;
            map_info_value.end_y = map_info_value.base_y + map_info_value.size_y * terrain_impl::TERRAIN_YSIZE;

            vct_map_info_.push_back(map_info_value);
        }

        return;
    }
}
