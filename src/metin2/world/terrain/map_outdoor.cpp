#include "map_outdoor.h"

#include "area_terrain.h"
#include "terrain_patch.h"

#include <metin2/core/crc32.h>
#include <metin2/core/utils.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/state_manager.h>

#include <algorithm>

namespace metin2::inline world::inline terrain {
    struct get_object_height_fn {
        bool height_found;
        float return_height;
        float request_x, request_y;

        get_object_height_fn(float request_x_value, float request_y_value) {
            request_x = request_x_value;
            request_y = request_y_value;
            height_found = false;
            return_height = 0.0f;
        }

        void operator()(graphic_object_instance *object) {
            if (object->object_height(request_x, request_y, &return_height)) {
                height_found = true;
            }
        }
    };

    struct get_picking_point_fn {
        math::vector3 start;
        math::vector3 dir_value;
        math::vector3 picking_point;
        bool picked;

        get_picking_point_fn(math::vector3 &start_value, math::vector3 &dir) : start(start_value), dir_value(dir), picked(false) {}
        void operator()(graphic_object_instance *instance) {
            if (instance && instance->get_type() == graphic_thing_instance::ID) {
                auto *thing = (graphic_thing_instance *)instance;
                if (!thing->is_object_height()) {
                    return;
                }

                float x, y, z;
                if (thing->picking(start, dir_value, x, y)) {
                    if (thing->object_height(x, -y, &z)) {
                        picking_point.x = x;
                        picking_point.y = y;
                        picking_point.z = z;
                        picked = true;
                    }
                }
            }
        }
    };

    map_outdoor::map_outdoor() {
        auto *alpha_fog_image = (graphic_image *)get_resource_manager().resource_pointer("D:/ymir work/special/fog.tga");
        auto *attr_image = (graphic_image *)get_resource_manager().resource_pointer("d:/ymir work/special/white.dds");
        auto *build_transparent_image = (graphic_image *)get_resource_manager().resource_pointer("d:/ymir Work/special/PCBlockerAlpha.dds");
        alpha_fog_image_instance_.set_image_pointer(alpha_fog_image);
        attr_image_instance_.set_image_pointer(attr_image);
        building_transparent_image_instance_.set_image_pointer(build_transparent_image);

        initialize();
    }

    map_outdoor::~map_outdoor() {
        destroy();
    }

    auto map_outdoor::initialize() -> bool {
        uint8_t i;
        for (i = 0; i < AROUND_AREA_NUM; ++i) {
            area_value_[i] = nullptr;
            terrain_[i] = nullptr;
        }

        terrain_patch_proxy_list_.reset();

        view_radius_ = 0;
        height_scale_ = 0.0f;

        terrain_count_x_ = terrain_count_y_ = 0;

        cur_coordinate_.terrain_coord_x = -1;
        cur_coordinate_.terrain_coord_y = -1;
        prev_coordinate_.terrain_coord_x = -1;
        prev_coordinate_.terrain_coord_y = -1;

        entry_point_map_.clear();

        center_x_ = center_y_ = 0;
        old_read_x_ = old_read_y_ = -1;

        memset(pwa_indices_, 0, sizeof(pwa_indices_));
        for (i = 0; i < TERRAINPATCH_LODMAX; ++i) {
            index_buffer_[i].destroy();
        }

        setting_terrain_visible_ = false;
        draw_shadow_ = false;
        draw_chr_shadow_ = false;

        splat_limit_ = 50000;

        patch_count_ = 0;

        root_node_ = nullptr;

        rendered_patch_num_ = 0;
        rendered_splat_num_ = 0;

        opaque_water_depth_ = 400.0f;

        terrain_vector_.clear();
        terrain_delete_vector_.clear();
        terrain_load_request_vector_.clear();
        terrain_load_wait_vector_.clear();

        area_vector_.clear();
        area_delete_vector_.clear();
        area_load_request_vector_.clear();
        area_load_wait_vector_.clear();

        patch_vector_.clear();

        terrain_render_sort_value_ = DISTANCE_SORT;

        world_for_common_use_ = math::matrix_identity();

        initialize_fog();
        initialize_visible_parts();

        base_x_ = 0;
        base_y_ = 0;

        settings_env_data_name_ = "";
        show_entire_patch_texture_count_ = false;
        transparent_tree_ = true;

        map_base::clear();

        x_mas_tree_initialize();
        special_effect_destroy();

        enable_terrain_only_for_height_ = false;
        enable_portal_ = false;

        shadow_map_size_ = 512;
        return true;
    }

    auto map_outdoor::destroy() -> bool {
        enable_terrain_only_for_height_ = false;
        enable_portal_ = false;

        x_mas_tree_destroy();

        destroy_terrain();
        destroy_area();
        destroy_terrain_patch_proxy_list();

        free_quad_tree();
        release_character_shadow_texture();

        terrain_area::destroy_system();
        area::destroy_system();

        list_guild_area_.clear();
        alpha_fog_image_instance_.destroy();

        get_speed_tree_forest_direct_x8().clear();

        return true;
    }

    void map_outdoor::clear() {
        unload_water_texture();
        destroy();
        initialize();
    }

    auto map_outdoor::set_terrain_count(short terrain_count_x, short terrain_count_y) -> bool {
        if (0 == terrain_count_x || MAX_MAPSIZE < terrain_count_x) {
            return false;
        }

        if (0 == terrain_count_y || MAX_MAPSIZE < terrain_count_y) {
            return false;
        }

        terrain_count_x_ = terrain_count_x;
        terrain_count_y_ = terrain_count_y;
        return true;
    }

    void map_outdoor::on_begin_environment() {
        if (!mc_environment_data_) {
            return;
        }

        auto &forest = get_speed_tree_forest_direct_x8();
        forest.set_fog(
            mc_environment_data_->get_fog_near_distance(),
            mc_environment_data_->get_fog_far_distance());

        const auto &light = mc_environment_data_->dir_lights[ENV_DIRLIGHT_CHARACTER];

        forest.set_light(
            (const float *)&light.direction,
            (const float *)&light.ambient,
            (const float *)&light.diffuse);
    }

    void map_outdoor::on_set_environment_data_ptr() {
        set_environment_screen_filter();
        set_environment_sky_box();
        set_environment_lens_flare();
    }

    void map_outdoor::on_reset_environment_data_ptr() {
        sky_box_value_.unload();
        set_environment_screen_filter();
        set_environment_sky_box();
        set_environment_lens_flare();
    }

    void map_outdoor::set_environment_screen_filter() {
        if (!mc_environment_data_) {
            return;
        }

        screen_filter_value_.set_enable(mc_environment_data_->filtering_enable);
        screen_filter_value_.set_blend_type(mc_environment_data_->filtering_alpha_src, mc_environment_data_->filtering_alpha_dest);
        screen_filter_value_.set_color(mc_environment_data_->filtering_color);
    }

    void map_outdoor::set_environment_sky_box() {
        if (!mc_environment_data_) {
            return;
        }

        sky_box_value_.set_sky_box_scale(mc_environment_data_->sky_box_scale);
        sky_box_value_.set_gradient_level(mc_environment_data_->sky_box_gradient_level_upper, mc_environment_data_->sky_box_gradient_level_lower);
        sky_box_value_.set_render_mode((mc_environment_data_->sky_box_texture_render_mode == true) ? sky_object::SKY_RENDER_MODE_TEXTURE : sky_object::SKY_RENDER_MODE_DIFFUSE);

        for (int i = 0; i < 6; ++i) {
            if (!mc_environment_data_->sky_box_face_file_name[i].empty()) {
                sky_box_value_.set_face_texture(mc_environment_data_->sky_box_face_file_name[i].c_str(), i);
            }
        }

        if (!mc_environment_data_->cloud_texture_file_name.empty()) {
            sky_box_value_.set_cloud_texture(mc_environment_data_->cloud_texture_file_name.c_str());
        }

        sky_box_value_.set_cloud_scale(mc_environment_data_->cloud_scale);
        sky_box_value_.set_cloud_height(mc_environment_data_->cloud_height);
        sky_box_value_.set_cloud_texture_scale(mc_environment_data_->cloud_texture_scale);
        sky_box_value_.set_cloud_scroll_speed(mc_environment_data_->cloud_speed);
        sky_box_value_.refresh();

        sky_box_value_.set_cloud_color(mc_environment_data_->cloud_gradient_color, mc_environment_data_->cloud_gradient_color, 1);

        if (!mc_environment_data_->sky_box_gradient_color_vector.empty()) {
            sky_box_value_.set_sky_color(mc_environment_data_->sky_box_gradient_color_vector, mc_environment_data_->sky_box_gradient_color_vector, 1);
        }

        sky_box_value_.start_transition();
    }

    void map_outdoor::set_environment_lens_flare() {
        if (!mc_environment_data_) {
            return;
        }

        lens_flare_value_.characterize_flare(mc_environment_data_->lens_flare_enable == 1 ? true : false,
            mc_environment_data_->main_flare_enable == 1 ? true : false,
            mc_environment_data_->lens_flare_max_brightness,
            mc_environment_data_->lens_flare_brightness_color);

        lens_flare_value_.initialize("d:/ymir work/environment");

        if (!mc_environment_data_->main_flare_texture_file_name.empty()) {
            lens_flare_value_.set_main_flare(mc_environment_data_->main_flare_texture_file_name.c_str(),
                mc_environment_data_->main_flare_size);
        }
    }

    void map_outdoor::create_terrain_patch_proxy_list() {
        patch_count_ = ((view_radius_ * 2) / TERRAIN_PATCHSIZE) + 2;

        terrain_patch_proxy_list_ = std::make_unique<terrain_patch_proxy[]>(patch_count_ * patch_count_);

        patch_terrain_vertex_count_ = (TERRAIN_PATCHSIZE + 1) * (TERRAIN_PATCHSIZE + 1);
        patch_water_vertex_count_ = TERRAIN_PATCHSIZE * TERRAIN_PATCHSIZE * 6;
        patch_terrain_vertex_size_ = 24;
        patch_water_vertex_size_ = 16;

        set_index_buffer();
    }

    void map_outdoor::destroy_terrain_patch_proxy_list() {
        if (terrain_patch_proxy_list_) {
            terrain_patch_proxy_list_.reset();
            terrain_patch_proxy_list_.reset();
        }

        for (int i = 0; i < TERRAINPATCH_LODMAX; ++i) {
            index_buffer_[i].destroy();
        }
    }

    void map_outdoor::enable_portal(bool flag) {
        enable_portal_ = flag;

        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            if (area_value_[i]) {
                area_value_[i]->enable_portal(flag);
            }
        }
    }

    void map_outdoor::destroy_area() {
        area_vector_.clear();
        area_delete_vector_.clear();

        area::pool_value.free_all();

        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            area_value_[i] = nullptr;
        }
    }

    void map_outdoor::destroy_terrain() {
        terrain_vector_.clear();
        terrain_delete_vector_.clear();

        terrain_area::pool_value.free_all();
        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            terrain_[i] = nullptr;
        }
    }

    auto map_outdoor::terrain_num(float fx, float fy, uint8_t *terrain_num) -> bool {
        if (fy < 0) {
            fy = -fy;
        }

        int ix, iy;

        PR_FLOAT_TO_INT(fx, ix);
        PR_FLOAT_TO_INT(fy, iy);

        uint16_t terrain_num_x = ix / (terrain_impl::TERRAIN_XSIZE);
        uint16_t terrain_num_y = iy / (terrain_impl::TERRAIN_YSIZE);

        return terrain_num_from_coord(terrain_num_x, terrain_num_y, terrain_num);
    }

    auto map_outdoor::picking_point(math::vector3 *intersect_pt) -> bool {
        return picking_point_with_ray(ray_value_, intersect_pt);
    }

    auto map_outdoor::pick_terrain_height(float &pos_value, const math::vector3 &start_value, const math::vector3 &end_value, float step, float ray_range, float limit_range, math::vector3 *pick_value) -> bool {
        terrain_area *terrain;

        math::vector3 cur_pos_value;

        float ray_range_inv = 1.0f / ray_range;
        while (pos_value < ray_range && pos_value < limit_range) {
            cur_pos_value = math::lerp(start_value, end_value, pos_value * ray_range_inv);
            uint8_t terrain_num_value;
            float multiplier = 1.0f;
            if (terrain_num(cur_pos_value.x, cur_pos_value.y, &terrain_num_value)) {
                if (terrain_pointer(terrain_num_value, &terrain)) {
                    int ix, iy;
                    PR_FLOAT_TO_INT(cur_pos_value.x, ix);
                    PR_FLOAT_TO_INT(fabs(cur_pos_value.y), iy);
                    float map_height = terrain->height(ix, iy);
                    if (map_height >= cur_pos_value.z) {
                        *pick_value = cur_pos_value;
                        return true;
                    } else {
                        multiplier = f_max(1.0f, 0.01f * (cur_pos_value.z - map_height));
                    }
                }
            }
            pos_value += step * multiplier;
        }

        return false;
    }
    auto map_outdoor::picking_point_with_ray(const ray &ray, math::vector3 *intersect_pt) -> bool {
        bool b_object_pick = false;
        bool b_terrain_pick = false;
        math::vector3 v3_object_pick, v3_terrain_pick;

        math::vector3 start, end, dir;
        float ray_range;

        ray.start_point(&start);
        ray.get_direction(&dir, &ray_range);
        ray.end_point(&end);

        vector3d v3d_start, v3d_end;
        v3d_start.set(start.x, start.y, start.z);
        v3d_end.set(end.x - start.x, end.y - start.y, end.z - start.z);

        if (!enable_terrain_only_for_height_) {
            culling_manager &culling_mgr = get_culling_manager();
            get_picking_point_fn get_picking_point(start, dir);
            culling_mgr.for_in_range2d(v3d_start, &get_picking_point);

            if (get_picking_point.picked) {
                b_object_pick = true;
                v3_object_pick = get_picking_point.picking_point;
            }
        }

        float pos = 0.0f;

        b_terrain_pick = true;
        if (!pick_terrain_height(pos, start, end, 5.0f, ray_range, 5000.0f, &v3_terrain_pick)) {
            if (!pick_terrain_height(pos, start, end, 10.0f, ray_range, 10000.0f, &v3_terrain_pick)) {
                if (!pick_terrain_height(pos, start, end, 100.0f, ray_range, 100000.0f, &v3_terrain_pick)) {
                    b_terrain_pick = false;
                }
            }
        }

        if (b_object_pick && b_terrain_pick) {
            const auto vv = (v3_terrain_pick - start);
            const auto vv2 = (v3_object_pick - start);
            if (math::length(vv2) >= math::length(vv)) {
                *intersect_pt = v3_terrain_pick;
            } else {
                *intersect_pt = v3_object_pick;
            }
            return true;
        } else if (b_object_pick) {
            *intersect_pt = v3_object_pick;
            return true;
        } else if (b_terrain_pick) {
            *intersect_pt = v3_terrain_pick;
            return true;
        }

        return false;
    }

    auto map_outdoor::picking_point_with_ray_only_terrain(const ray &ray, math::vector3 *intersect_pt) -> bool {
        bool b_terrain_pick = false;
        math::vector3 v3_terrain_pick;

        math::vector3 start, end, dir;
        float ray_range;
        ray.start_point(&start);
        ray.get_direction(&dir, &ray_range);
        ray.end_point(&end);

        vector3d v3d_start, v3d_end;
        v3d_start.set(start.x, start.y, start.z);
        v3d_end.set(end.x - start.x, end.y - start.y, end.z - start.z);

        float pos = 0.0f;
        b_terrain_pick = true;
        if (!pick_terrain_height(pos, start, end, 5.0f, ray_range, 5000.0f, &v3_terrain_pick)) {
            if (!pick_terrain_height(pos, start, end, 10.0f, ray_range, 10000.0f, &v3_terrain_pick)) {
                if (!pick_terrain_height(pos, start, end, 100.0f, ray_range, 100000.0f, &v3_terrain_pick)) {
                    b_terrain_pick = false;
                }
            }
        }

        if (b_terrain_pick) {
            *intersect_pt = v3_terrain_pick;
            return true;
        }

        return false;
    }

    void map_outdoor::height_map(const uint8_t &ruc_terrain_num, uint16_t **height_map) {
        if (ruc_terrain_num > AROUND_AREA_NUM - 1 || !terrain_[ruc_terrain_num]) {
            *height_map = nullptr;
            return;
        }

        *height_map = terrain_[ruc_terrain_num]->height_map();
    }

    void map_outdoor::normal_map(const uint8_t &ruc_terrain_num, char **puc_normal_map) {
        if (ruc_terrain_num > AROUND_AREA_NUM - 1 || !terrain_[ruc_terrain_num]) {
            *puc_normal_map = nullptr;
            return;
        }

        *puc_normal_map = terrain_[ruc_terrain_num]->normal_map();
    }

    void map_outdoor::water_map(const uint8_t &ruc_terrain_num, uint8_t **puc_water_map) {
        if (ruc_terrain_num > AROUND_AREA_NUM - 1 || !terrain_[ruc_terrain_num]) {
            *puc_water_map = nullptr;
            return;
        }

        *puc_water_map = terrain_[ruc_terrain_num]->water_map();
    }

    void map_outdoor::water_height(uint8_t terrain_num, uint8_t water_num, int32_t *pl_water_height) {
        if (terrain_num > AROUND_AREA_NUM - 1 || !terrain_[terrain_num]) {
            *pl_water_height = -1;
            return;
        }

        terrain_[terrain_num]->water_height(water_num, pl_water_height);
    }

    auto map_outdoor::water_height(int x, int y, int32_t *pl_water_height) -> bool {
        if (x < 0 || y < 0 || x > terrain_count_x_ * terrain_impl::TERRAIN_XSIZE || y > terrain_count_y_ * terrain_impl::TERRAIN_YSIZE) {
            return false;
        }

        uint16_t terrain_coord_x, terrain_coord_y;
        terrain_coord_x = x / terrain_impl::TERRAIN_XSIZE;
        terrain_coord_y = y / terrain_impl::TERRAIN_YSIZE;

        uint8_t terrain_num;
        if (!terrain_num_from_coord(terrain_coord_x, terrain_coord_y, &terrain_num)) {
            return false;
        }

        terrain_area *terrain;
        if (!terrain_pointer(terrain_num, &terrain)) {
            return false;
        }

        // The water map has one entry per terrain cell.
        const auto local_x = static_cast<uint16_t>(std::min<int>((x - terrain_coord_x * terrain_impl::TERRAIN_XSIZE) / terrain_impl::CELLSCALE, terrain_impl::WATERMAP_XSIZE - 1));
        const auto local_y = static_cast<uint16_t>(std::min<int>((y - terrain_coord_y * terrain_impl::TERRAIN_YSIZE) / terrain_impl::CELLSCALE, terrain_impl::WATERMAP_YSIZE - 1));

        return terrain->water_height(local_x, local_y, pl_water_height);
    }

    auto map_outdoor::terrain_num_from_coord(uint16_t coord_x, uint16_t coord_y, uint8_t *terrain_num) -> bool {
        *terrain_num = (coord_y - cur_coordinate_.terrain_coord_y + LOAD_SIZE_WIDTH) * 3 +
                       (coord_x - cur_coordinate_.terrain_coord_x + LOAD_SIZE_WIDTH);

        if (*terrain_num > AROUND_AREA_NUM) {
            return false;
        }

        return true;
    }

    void map_outdoor::build_view_frustum(math::matrix &mat) {
        plane_[0] = math::plane(mat._13, mat._23, mat._33, mat._43);                                         // Near
        plane_[1] = math::plane(mat._14 - mat._13, mat._24 - mat._23, mat._34 - mat._33, mat._44 - mat._43); // Far
        plane_[2] = math::plane(mat._14 + mat._11, mat._24 + mat._21, mat._34 + mat._31, mat._44 + mat._41); // Left
        plane_[3] = math::plane(mat._14 - mat._11, mat._24 - mat._21, mat._34 - mat._31, mat._44 - mat._41); // Right
        plane_[4] = math::plane(mat._14 + mat._12, mat._24 + mat._22, mat._34 + mat._32, mat._44 + mat._42); // Bottom
        plane_[5] = math::plane(mat._14 - mat._12, mat._24 - mat._22, mat._34 - mat._32, mat._44 - mat._42); // Top

        for (int i = 0; i < 6; ++i) {
            plane_[i] = math::plane_normalize(plane_[i]);
        }
    }

    auto map_outdoor::height(float fx, float fy) -> float {
        float terrain_height_value = terrain_height(fx, fy);

        if (!enable_terrain_only_for_height_) {
            auto &culling_manager = get_culling_manager();

            float check_height = 25000.0f;
            float object_height_value = -check_height;

            vector3d vector3d_value;
            vector3d_value.set(fx, -fy, terrain_height_value);

            get_object_height_fn get_obj_height(fx, fy);

            range_tester<get_object_height_fn> range_tester_get_obj_height(&get_obj_height);
            culling_manager.point_test2d(vector3d_value, &range_tester_get_obj_height);

            if (get_obj_height.height_found) {
                object_height_value = get_obj_height.return_height;
            }

            return f_max(object_height_value, terrain_height_value);
        }

        return terrain_height_value;
    }

    auto map_outdoor::get_normal(int x, int y, math::vector3 *normal) -> bool {
        if (x <= 0) {
            x = 0;
        } else if (x >= terrain_count_x_ * terrain_impl::TERRAIN_XSIZE) {
            x = terrain_count_x_ * terrain_impl::TERRAIN_XSIZE;
        }

        if (y <= 0) {
            y = 0;
        } else if (y >= terrain_count_y_ * terrain_impl::TERRAIN_YSIZE) {
            y = terrain_count_y_ * terrain_impl::TERRAIN_YSIZE;
        }

        uint16_t us_coord_x, us_coord_y;

        us_coord_x = (uint16_t)(x / (terrain_impl::TERRAIN_XSIZE));
        us_coord_y = (uint16_t)(y / (terrain_impl::TERRAIN_YSIZE));

        if (us_coord_x >= terrain_count_x_ - 1) {
            us_coord_x = terrain_count_x_ - 1;
        }

        if (us_coord_y >= terrain_count_y_ - 1) {
            us_coord_y = terrain_count_y_ - 1;
        }

        uint8_t terrain_num;
        if (!terrain_num_from_coord(us_coord_x, us_coord_y, &terrain_num)) {
            return false;
        }

        terrain_area *terrain;

        if (!terrain_pointer(terrain_num, &terrain)) {
            return false;
        }

        while (x >= terrain_impl::TERRAIN_XSIZE) {
            x -= terrain_impl::TERRAIN_XSIZE;
        }

        while (y >= terrain_impl::TERRAIN_YSIZE) {
            y -= terrain_impl::TERRAIN_YSIZE;
        }

        return terrain->get_normal(x, y, normal);
    }

    auto map_outdoor::terrain_height(float fx, float fy) -> float {
        if (fy < 0) {
            fy = -fy;
        }

        int32_t lx, ly;
        PR_FLOAT_TO_INT(fx, lx);
        PR_FLOAT_TO_INT(fy, ly);

        uint16_t us_coord_x, us_coord_y;

        us_coord_x = (uint16_t)(lx / terrain_impl::TERRAIN_XSIZE);
        us_coord_y = (uint16_t)(ly / terrain_impl::TERRAIN_YSIZE);

        uint8_t terrain_num;
        if (!terrain_num_from_coord(us_coord_x, us_coord_y, &terrain_num)) {
            return 0.0f;
        }

        terrain_area *terrain;

        if (!terrain_pointer(terrain_num, &terrain)) {
            return 0.0f;
        }

        return terrain->height(lx, ly);
    }

    auto map_outdoor::height(float *pos) -> float {
        pos[2] = height(pos[0], pos[1]);
        return pos[2];
    }

    auto map_outdoor::area_pointer(const uint8_t area_num, area **area) -> bool {
        if (area_num >= AROUND_AREA_NUM) {
            *area = nullptr;
            return false;
        }

        if (nullptr == area_value_[area_num]) {
            *area = nullptr;
            return false;
        }

        *area = area_value_[area_num];
        return true;
    }

    auto map_outdoor::terrain_pointer(const uint8_t terrain_num, terrain_area **terrain) -> bool {
        if (terrain_num >= AROUND_AREA_NUM) {
            *terrain = nullptr;
            return false;
        }

        if (nullptr == terrain_[terrain_num]) {
            *terrain = nullptr;
            return false;
        }

        *terrain = terrain_[terrain_num];
        return true;
    }

    void map_outdoor::initialize_fog() {
        memset(static_cast<void *>(&alpha_fog_texture_), 0, sizeof(math::matrix));
        alpha_fog_texture_._31 = -0.001f;
        alpha_fog_texture_._41 = -7.0f;
        alpha_fog_texture_._42 = 0.5f;
    }

    void map_outdoor::set_draw_shadow(bool draw_shadow) {
        draw_shadow_ = draw_shadow;
    }

    void map_outdoor::set_draw_character_shadow(bool draw_chr_shadow) {
        draw_chr_shadow_ = draw_chr_shadow;
    }

    auto map_outdoor::shadow_map_color(float fx, float fy) -> uint32_t {
        if (fy < 0) {
            fy = -fy;
        }

        auto terrain_size = (float)(terrain_impl::TERRAIN_XSIZE);
        float x_ref = fx - (float)(cur_coord_start_x_);
        float y_ref = fy - (float)(cur_coord_start_y_);

        terrain_area *terrain;

        if (y_ref < -terrain_size) {
            return 0xFFFFFFFF;
        } else if (y_ref >= -terrain_size && y_ref < 0.0f) {
            if (x_ref < -terrain_size) {
                return 0xFFFFFFFF;
            } else if (x_ref >= -terrain_size && x_ref < 0.0f) {
                if (terrain_pointer(0, &terrain)) {
                    return terrain->shadow_map_color(x_ref + terrain_size, y_ref + terrain_size);
                } else {
                    return 0xFFFFFFFF;
                }
            } else if (x_ref >= 0.0f && x_ref < terrain_size) {
                if (terrain_pointer(1, &terrain)) {
                    return terrain->shadow_map_color(x_ref, y_ref + terrain_size);
                } else {
                    return 0xFFFFFFFF;
                }
            } else if (x_ref >= terrain_size && x_ref < 2.0f * terrain_size) {
                if (terrain_pointer(2, &terrain)) {
                    return terrain->shadow_map_color(x_ref - terrain_size, y_ref + terrain_size);
                } else {
                    return 0xFFFFFFFF;
                }
            } else {
                return 0xFFFFFFFF;
            }
        } else if (y_ref >= 0.0f && y_ref < terrain_size) {
            if (x_ref < -terrain_size) {
                return 0xFFFFFFFF;
            } else if (x_ref >= -terrain_size && x_ref < 0.0f) {
                if (terrain_pointer(3, &terrain)) {
                    return terrain->shadow_map_color(x_ref + terrain_size, y_ref);
                } else {
                    return 0xFFFFFFFF;
                }
            } else if (x_ref >= 0.0f && x_ref < terrain_size) {
                if (terrain_pointer(4, &terrain)) {
                    return terrain->shadow_map_color(x_ref, y_ref);
                } else {
                    return 0xFFFFFFFF;
                }
            } else if (x_ref >= terrain_size && x_ref < 2.0f * terrain_size) {
                if (terrain_pointer(5, &terrain)) {
                    return terrain->shadow_map_color(x_ref - terrain_size, y_ref);
                } else {
                    return 0xFFFFFFFF;
                }
            } else {
                return 0xFFFFFFFF;
            }
        } else if (y_ref >= terrain_size && y_ref < 2.0f * terrain_size) {
            if (x_ref < -terrain_size) {
                return 0xFFFFFFFF;
            } else if (x_ref >= -terrain_size && x_ref < 0.0f) {
                if (terrain_pointer(6, &terrain)) {
                    return terrain->shadow_map_color(x_ref + terrain_size, y_ref - terrain_size);
                } else {
                    return 0xFFFFFFFF;
                }
            } else if (x_ref >= 0.0f && x_ref < terrain_size) {
                if (terrain_pointer(7, &terrain)) {
                    return terrain->shadow_map_color(x_ref, y_ref - terrain_size);
                } else {
                    return 0xFFFFFFFF;
                }
            } else if (x_ref >= terrain_size && x_ref < 2.0f * terrain_size) {
                if (terrain_pointer(8, &terrain)) {
                    return terrain->shadow_map_color(x_ref - terrain_size, y_ref - terrain_size);
                } else {
                    return 0xFFFFFFFF;
                }
            } else {
                return 0xFFFFFFFF;
            }
        } else {
            return 0xFFFFFFFF;
        }

        return 0xFFFFFFFF;
    }

    auto map_outdoor::is_attr_on(float f_x, float f_y, uint8_t attr) -> bool {
        int i_x, i_y;
        PR_FLOAT_TO_INT(f_x, i_x);
        PR_FLOAT_TO_INT(f_y, i_y);

        return is_attr_on(i_x, i_y, attr);
    }

    auto map_outdoor::attr(float f_x, float f_y, uint8_t *attr_value) -> bool {
        int i_x, i_y;
        PR_FLOAT_TO_INT(f_x, i_x);
        PR_FLOAT_TO_INT(f_y, i_y);

        return attr(i_x, i_y, attr_value);
    }

    auto map_outdoor::is_attr_on(int x, int y, uint8_t attr) -> bool {
        if (x < 0 || y < 0 || x > terrain_count_x_ * terrain_impl::TERRAIN_XSIZE || y > terrain_count_y_ * terrain_impl::TERRAIN_YSIZE) {
            return false;
        }

        uint16_t terrain_coord_x, terrain_coord_y;
        terrain_coord_x = x / terrain_impl::TERRAIN_XSIZE;
        terrain_coord_y = y / terrain_impl::TERRAIN_YSIZE;

        uint8_t terrain_num;
        if (!terrain_num_from_coord(terrain_coord_x, terrain_coord_y, &terrain_num)) {
            return false;
        }

        terrain_area *terrain;
        if (!terrain_pointer(terrain_num, &terrain)) {
            return false;
        }

        uint16_t local_x, local_y;
        local_x = (x - terrain_coord_x * terrain_impl::TERRAIN_XSIZE) / (terrain_impl::HALF_CELLSCALE);
        local_y = (y - terrain_coord_y * terrain_impl::TERRAIN_YSIZE) / (terrain_impl::HALF_CELLSCALE);

        return terrain->is_attr_on(local_x, local_y, attr);
    }

    auto map_outdoor::attr(int x, int y, uint8_t *pby_attr) -> bool {
        if (x < 0 || y < 0 || x > terrain_count_x_ * terrain_impl::TERRAIN_XSIZE || y > terrain_count_y_ * terrain_impl::TERRAIN_YSIZE) {
            return false;
        }

        uint16_t terrain_coord_x, terrain_coord_y;
        terrain_coord_x = x / terrain_impl::TERRAIN_XSIZE;
        terrain_coord_y = y / terrain_impl::TERRAIN_YSIZE;

        uint8_t terrain_num;
        if (!terrain_num_from_coord(terrain_coord_x, terrain_coord_y, &terrain_num)) {
            return false;
        }

        terrain_area *terrain;
        if (!terrain_pointer(terrain_num, &terrain)) {
            return false;
        }

        uint16_t local_x, local_y;
        local_x = (uint16_t)(x - terrain_coord_x * terrain_impl::TERRAIN_XSIZE) / (terrain_impl::HALF_CELLSCALE);
        local_y = (uint16_t)(y - terrain_coord_y * terrain_impl::TERRAIN_YSIZE) / (terrain_impl::HALF_CELLSCALE);

        uint8_t by_attr = terrain->attr(local_x, local_y);

        *pby_attr = by_attr;

        return true;
    }

    void map_outdoor::base_xy(uint32_t *base_x, uint32_t *base_y) {
        *base_x = base_x_;
        *base_y = base_y_;
    }

    void map_outdoor::set_base_xy(uint32_t base_x, uint32_t base_y) {
        base_x_ = base_x;
        base_y_ = base_y;
    }

    void map_outdoor::x_mas_tree_initialize() {
        x_mas_.tree = nullptr;
        x_mas_.effect_id = -1;
    }

    void map_outdoor::x_mas_tree_destroy() {
        if (x_mas_.tree) {
            auto &forest = get_speed_tree_forest_direct_x8();
            x_mas_.tree->clear();
            forest.delete_instance(x_mas_.tree);
            x_mas_.tree = nullptr;
        }

        if (-1 != x_mas_.effect_id) {
            auto &effect_manager = get_effect_manager();
            effect_manager.destroy_effect_instance(x_mas_.effect_id);
            x_mas_.effect_id = -1;
        }
    }

    void map_outdoor::x_mas_tree_create(float x, float y, float z, const char *tree_name, const char *eff_name) {
        assert(nullptr == x_mas_.tree);
        assert(-1 == x_mas_.effect_id);

        auto &forest = get_speed_tree_forest_direct_x8();
        auto crc32 = get_case_crc32(tree_name, strlen(tree_name));
        x_mas_.tree = forest.create_instance(x, y, z, crc32, tree_name);

        auto &effect_manager = get_effect_manager();
        effect_manager.register_effect(eff_name);
        x_mas_.effect_id = effect_manager.create_effect(eff_name,
            math::vector3(x, y, z),
            math::vector3(0.0f, 0.0f, 0.0f));
    }

    void map_outdoor::x_mas_tree_set(float x, float y, float z, const char *tree_name, const char *eff_name) {
        x_mas_tree_destroy();
        x_mas_tree_create(x, y, z, tree_name, eff_name);
    }

    void map_outdoor::special_effect_create(uint32_t id, float x, float y, float z, const char *eff_name) {
        auto &effect_manager = get_effect_manager();

        auto itor = map_id_effect_id_.find(id);
        if (map_id_effect_id_.end() != itor) {
            uint32_t effect_id = itor->second;

            if (effect_manager.select_effect_instance(effect_id)) {
                math::matrix mat;
                mat = math::matrix_identity();
                mat._41 = x;
                mat._42 = y;
                mat._43 = z;
                effect_manager.set_effect_instance_global_matrix(mat);
                return;
            }
        }

        effect_manager.register_effect(eff_name);
        uint32_t effect_id = effect_manager.create_effect(eff_name,
            math::vector3(x, y, z),
            math::vector3(0.0f, 0.0f, 0.0f));

        map_id_effect_id_.insert(std::make_pair(id, effect_id));
    }

    void map_outdoor::special_effect_delete(uint32_t id) {
        auto itor = map_id_effect_id_.find(id);

        if (map_id_effect_id_.end() == itor) {
            return;
        }

        effect_manager &eff_mgr = get_effect_manager();
        int effect_id = itor->second;
        eff_mgr.destroy_effect_instance(effect_id);
    }

    void map_outdoor::special_effect_destroy() {
        effect_manager &eff_mgr = get_effect_manager();

        auto itor = map_id_effect_id_.begin();
        for (; itor != map_id_effect_id_.end(); ++itor) {
            int effect_id = itor->second;
            eff_mgr.destroy_effect_instance(effect_id);
        }
    }

    void map_outdoor::clear_guild_area() {
        list_guild_area_.clear();
    }

    void map_outdoor::register_guild_area(int isx, int isy, int iex, int iey) {
        rectangle rect_value;
        rect_value.left = isx;
        rect_value.top = isy;
        rect_value.right = iex;
        rect_value.bottom = iey;
        list_guild_area_.push_back(rect_value);
    }

    void map_outdoor::visible_marked_area() {
        std::map<int, std::vector<uint8_t>> map_mark_buf;
        std::set<int> set_processed_map_index;

        auto itor_rect = list_guild_area_.begin();
        for (; itor_rect != list_guild_area_.end(); ++itor_rect) {
            const rectangle &rect_value = *itor_rect;

            int ix1_cell;
            int iy1_cell;
            uint8_t byx1_sub_cell;
            uint8_t byy1_sub_cell;
            uint16_t wx1_terrain_num;
            uint16_t wy1_terrain_num;

            int ix2_cell;
            int iy2_cell;
            uint8_t byx2_sub_cell;
            uint8_t byy2_sub_cell;
            uint16_t wx2_terrain_num;
            uint16_t wy2_terrain_num;

            convert_to_map_coords(float(rect_value.left), float(rect_value.top), &ix1_cell, &iy1_cell, &byx1_sub_cell, &byy1_sub_cell, &wx1_terrain_num, &wy1_terrain_num);
            convert_to_map_coords(float(rect_value.right), float(rect_value.bottom), &ix2_cell, &iy2_cell, &byx2_sub_cell, &byy2_sub_cell, &wx2_terrain_num, &wy2_terrain_num);

            ix1_cell = ix1_cell + wx1_terrain_num * terrain_area::ATTRMAP_XSIZE;
            iy1_cell = iy1_cell + wy1_terrain_num * terrain_area::ATTRMAP_YSIZE;
            ix2_cell = ix2_cell + wx2_terrain_num * terrain_area::ATTRMAP_XSIZE;
            iy2_cell = iy2_cell + wy2_terrain_num * terrain_area::ATTRMAP_YSIZE;

            for (int ix_cell = ix1_cell; ix_cell <= ix2_cell; ++ix_cell) {
                for (int iy_cell = iy1_cell; iy_cell <= iy2_cell; ++iy_cell) {
                    int ix_local_cell = ix_cell % terrain_area::ATTRMAP_XSIZE;
                    int iy_local_cell = iy_cell % terrain_area::ATTRMAP_YSIZE;
                    int ix_terrain = ix_cell / terrain_area::ATTRMAP_XSIZE;
                    int iy_terrain = iy_cell / terrain_area::ATTRMAP_YSIZE;
                    int i_terrain_num = ix_terrain + iy_terrain * 100;

                    uint8_t by_terrain_num;
                    if (!terrain_num_from_coord(ix_terrain, iy_terrain, &by_terrain_num)) {
                        continue;
                    }

                    terrain_area *terrain;
                    if (!terrain_pointer(by_terrain_num, &terrain)) {
                        continue;
                    }

                    std::vector<uint8_t> &mark = map_mark_buf[i_terrain_num];
                    if (mark.empty()) {
                        mark.assign(terrain_area::ATTRMAP_XSIZE * terrain_area::ATTRMAP_YSIZE, 0);
                    }

                    mark[ix_local_cell + iy_local_cell * terrain_area::ATTRMAP_XSIZE] = 0xff;
                }
            }
        }

        auto itor_terrain = map_mark_buf.begin();
        for (; itor_terrain != map_mark_buf.end(); ++itor_terrain) {
            int terrain_num = itor_terrain->first;
            int ix_terrain = terrain_num % 100;
            int iy_terrain = terrain_num / 100;
            uint8_t *buf = itor_terrain->second.data();

            uint8_t terrain_num_2;
            if (!terrain_num_from_coord(ix_terrain, iy_terrain, &terrain_num_2)) {
                continue;
            }

            terrain_area *terrain;
            if (!terrain_pointer(terrain_num_2, &terrain)) {
                continue;
            }

            terrain->allocate_marked_splats(buf);
        }
    }

    void map_outdoor::disable_marked_area() {
        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            if (!terrain_[i]) {
                continue;
            }

            terrain_[i]->deallocate_marked_splats();
        }
    }

    void map_outdoor::convert_to_map_coords(float fx, float fy, int *i_cell_x, int *i_cell_y, uint8_t *puc_sub_cell_x, uint8_t *puc_sub_cell_y, uint16_t *terrain_num_x, uint16_t *terrain_num_y) {
        if (fy < 0) {
            fy = -fy;
        }

        int ix, iy;
        PR_FLOAT_TO_INT(fx, ix);
        PR_FLOAT_TO_INT(fy, iy);

        *terrain_num_x = ix / (terrain_impl::TERRAIN_XSIZE);
        *terrain_num_y = iy / (terrain_impl::TERRAIN_YSIZE);

        auto maxx = (float)terrain_impl::TERRAIN_XSIZE;
        auto maxy = (float)terrain_impl::TERRAIN_YSIZE;

        while (fx < 0) {
            fx += maxx;
        }

        while (fy < 0) {
            fy += maxy;
        }

        while (fx >= maxx) {
            fx -= maxx;
        }

        while (fy >= maxy) {
            fy -= maxy;
        }

        float fooscale = 1.0f / (float)(terrain_impl::HALF_CELLSCALE);

        float f_cell_x, f_cell_y;

        f_cell_x = fx * fooscale;
        f_cell_y = fy * fooscale;

        PR_FLOAT_TO_INT(f_cell_x, *i_cell_x);
        PR_FLOAT_TO_INT(f_cell_y, *i_cell_y);

        float ratioooscale = ((float)terrain_impl::HEIGHT_TILE_XRATIO) * fooscale;

        float subcell_x, subcell_y;
        subcell_x = fx * ratioooscale;
        subcell_y = fy * ratioooscale;

        PR_FLOAT_TO_INT(subcell_x, *puc_sub_cell_x);
        PR_FLOAT_TO_INT(subcell_y, *puc_sub_cell_y);

        *puc_sub_cell_x = (*puc_sub_cell_x) % terrain_impl::HEIGHT_TILE_XRATIO;
        *puc_sub_cell_y = (*puc_sub_cell_y) % terrain_impl::HEIGHT_TILE_YRATIO;
    }
}
