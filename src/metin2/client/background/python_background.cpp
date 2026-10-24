#include "python_background.h"

#include "python_mini_map.h"

#include <metin2/client/app/python_system.h>
#include <metin2/client/characters/python_character_manager.h>
#include <metin2/client/protocol/python_network_stream.h>
#include <metin2/core/timer.h>
#include <metin2/geometry/culling_manager.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/camera.h>
#include <metin2/log/log.h>
#include <metin2/window/events.h>
#include <metin2/world/terrain/map_outdoor.h>

#include <algorithm>

#include <format>

namespace metin2::inline client::inline background {

    std::string effect_name = "d:/ymir work/effect/etc/direction/direction_land.mse";

    auto python_background::render_shadow_time() -> uint32_t {
        return render_shadow_time_;
    }

    auto python_background::set_visible_part(int map_out_door_part, bool is_visible) -> bool {
        if (!map_)
            return false;

        map_outdoor &map = map_outdoor_ref();
        map.set_visible_part(map_out_door_part, is_visible);
        return true;
    }

    void python_background::enable_terrain_only_for_height() {
        if (!map_)
            return;

        map_outdoor &map = map_outdoor_ref();
        map.enable_terrain_only_for_height(true);
    }

    auto python_background::set_splat_limit(int splat_num) -> bool {
        if (!map_)
            return false;

        map_outdoor &map = map_outdoor_ref();
        map.set_splat_limit(splat_num);
        return true;
    }

    void python_background::create_character_shadow_texture() {
        if (!map_)
            return;

        map_outdoor &map = map_outdoor_ref();
        map.create_character_shadow_texture();
    }

    void python_background::release_character_shadow_texture() {
        if (!map_)
            return;

        map_outdoor &map = map_outdoor_ref();
        map.release_character_shadow_texture();
    }

    void python_background::refresh_shadow_level() {
        set_shadow_level(get_python_system().shadow_level());
    }

    void python_background::refresh_terrain_options() {
        if (!map_) {
            return;
        }
        constexpr float DETAIL_STRENGTH[] = {0.0f, 1.5f, 3.0f};
        map_outdoor &map = map_outdoor_ref();
        map.set_terrain_detail(DETAIL_STRENGTH[std::clamp(get_python_system().terrain_detail(), 0, 2)]);
    }

    auto python_background::set_shadow_level(int level) -> bool {
        if (!map_)
            return false;

        if (shadow_level_ == level)
            return true;

        map_outdoor &map = map_outdoor_ref();

        shadow_level_ = level;

        switch (shadow_level_) {
        case SHADOW_NONE:
            map.set_draw_shadow(false);
            map.set_draw_character_shadow(false);
            map.set_shadow_texture_size(512);
            break;

        case SHADOW_GROUND:
            map.set_draw_shadow(true);
            map.set_draw_character_shadow(false);
            map.set_shadow_texture_size(512);
            break;

        case SHADOW_GROUND_AND_SOLO:
            map.set_draw_shadow(true);
            map.set_draw_character_shadow(true);
            map.set_shadow_texture_size(512);
            break;

        case SHADOW_ALL:
            map.set_draw_shadow(true);
            map.set_draw_character_shadow(true);
            map.set_shadow_texture_size(512);
            break;

        case SHADOW_ALL_HIGH:
            map.set_draw_shadow(true);
            map.set_draw_character_shadow(true);
            map.set_shadow_texture_size(1024);
            break;

        case SHADOW_ALL_MAX:
            map.set_draw_shadow(true);
            map.set_draw_character_shadow(true);
            map.set_shadow_texture_size(2048);
            break;
        }

        return true;
    }

    void python_background::select_view_distance_num(int num) {
        if (!map_)
            return;

        map_outdoor &map = map_outdoor_ref();

        if (!mc_pcur_environment_data_) {
            metin2::log::error("Cannot select view distance {}: no environment is loaded", num);
            return;
        }

        view_distance_num_ = num;

        environment_data *env = ((environment_data *)mc_pcur_environment_data_);

        // Only reserved environments take the fog of the view distance set; the others keep their
        // own, which may set the mood of the area.
        if (env->reserve) {
            env->fog_near_distance = view_distance_set_[view_distance_num_].fog_start;
            env->fog_far_distance = view_distance_set_[view_distance_num_].fog_end;
            env->sky_box_scale = view_distance_set_[view_distance_num_].sky_box_scale;
            map.set_environment_sky_box();
        }
    }

    void python_background::set_view_distance_set(int num, float far_clip) {
        if (!map_)
            return;

        view_distance_set_[num].fog_start = far_clip * 0.5f;
        view_distance_set_[num].fog_end = far_clip * 0.7f;

        float sky_box_scale = far_clip * 0.6f;
        view_distance_set_[num].sky_box_scale = math::vector3(sky_box_scale, sky_box_scale, sky_box_scale);
        view_distance_set_[num].far_clip = far_clip;

        if (num == view_distance_num_)
            select_view_distance_num(num);
    }

    auto python_background::far_clip() -> float {
        if (!map_)
            return 50000.0f;

        if (view_distance_set_[view_distance_num_].far_clip == 0.0f) {
            metin2::log::warn("View distance {} has no far clip; 25600 is used", view_distance_num_);
            view_distance_set_[view_distance_num_].far_clip = 25600.0f;
        }

        return view_distance_set_[view_distance_num_].far_clip;
    }

    void python_background::distance_set_info(int *pe_num, float *start, float *end, float *far_clip) {
        if (!map_) {
            *pe_num = 4;
            *start = 10000.0f;
            *end = 15000.0f;
            *far_clip = 50000.0f;
            return;
        }
        *pe_num = view_distance_num_;
        *start = view_distance_set_[view_distance_num_].fog_start;
        *end = view_distance_set_[view_distance_num_].fog_end;
        *far_clip = view_distance_set_[view_distance_num_].far_clip;
    }

    python_background::python_background() {
        render_shadow_time_ = 0;
        view_distance_num_ = 0;
        view_distance_num_ = 0;
        view_distance_num_ = 0;
        shadow_level_ = SHADOW_NONE;
        base_x_ = 0;
        base_y_ = 0;
        map_name_ = "";
        day_mode_ = DAY_MODE_LIGHT;
        x_mas_tree_grade_ = 0;
        visible_guild_area_ = false;

        set_view_distance_set(4, 25600.0f);
        set_view_distance_set(3, 25600.0f);
        set_view_distance_set(2, 25600.0f);
        set_view_distance_set(1, 25600.0f);
        set_view_distance_set(0, 25600.0f);
        initialize();
    }

    python_background::~python_background() {
        metin2::log::debug("Clearing the background");
    }

    void python_background::initialize() {
        set_atlas_info_file_name(std::format("{}/AtlasInfo.txt", LOCALE_DIR).c_str());
        map_manager::initialize();
    }

    void python_background::create_property() {
        get_property_manager().initialize();
    }

    auto python_background::picking_point(math::vector3 *intersect_pt) -> bool {
        map_outdoor &map = map_outdoor_ref();
        return map.picking_point(intersect_pt);
    }

    auto python_background::picking_point_with_ray(const ray &ray, math::vector3 *intersect_pt) -> bool {
        map_outdoor &map = map_outdoor_ref();
        return map.picking_point_with_ray(ray, intersect_pt);
    }

    auto python_background::picking_point_with_ray_only_terrain(const ray &ray, math::vector3 *intersect_pt) -> bool {
        map_outdoor &map = map_outdoor_ref();
        return map.picking_point_with_ray_only_terrain(ray, intersect_pt);
    }

    void python_background::destroy() {
        map_manager::destroy();
        snow_environment_value_.destroy();
        visible_guild_area_ = false;
    }

    void python_background::create() {
        static int is_create_property = false;

        if (!is_create_property) {
            is_create_property = true;
            create_property();
        }

        map_manager::create();

        snow_environment_value_.create();
    }

    struct get_portal_id_fn {
        float request_x, request_y;
        std::set<int> set_portal_id;
        get_portal_id_fn(float request_x_value, float request_y_value) {
            request_x = request_x_value;
            request_y = request_y_value;
        }
        void operator()(graphic_object_instance *object) {
            for (int i = 0; i < PORTAL_ID_MAX_NUM; ++i) {
                int id = object->portal(i);
                if (0 == id)
                    break;

                set_portal_id.insert(id);
            }
        }
    };

    void python_background::update(float center_x, float center_y, float center_z) {
        if (!is_map_ready())
            return;
        update_map(center_x, center_y, center_z);
        update_around_ambience(center_x, center_y, center_z);
        snow_environment_value_.update(math::vector3(center_x, -center_y, center_z));

        map_outdoor &map_value = map_outdoor_ref();
        if (map_value.is_enable_portal()) {
            culling_manager &culling_mgr = get_culling_manager();
            get_portal_id_fn get_portal_id(center_x, -center_y);

            vector3d vector3d_value;
            vector3d_value.set(center_x, -center_y, center_z);

            vector3d to_top;
            to_top.set(0, 0, 25000.0f);

            culling_mgr.for_in_ray(vector3d_value, to_top, &get_portal_id);

            if (!is_same(get_portal_id.set_portal_id, set_showing_portal_id_)) {
                clear_portal();
                auto itor = get_portal_id.set_portal_id.begin();
                for (; itor != get_portal_id.set_portal_id.end(); ++itor) {
                    add_showing_portal_id(*itor);
                }
                refresh_portal();

                set_showing_portal_id_ = get_portal_id.set_portal_id;
            }
        }

        {
            auto itor = map_target_id_chr_id_.begin();
            for (; itor != map_target_id_chr_id_.end(); ++itor) {
                uint32_t target_id = itor->first;
                uint32_t chr_id = itor->second;

                instance_base *instance_value = get_python_character_manager().instance_ptr(chr_id);

                if (!instance_value)
                    continue;

                pixel_position pixel_position_value;
                instance_value->new_get_pixel_position(&pixel_position_value);

                create_special_effect(target_id,
                    +pixel_position_value.x,
                    -pixel_position_value.y,
                    +pixel_position_value.z,
                    effect_name.c_str());
            }
        }

        {
            auto itor = map_id_reserve_target_effect_.begin();
            for (; itor != map_id_reserve_target_effect_.end();) {
                uint32_t id = itor->first;
                reserve_target_effect &reserve_target_effect_value = itor->second;

                auto ilx = float(reserve_target_effect_value.ilx);
                auto ily = float(reserve_target_effect_value.ily);

                float height_value = map_value.height(ilx, ily);
                if (0.0f == height_value) {
                    ++itor;
                    continue;
                }

                create_special_effect(id, ilx, ily, height_value, effect_name.c_str());

                itor = map_id_reserve_target_effect_.erase(itor);
            }
        }
    }

    auto python_background::is_same(std::set<int> &rleft, std::set<int> &rright) -> bool {
        std::set<int>::iterator itor_l;
        std::set<int>::iterator itor_r;

        for (itor_l = rleft.begin(); itor_l != rleft.end(); ++itor_l) {
            if (rright.end() == rright.find(*itor_l))
                return false;
        }

        for (itor_r = rright.begin(); itor_r != rright.end(); ++itor_r) {
            if (rleft.end() == rleft.find(*itor_r))
                return false;
        }

        return true;
    }

    void python_background::render() {
        if (!is_map_ready())
            return;

        snow_environment_value_.deform();

        map_outdoor &map = map_outdoor_ref();
        map.render();
        if (visible_guild_area_)
            map.render_marked_area();
    }

    void python_background::render_snow() {
        snow_environment_value_.render();
    }

    void python_background::render_pc_blocker() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.render_pc_blocker();
    }

    void python_background::render_collision() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.render_collision();
    }

    void python_background::render_character_shadow_to_texture() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        uint32_t t1 = el_timer_get_m_sec();

        if (shadow_level_ == SHADOW_ALL ||
            shadow_level_ == SHADOW_ALL_HIGH ||
            shadow_level_ == SHADOW_ALL_MAX ||
            shadow_level_ == SHADOW_GROUND_AND_SOLO) {
            math::matrix world;
            STATEMANAGER.get_transform(gpu::transform::WORLD, &world);

            bool can_render = map.begin_render_character_shadow_to_texture();
            if (can_render) {
                python_character_manager &chr_mgr = get_python_character_manager();

                if (shadow_level_ == SHADOW_GROUND_AND_SOLO)
                    chr_mgr.render_shadow_main_instance();
                else
                    chr_mgr.render_shadow_all_instances();
            }
            map.end_render_character_shadow_to_texture();

            STATEMANAGER.set_transform(gpu::transform::WORLD, &world);
        }

        uint32_t t2 = el_timer_get_m_sec();

        render_shadow_time_ = t2 - t1;
    }

    inline auto interpolate(float start, float end, float percent) -> float {
        return start + (end - start) * percent;
    }
    struct collision_checker {
        bool is_blocked;
        instance_base *instance;
        collision_checker(instance_base *instance_value) : is_blocked(false), instance(instance_value) {}
        void operator()(graphic_object_instance *opponent) {
            if (is_blocked)
                return;

            if (!opponent)
                return;

            if (instance->is_block_object(*opponent))
                is_blocked = true;
        }
    };

    struct collision_adjust_checker {
        bool is_blocked;
        instance_base *instance;
        collision_adjust_checker(instance_base *instance_value) : is_blocked(false), instance(instance_value) {}
        void operator()(graphic_object_instance *opponent) {
            if (!opponent)
                return;

            if (instance->avoid_object(*opponent))
                is_blocked = true;
        }
    };

    auto python_background::check_advancing(instance_base *instance) -> bool {
        if (!is_map_ready())
            return true;

        vector3d center;
        float radius;
        instance->graphic_thing_instance_ref().bounding_sphere(center, radius);

        culling_manager &culling_mgr = get_culling_manager();

        collision_adjust_checker collision_adjust_checker(instance);
        culling_mgr.for_in_range(center, radius, &collision_adjust_checker);
        if (collision_adjust_checker.is_blocked) {
            collision_checker collision_checker(instance);
            culling_mgr.for_in_range(center, radius, &collision_checker);
            if (collision_checker.is_blocked) {
                instance->block_movement();
                return true;
            } else {
                instance->new_move_to_dest_pixel_position_direction(instance->new_get_dst_pixel_position_ref());
            }
            return false;
        }
        return false;
    }

    void python_background::render_sky() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.render_sky();
    }

    void python_background::render_cloud() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.render_cloud();
    }

    void python_background::render_water() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.render_water();
    }

    void python_background::render_effect() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.render_effect();
    }

    void python_background::render_before_lens_flare() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.render_before_lens_flare();
    }

    void python_background::render_after_lens_flare() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.render_after_lens_flare();
    }

    void python_background::clear_guild_area() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.clear_guild_area();
    }

    void python_background::register_guild_area(int isx, int isy, int iex, int iey) {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.register_guild_area(isx, isy, iex, iey);
    }

    void python_background::set_character_dir_light() {
        if (!is_map_ready())
            return;

        if (!mc_pcur_environment_data_)
            return;

        STATEMANAGER.set_light(0, &mc_pcur_environment_data_->dir_lights[ENV_DIRLIGHT_CHARACTER]);
    }

    void python_background::set_background_dir_light() {
        if (!is_map_ready())
            return;
        if (!mc_pcur_environment_data_)
            return;

        STATEMANAGER.set_light(0, &mc_pcur_environment_data_->dir_lights[ENV_DIRLIGHT_BACKGROUND]);
    }

    void python_background::global_position_to_local_position(int32_t &global_x, int32_t &global_y) {
        global_x -= base_x_;
        global_y -= base_y_;
    }

    void python_background::local_position_to_global_position(int32_t &local_x, int32_t &local_y) {
        local_x += base_x_;
        local_y += base_y_;
    }

    void python_background::register_dungeon_map_name(const char *map_name) {
        set_dungeon_map_name_.insert(map_name);
    }

    auto python_background::global_position_to_map_info(uint32_t global_x, uint32_t global_y) -> python_background::map_info * {
        auto f = std::find_if(vct_map_info_.begin(), vct_map_info_.end(), find_warp_map_name_fn(global_x, global_y));
        if (f == vct_map_info_.end())
            return nullptr;

        return &(*f);
    }

    void python_background::warp(uint32_t x, uint32_t y) {
        map_info *pk_map_info = global_position_to_map_info(x, y);
        if (!pk_map_info) {
            metin2::log::error("No map contains the global position ({}, {})", x, y);
            return;
        }

        refresh_shadow_level();
        refresh_terrain_options();
        map_info &r_map_info = *pk_map_info;
        assert((x >= r_map_info.base_x) && (y >= r_map_info.base_y));

        if (!load_map(r_map_info.name_value, float(x - r_map_info.base_x), float(y - r_map_info.base_y), 0)) {
            window::request_quit();
            return;
        }

        get_python_mini_map().load_atlas();

        base_x_ = r_map_info.base_x;
        base_y_ = r_map_info.base_y;

        map_name_ = r_map_info.name_value;

        set_x_max_tree(x_mas_tree_grade_);

        if (set_dungeon_map_name_.end() != set_dungeon_map_name_.find(map_name_)) {
            enable_terrain_only_for_height();

            map_outdoor &map = map_outdoor_ref();
            map.enable_portal(true);
        }

        set_showing_portal_id_.clear();
        map_target_id_chr_id_.clear();
        map_id_reserve_target_effect_.clear();
    }

    void python_background::visible_guild_area() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.visible_marked_area();

        visible_guild_area_ = true;
    }

    void python_background::disable_guild_area() {
        if (!is_map_ready())
            return;

        map_outdoor &map = map_outdoor_ref();
        map.disable_marked_area();

        visible_guild_area_ = false;
    }

    auto python_background::warp_map_name() -> const char * {
        return map_name_.c_str();
    }

    void python_background::enable_snow_environment() {
        snow_environment_value_.enable();
    }

    void python_background::disable_snow_environment() {
        snow_environment_value_.disable();
    }

    const math::vector3 c_v3TreePos = math::vector3(76500.0f, -60900.0f, 20215.0f);

    void python_background::set_x_max_tree(int grade) {
        if (!map_)
            return;

        assert(grade >= 0 && grade <= 3);
        x_mas_tree_grade_ = grade;

        map_outdoor &map = map_outdoor_ref();

        if ("map_n_snowm_01" != map_name_) {
            map.x_mas_tree_destroy();
            return;
        }

        if (0 == grade) {
            map.x_mas_tree_destroy();
            return;
        }

        grade -= 1;
        grade = std::max(grade, 0);
        grade = std::min(grade, 2);

        static std::string tree_name[3] = {
            "d:/ymir work/tree/christmastree1.spt",
            "d:/ymir work/tree/christmastree2.spt",
            "d:/ymir work/tree/christmastree3.spt"};
        static std::string effect_name_value[3] = {
            "d:/ymir work/effect/etc/christmas_tree/tree_1s.mse",
            "d:/ymir work/effect/etc/christmas_tree/tree_2s.mse",
            "d:/ymir work/effect/etc/christmas_tree/tree_3s.mse",
        };
        map.x_mas_tree_set(c_v3TreePos.x, c_v3TreePos.y, c_v3TreePos.z, tree_name[grade].c_str(), effect_name_value[grade].c_str());
    }

    void python_background::create_target_effect(uint32_t id, uint32_t chr_vid) {
        map_target_id_chr_id_.insert(std::make_pair(id, chr_vid));
    }

    void python_background::create_target_effect(uint32_t id, int32_t lx, int32_t ly) {
        if (map_target_id_chr_id_.end() != map_target_id_chr_id_.find(id))
            return;

        map_outdoor &map = map_outdoor_ref();

        uint32_t base_x;
        uint32_t base_y;
        map.base_xy(&base_x, &base_y);

        int ilx = +(lx - int(base_x));
        int ily = -(ly - int(base_y));

        float height = map.height(float(ilx), float(ily));

        if (0.0f == height) {
            reserve_target_effect reserve_target_effect_value;
            reserve_target_effect_value.ilx = ilx;
            reserve_target_effect_value.ily = ily;
            map_id_reserve_target_effect_.insert(std::make_pair(id, reserve_target_effect_value));
            return;
        }

        create_special_effect(id, ilx, ily, height, effect_name.c_str());
    }

    void python_background::delete_target_effect(uint32_t id) {
        if (map_id_reserve_target_effect_.end() != map_id_reserve_target_effect_.find(id)) {
            map_id_reserve_target_effect_.erase(id);
        }
        if (map_target_id_chr_id_.end() != map_target_id_chr_id_.find(id)) {
            map_target_id_chr_id_.erase(id);
        }

        delete_special_effect(id);
    }

    void python_background::create_special_effect(uint32_t id, float fx, float fy, float fz, const char *file_name) {
        map_outdoor &map = map_outdoor_ref();
        map.special_effect_create(id, fx, fy, fz, file_name);
    }

    void python_background::delete_special_effect(uint32_t id) {
        map_outdoor &map = map_outdoor_ref();
        map.special_effect_delete(id);
    }
}
