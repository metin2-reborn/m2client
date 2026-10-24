#include "map_outdoor.h"

#include "area_terrain.h"
#include "terrain_patch.h"
#include "terrain_quadtree.h"

#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/graphics/camera.h>
#include <metin2/log/log.h>
#include <metin2/world/actors/actor_instance.h>

#include <algorithm>

namespace metin2::inline world::inline terrain {
    // std::vector access was too slow here, so the spheres live in a fixed array on the stack.
    class pc_blocker_dynamic_sphere_instance_vector {
      public:
        using iterator = dynamic_sphere_instance *;

        enum {
            SIZE = 4,
        };

      public:
        pc_blocker_dynamic_sphere_instance_vector() = default;

        ~pc_blocker_dynamic_sphere_instance_vector() = default;

        auto begin() -> iterator {
            return dsi_ + 0;
        }
        auto end() -> iterator {
            return dsi_ + 4;
        }

      private:
        dynamic_sphere_instance dsi_[4];
    };

    auto map_outdoor::update(float x, float y, float z) -> bool {
        math::vector3 player(x, y, z);

        player_ = player;

        uint32_t t1 = el_timer_get_m_sec();

        int ix, iy;
        PR_FLOAT_TO_INT(x, ix);
        if (y < 0) {
            y = -y;
        }

        PR_FLOAT_TO_INT(y, iy);

        short coord_x = std::max(0, std::min(ix / terrain_impl::TERRAIN_XSIZE, terrain_count_x_ - 1));
        short coord_y = std::max(0, std::min(iy / terrain_impl::TERRAIN_YSIZE, terrain_count_y_ - 1));

        bool need_init = (prev_coordinate_.terrain_coord_x == -1 || prev_coordinate_.terrain_coord_y == -1);

        if (need_init ||
            (cur_coordinate_.terrain_coord_x / LOAD_SIZE_WIDTH) != (coord_x / LOAD_SIZE_WIDTH) ||
            (cur_coordinate_.terrain_coord_y / LOAD_SIZE_WIDTH) != (coord_y / LOAD_SIZE_WIDTH)) {
            if (need_init) {
                prev_coordinate_.terrain_coord_x = coord_x;
                prev_coordinate_.terrain_coord_y = coord_y;
            } else {
                prev_coordinate_.terrain_coord_x = cur_coordinate_.terrain_coord_x;
                prev_coordinate_.terrain_coord_y = cur_coordinate_.terrain_coord_y;
            }

            cur_coordinate_.terrain_coord_x = coord_x;
            cur_coordinate_.terrain_coord_y = coord_y;
            cur_coord_start_x_ = coord_x * terrain_impl::TERRAIN_XSIZE;
            cur_coord_start_y_ = coord_y * terrain_impl::TERRAIN_YSIZE;

            uint16_t cell_coord_x = (ix % terrain_impl::TERRAIN_XSIZE) / terrain_impl::CELLSCALE;
            uint16_t cell_coord_y = (iy % terrain_impl::TERRAIN_YSIZE) / terrain_impl::CELLSCALE;

            short reference_coord_min_x, reference_coord_max_x, reference_coord_min_y, reference_coord_max_y;
            reference_coord_min_x = std::max(cur_coordinate_.terrain_coord_x - LOAD_SIZE_WIDTH, 0);
            reference_coord_max_x = std::min(cur_coordinate_.terrain_coord_x + LOAD_SIZE_WIDTH, terrain_count_x_ - 1);
            reference_coord_min_y = std::max(cur_coordinate_.terrain_coord_y - LOAD_SIZE_WIDTH, 0);
            reference_coord_max_y = std::min(cur_coordinate_.terrain_coord_y + LOAD_SIZE_WIDTH, terrain_count_y_ - 1);

            for (uint16_t us_y = reference_coord_min_y; us_y <= reference_coord_max_y; ++us_y) {
                for (uint16_t us_x = reference_coord_min_x; us_x <= reference_coord_max_x; ++us_x) {
                    load_terrain(us_x, us_y, cell_coord_x, cell_coord_y);
                    load_area(us_x, us_y, cell_coord_x, cell_coord_y);
                }
            }

            assign_terrain_ptr();
            old_read_x_ = -1;

            metin2::log::debug("Terrain update loaded in {} ms", el_timer_get_m_sec() - t1);
        }

        update_garvage();
        update_terrain(x, y);
        update_area(player);
        update_sky();

        return true;
    }

    void map_outdoor::update_sky() {
        sky_box_value_.update();
    }

    struct get_shadow_receiver_from_collision_data_fn {
        bool collide;
        std::vector<graphic_object_instance *> *vct_shadow_receiver;
        dynamic_sphere_instance *pdsi;

        get_shadow_receiver_from_collision_data_fn(dynamic_sphere_instance *pdsi_value, std::vector<graphic_object_instance *> *vct_shadow_receiver_value) : collide(false), pdsi(pdsi_value) {
            vct_shadow_receiver = vct_shadow_receiver_value;
            vct_shadow_receiver->clear();
        }

        void operator()(graphic_object_instance *instance) {
            if (!instance) {
                return;
            }

            if (TREE_OBJECT == instance->get_type() || ACTOR_OBJECT == instance->get_type() || EFFECT_OBJECT == instance->get_type()) {
                return;
            }

            if (instance->collision_dynamic_sphere(*pdsi)) {
                vct_shadow_receiver->push_back(instance);
                collide = true;
            }
        }
    };

    struct pc_blocker_distance_sort_fn {
        math::vector3 eye;
        pc_blocker_distance_sort_fn(math::vector3 &eye_value) : eye(eye_value) {}

        auto operator()(graphic_object_instance *plhs, graphic_object_instance *prhs) const -> bool {
            const auto vv = (plhs->get_position() - eye);
            const auto vv2 = (prhs->get_position() - eye);
            return math::length_sq(vv) > math::length_sq(vv2);
        }
    };

    void map_outdoor::update_around_ambience(float x, float y, float z) {
        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            area *area;
            if (area_pointer(i, &area)) {
                area->update_around_ambience(x, y, z);
            }
        }
    }

    void map_outdoor::update_area(math::vector3 &player) {
        game_update_area(player);
    }

    void map_outdoor::game_update_area(math::vector3 &player) {
        pc_blocker_vector_.clear();
        shadow_receiver_vector_.clear();

        auto &cmr_mgr = get_camera_manager();
        auto *camera = cmr_mgr.current_camera();
        if (!camera) {
            return;
        }

        float distance = camera->distance();

        math::vector3 eye = camera->eye();

        math::vector3 light = math::vector3(1.732f, 1.0f, -3.464f);
        light *= 50.0f / math::length(light);

        collect_shadow_receiver(player, light);
        collect_collision_pc_blocker(eye, player, distance);
        collect_collision_shadow_receiver(player, light);
        update_around_area_list();
    }

    void map_outdoor::update_around_area_list() {
        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            area *area;
            if (area_pointer(i, &area)) {
                area->update();
            }
        }
    }

    struct get_shadow_receiver_from_height_data_fn {
        enum {
            COLLECT_MAX = 100,
        };

        uint32_t collect_over_count;
        uint32_t collect_count;
        uint32_t check_count;
        bool receiver_found;
        float from_x, from_y, to_x, to_y;
        float return_height;

        graphic_object_instance *shadow_receiver[COLLECT_MAX];

        get_shadow_receiver_from_height_data_fn(float from_x_value, float from_y_value, float to_x_value, float to_y_value) : receiver_found(false), from_x(from_x_value), from_y(from_y_value), to_x(to_x_value), to_y(to_y_value) {
            check_count = 0;
            collect_over_count = 0;
            collect_count = 0;
        }

        auto collect_item(uint32_t index) -> graphic_object_instance * {
            if (index >= collect_count) {
                return nullptr;
            }

            return shadow_receiver[index];
        }

        auto get_collect_count() -> uint32_t {
            return collect_count;
        }

        void operator()(graphic_object_instance *instance) {
            check_count++;

            if (!instance) {
                return;
            }

            if (from_y < 0) {
                from_y = -from_y;
            }

            if (to_y < 0) {
                to_y = -to_y;
            }

            if (instance->object_height(from_x, from_y, &return_height) ||
                instance->object_height(to_x, to_y, &return_height)) {
                if (collect_count < COLLECT_MAX) {
                    shadow_receiver[collect_count++] = instance;
                } else {
                    collect_over_count++;
                }

                receiver_found = true;
            }
        }
    };

    void map_outdoor::collect_shadow_receiver(math::vector3 &target, math::vector3 &light) {
        dynamic_sphere_instance s;
        s.last_position = target + light;
        s.position_value = s.last_position + light;
        s.radius = 50.0f;

        vector3d vector3d;
        vector3d.set(target.x, target.y, target.z);

        culling_manager &culling_mgr = get_culling_manager();

        get_shadow_receiver_from_height_data_fn get_shadow_receiver_from_height_data(target.x, target.y, s.position_value.x, s.position_value.y);
        culling_mgr.for_in_range(vector3d, 10.0f, &get_shadow_receiver_from_height_data);

        if (get_shadow_receiver_from_height_data.receiver_found) {
            for (uint32_t i = 0; i < get_shadow_receiver_from_height_data.get_collect_count(); ++i) {
                graphic_object_instance *obj_inst_each = get_shadow_receiver_from_height_data.collect_item(i);
                if (!is_in_shadow_receiver_list(obj_inst_each)) {
                    shadow_receiver_vector_.push_back(obj_inst_each);
                }
            }
        }
    }

    struct pc_blocker_instance_list {
        using item = graphic_object_instance *;
        using iterator = item *;

        enum {
            CAPACITY = 512,
        };

        uint32_t inst_count;
        uint32_t blocker_count;
        uint32_t blocker_over_count;

        item pc_blocker[CAPACITY];

        pc_blocker_dynamic_sphere_instance_vector *dsi_vector;

        camera *camera_value;
        math::vector2 view;
        math::vector2 target;

        pc_blocker_instance_list(pc_blocker_dynamic_sphere_instance_vector *dsi_vector_value) {
            camera_value = get_camera_manager().current_camera();
            if (!camera_value) {
                return;
            }

            math::vector3 view_value = camera_value->view();
            math::vector3 target_value = camera_value->target();

            view.x = view_value.x;
            view.y = view_value.y;

            target.x = target_value.x;
            target.y = target_value.y;

            dsi_vector = dsi_vector_value;
            blocker_count = 0;
            blocker_over_count = 0;
            inst_count = 0;
        }

        ~pc_blocker_instance_list() = default;

        auto begin() -> iterator {
            return pc_blocker;
        }
        auto end() -> iterator {
            return pc_blocker + blocker_count;
        }

        auto size() -> uint32_t {
            return blocker_count;
        }

        auto is_empty() -> bool {
            if (blocker_count > 0) {
                return false;
            }

            return true;
        }

        void append_pc_blocker(graphic_object_instance *instance) {
            if (blocker_count < CAPACITY) {
                pc_blocker[blocker_count++] = instance;
            } else {
                blocker_over_count++;
            }
        }

        void append_object(graphic_object_instance *instance) {
            math::vector3 center;
            float radius;
            instance->bounding_sphere(center, radius);

            math::vector2 target_to_center;
            target_to_center.x = center.x - target.x;
            target_to_center.y = center.y - target.y;
            if (math::dot(view, target_to_center) <= 0) {
                append_pc_blocker(instance);
                return;
            }
        }

        void operator()(graphic_object_instance *instance) {
            if (!camera_value) {
                return;
            }

            if (!instance) {
                return;
            }

            ++inst_count;

            pc_blocker_dynamic_sphere_instance_vector::iterator i;

            for (i = dsi_vector->begin(); i != dsi_vector->end(); ++i) {
                dynamic_sphere_instance &dsi = *i;
                if (instance->collision_dynamic_sphere(dsi)) {
                    if (TREE_OBJECT == instance->get_type()) {
                        append_pc_blocker(instance);
                        return;
                    } else if (THING_OBJECT == instance->get_type()) {
                        append_object(instance);
                    } else if (ACTOR_OBJECT == instance->get_type()) {
                        if (((actor_instance *)instance)->is_building()) {
                            append_object(instance);
                        }
                    }
                }
            }
        }
    };

    void map_outdoor::collect_collision_pc_blocker(math::vector3 &eye, math::vector3 &target, float distance) {
        vector3d v3d_ray_start;
        v3d_ray_start.set(eye.x, eye.y, eye.z);

        pc_blocker_dynamic_sphere_instance_vector dynamic_sphere_instance_vector;
        {
            dynamic_sphere_instance *dsi = dynamic_sphere_instance_vector.begin();
            dsi->radius = distance * 0.5f;
            dsi->last_position = eye;
            dsi->position_value = eye + 0.5f * (target - eye);
            ++dsi;

            dsi->radius = distance * 0.5f;
            dsi->last_position = eye + 0.5f * (target - eye);
            dsi->position_value = target;
            ++dsi;

            dsi->radius = distance * 0.5f;
            dsi->last_position = target;
            dsi->position_value = eye + 0.5f * (target - eye);
            ++dsi;

            dsi->radius = distance * 0.5f;
            dsi->last_position = eye + 0.5f * (target - eye);
            dsi->position_value = eye;
            ++dsi;
        }
        culling_manager &culling_mgr = get_culling_manager();

        pc_blocker_instance_list pc_blocker_list(&dynamic_sphere_instance_vector);
        range_tester<pc_blocker_instance_list> pc_blocker_range_tester(&pc_blocker_list);
        culling_mgr.range_test(v3d_ray_start, distance, &pc_blocker_range_tester);

        if (!pc_blocker_list.is_empty()) {
            pc_blocker_instance_list::iterator i;

            for (i = pc_blocker_list.begin(); i != pc_blocker_list.end(); ++i) {
                graphic_object_instance *obj_inst_each = *i;

                if (!obj_inst_each) {
                    continue;
                }

                if (TREE_OBJECT == obj_inst_each->get_type() && !transparent_tree_) {
                    continue;
                }

                if (!is_in_shadow_receiver_list(obj_inst_each)) {
                    if (!is_in_pc_blocker_list(obj_inst_each)) {
                        pc_blocker_vector_.push_back(obj_inst_each);
                    }
                }
            }
        }

        std::ranges::sort(pc_blocker_vector_, pc_blocker_distance_sort_fn(eye));
    }

    void map_outdoor::collect_collision_shadow_receiver(math::vector3 &target, math::vector3 &light) {
        dynamic_sphere_instance s;
        s.radius = 50.0f;
        s.last_position = target + light;
        s.position_value = s.last_position + light;

        vector3d vector3d;
        vector3d.set(target.x, target.y, target.z);

        culling_manager &culling_mgr = get_culling_manager();

        std::vector<graphic_object_instance *> vct_shadow_receiver;
        get_shadow_receiver_from_collision_data_fn get_shadow_receiver_from_collision_data(&s, &vct_shadow_receiver);
        culling_mgr.for_in_range(vector3d, 100.0f, &get_shadow_receiver_from_collision_data);
        if (!get_shadow_receiver_from_collision_data.collide) {
            return;
        }

        std::vector<graphic_object_instance *>::iterator i;
        for (i = vct_shadow_receiver.begin(); i != vct_shadow_receiver.end(); ++i) {
            graphic_object_instance *obj_inst_each = *i;
            if (!is_in_pc_blocker_list(obj_inst_each)) {
                if (!is_in_shadow_receiver_list(obj_inst_each)) {
                    shadow_receiver_vector_.push_back(obj_inst_each);
                }
            }
        }
    }

    auto map_outdoor::is_in_shadow_receiver_list(graphic_object_instance *obj_inst_test) -> bool {
        if (shadow_receiver_vector_.end() == std::find(shadow_receiver_vector_.begin(), shadow_receiver_vector_.end(), obj_inst_test)) {
            return false;
        }

        return true;
    }

    auto map_outdoor::is_in_pc_blocker_list(graphic_object_instance *obj_inst_test) -> bool {
        if (pc_blocker_vector_.end() == std::find(pc_blocker_vector_.begin(), pc_blocker_vector_.end(), obj_inst_test)) {
            return false;
        }

        return true;
    }

    void map_outdoor::update_terrain(float x, float y) {
        if (y < 0) {
            y = -y;
        }

        int sx, sy;
        PR_FLOAT_TO_INT(x, sx);
        PR_FLOAT_TO_INT(y, sy);

        int32_t divider = (terrain_impl::CELLSCALE * TERRAIN_PATCHSIZE);

        center_x_ = (sx - cur_coord_start_x_) / divider;
        center_y_ = (sy - cur_coord_start_y_) / divider;

        if ((center_x_ != old_read_x_) || (center_y_ != old_read_y_)) {
            int32_t real_center_x = center_x_ * TERRAIN_PATCHSIZE;
            int32_t real_center_y = center_y_ * TERRAIN_PATCHSIZE;
            old_read_x_ = center_x_;
            old_read_y_ = center_y_;

            convert_terrain_to_tn_l(real_center_x, real_center_y);
            update_area_list(real_center_x, real_center_y);
        }
    }

    void map_outdoor::push_terrain_to_delete_vector_fn::operator()(terrain_area *terrain) {
        auto iterator = std::ranges::find(return_terrain_vector, terrain);
        if (iterator != return_terrain_vector.end()) {
            return;
        }

        uint16_t reference_coord_x = cur_coordinate.terrain_coord_x;
        uint16_t reference_coord_y = cur_coordinate.terrain_coord_y;

        uint16_t coord_x, coord_y;
        terrain->coordinate(&coord_x, &coord_y);

        switch (lr_delete_dir) {
        case DELETE_LEFT:
            if (coord_x < reference_coord_x - LOAD_SIZE_WIDTH) {
                return_terrain_vector.push_back(terrain);
            }
            break;

        case DELETE_RIGHT:
            if (coord_x > reference_coord_x + LOAD_SIZE_WIDTH) {
                return_terrain_vector.push_back(terrain);
            }
            break;

        default:
            break;
        }

        iterator = std::ranges::find(return_terrain_vector, terrain);
        if (iterator != return_terrain_vector.end()) {
            return;
        }

        switch (tb_delete_dir) {
        case DELETE_TOP:
            if (coord_y < reference_coord_y - LOAD_SIZE_WIDTH) {
                return_terrain_vector.push_back(terrain);
            }
            break;

        case DELETE_BOTTOM:
            if (coord_y > reference_coord_y + LOAD_SIZE_WIDTH) {
                return_terrain_vector.push_back(terrain);
            }
            break;

        default:
            break;
        }
    }

    void map_outdoor::push_area_to_delete_vector_fn::operator()(area *area) {
        auto iterator = std::ranges::find(return_area_vector, area);
        if (iterator != return_area_vector.end()) {
            return;
        }

        uint16_t reference_coord_x = cur_coordinate.terrain_coord_x;
        uint16_t reference_coord_y = cur_coordinate.terrain_coord_y;

        uint16_t coord_x, coord_y;
        area->coordinate(&coord_x, &coord_y);

        switch (lr_delete_dir) {
        case DELETE_LEFT:
            if (coord_x < reference_coord_x - LOAD_SIZE_WIDTH) {
                return_area_vector.push_back(area);
            }
            break;

        case DELETE_RIGHT:
            if (coord_x > reference_coord_x + LOAD_SIZE_WIDTH) {
                return_area_vector.push_back(area);
            }
            break;

        default:
            break;
        }

        iterator = std::find(return_area_vector.begin(), return_area_vector.end(), area);
        if (iterator != return_area_vector.end()) {
            return;
        }

        switch (tb_delete_dir) {
        case DELETE_TOP:
            if (coord_y < reference_coord_y - LOAD_SIZE_WIDTH) {
                return_area_vector.push_back(area);
            }
            break;

        case DELETE_BOTTOM:
            if (coord_y > reference_coord_y + LOAD_SIZE_WIDTH) {
                return_area_vector.push_back(area);
            }
            break;

        default:
            break;
        }
    }

    void map_outdoor::clear_garvage() {
        std::ranges::for_each(terrain_delete_vector_, terrain_area::delete_value);
        terrain_delete_vector_.clear();

        std::ranges::for_each(area_delete_vector_, area::delete_value);
        area_delete_vector_.clear();
    }

    void map_outdoor::update_garvage() {
        const uint32_t terrain_erase_interval = 1000 * 60;
        static uint32_t erase_time = el_timer_get_m_sec();

        if (!terrain_delete_vector_.empty()) {
            if (el_timer_get_m_sec() - erase_time <= terrain_erase_interval) {
                return;
            }

            auto terrain_ptr_delete_itertor = terrain_delete_vector_.begin();
            terrain_area *terrain = *terrain_ptr_delete_itertor;
            terrain_area::delete_value(terrain);

            terrain_ptr_delete_itertor = terrain_delete_vector_.erase(terrain_ptr_delete_itertor);
            erase_time = el_timer_get_m_sec();
            metin2::log::debug("Deleting terrain");
            return;
        }

        if (!area_delete_vector_.empty()) {
            if (el_timer_get_m_sec() - erase_time <= terrain_erase_interval) {
                return;
            }

            auto area_ptr_delete_itertor = area_delete_vector_.begin();

            area *area = *area_ptr_delete_itertor;
            area::delete_value(area);

            area_ptr_delete_itertor = area_delete_vector_.erase(area_ptr_delete_itertor);
            erase_time = el_timer_get_m_sec();
            metin2::log::debug("Deleting area");
            return;
        }
    }

    void map_outdoor::update_area_list(int32_t center_x, int32_t center_y) {
        if (terrain_vector_.size() <= AROUND_AREA_NUM && area_vector_.size() <= AROUND_AREA_NUM) {
            return;
        }

        clear_garvage();

        push_to_delete_vector_fn::delete_dir delete_lr_dir, delete_tb_dir;

        if (center_x > terrain_impl::XSIZE / 2) {
            delete_lr_dir = push_to_delete_vector_fn::DELETE_LEFT;
        } else {
            delete_lr_dir = push_to_delete_vector_fn::DELETE_RIGHT;
        }

        if (center_y > terrain_impl::YSIZE / 2) {
            delete_tb_dir = push_to_delete_vector_fn::DELETE_TOP;
        } else {
            delete_tb_dir = push_to_delete_vector_fn::DELETE_BOTTOM;
        }

        push_terrain_to_delete_vector_fn push_terrain_to_delete_vector = std::for_each(terrain_vector_.begin(), terrain_vector_.end(),
            push_terrain_to_delete_vector_fn(delete_lr_dir, delete_tb_dir, cur_coordinate_));
        push_area_to_delete_vector_fn push_area_to_delete_vector = std::for_each(area_vector_.begin(), area_vector_.end(),
            push_area_to_delete_vector_fn(delete_lr_dir, delete_tb_dir, cur_coordinate_));

        if (!push_terrain_to_delete_vector.return_terrain_vector.empty()) {
            terrain_delete_vector_.resize(push_terrain_to_delete_vector.return_terrain_vector.size());
            std::ranges::copy(push_terrain_to_delete_vector.return_terrain_vector, terrain_delete_vector_.begin());

            for (uint32_t index = 0; index < push_terrain_to_delete_vector.return_terrain_vector.size(); ++index) {
                auto terrain_ptr_itertor = terrain_vector_.begin();
                while (terrain_ptr_itertor != terrain_vector_.end()) {
                    terrain_area *terrain = *terrain_ptr_itertor;
                    if (terrain == push_terrain_to_delete_vector.return_terrain_vector[index]) {
                        terrain_ptr_itertor = terrain_vector_.erase(terrain_ptr_itertor);
                    } else {
                        ++terrain_ptr_itertor;
                    }
                }
            }
        }
        if (!push_area_to_delete_vector.return_area_vector.empty()) {
            area_delete_vector_.resize(push_area_to_delete_vector.return_area_vector.size());
            std::ranges::copy(push_area_to_delete_vector.return_area_vector, area_delete_vector_.begin());

            for (uint32_t index = 0; index < push_area_to_delete_vector.return_area_vector.size(); ++index) {
                auto area_ptr_itertor = area_vector_.begin();
                while (area_ptr_itertor != area_vector_.end()) {
                    area *area = *area_ptr_itertor;
                    if (area == push_area_to_delete_vector.return_area_vector[index]) {
                        area_ptr_itertor = area_vector_.erase(area_ptr_itertor);
                    } else {
                        ++area_ptr_itertor;
                    }
                }
            }
        }
    }

    void map_outdoor::convert_terrain_to_tn_l(int32_t lx, int32_t ly) {
        assert(nullptr != terrain_patch_proxy_list_ && "CMapOutdoor::ConvertTerrainToTnL");

        for (int32_t i = 0; i < patch_count_ * patch_count_; i++) {
            terrain_patch_proxy_list_[i].set_used(false);
        }

        lx -= view_radius_; // Top-left corner of the view rectangle.
        ly -= view_radius_;

        int32_t diameter = view_radius_ * 2;

        int32_t x0 = lx / TERRAIN_PATCHSIZE;
        int32_t y0 = ly / TERRAIN_PATCHSIZE;
        int32_t x1 = (lx + diameter - 1) / TERRAIN_PATCHSIZE;
        int32_t y1 = (ly + diameter - 1) / TERRAIN_PATCHSIZE;

        int32_t xw = x1 - x0 + 1; // Number of patches needed.
        int32_t yw = y1 - y0 + 1;

        int32_t ex = lx + diameter;
        int32_t ey = ly + diameter;

        y0 = ly;
        for (int32_t yp = 0; yp < yw; yp++) {
            x0 = lx;
            y1 = (y0 / TERRAIN_PATCHSIZE + 1) * TERRAIN_PATCHSIZE;
            if (y1 > ey) {
                y1 = ey;
            }

            for (int32_t xp = 0; xp < xw; xp++) {
                x1 = (x0 / TERRAIN_PATCHSIZE + 1) * TERRAIN_PATCHSIZE;
                if (x1 > ex) {
                    x1 = ex;
                }

                assign_patch(yp * patch_count_ + xp, x0, y0, x1, y1);
                x0 = x1;
            }
            y0 = y1;
        }
        update_quad_tree_heights(root_node_);
    }

    void map_outdoor::assign_patch(int32_t patch_num, int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
        assert(nullptr != terrain_patch_proxy_list_ && "CMapOutdoor::AssignPatch");

        terrain_patch_proxy *terrain_patch_proxy = &terrain_patch_proxy_list_[patch_num];

        if (y0 < 0 && y1 <= 0) {
            if (x0 < 0 && x1 <= 0) {
                terrain_patch_proxy->set_terrain_num(0);
                x0 += terrain_impl::XSIZE;
                x1 += terrain_impl::XSIZE;
            } else if (x0 >= terrain_impl::XSIZE && x1 > terrain_impl::XSIZE) {
                terrain_patch_proxy->set_terrain_num(2);
                x0 -= terrain_impl::XSIZE;
                x1 -= terrain_impl::XSIZE;
            } else {
                terrain_patch_proxy->set_terrain_num(1);
            }

            y0 += terrain_impl::YSIZE;
            y1 += terrain_impl::YSIZE;
        } else if (y0 >= terrain_impl::YSIZE && y1 > terrain_impl::YSIZE) {
            if (x0 < 0 && x1 <= 0) {
                terrain_patch_proxy->set_terrain_num(6);
                x0 += terrain_impl::XSIZE;
                x1 += terrain_impl::XSIZE;
            } else if (x0 >= terrain_impl::XSIZE && x1 > terrain_impl::XSIZE) {
                terrain_patch_proxy->set_terrain_num(8);
                x0 -= terrain_impl::XSIZE;
                x1 -= terrain_impl::XSIZE;
            } else {
                terrain_patch_proxy->set_terrain_num(7);
            }

            y0 -= terrain_impl::YSIZE;
            y1 -= terrain_impl::YSIZE;
        } else {
            if (x0 < 0 && x1 <= 0) {
                terrain_patch_proxy->set_terrain_num(3);
                x0 += terrain_impl::XSIZE;
                x1 += terrain_impl::XSIZE;
            } else if (x0 >= terrain_impl::XSIZE && x1 > terrain_impl::XSIZE) {
                terrain_patch_proxy->set_terrain_num(5);
                x0 -= terrain_impl::XSIZE;
                x1 -= terrain_impl::XSIZE;
            } else {
                terrain_patch_proxy->set_terrain_num(4);
            }
        }

        terrain_area *terrain;
        if (!terrain_pointer(terrain_patch_proxy->terrain_num(), &terrain)) {
            return;
        }

        uint8_t patch_num_x, patch_num_y;
        patch_num_x = x0 / terrain_impl::PATCH_XSIZE;
        patch_num_y = y0 / terrain_impl::PATCH_YSIZE;

        terrain_patch *terrain_patch = terrain->terrain_patch_ptr(patch_num_x, patch_num_y);
        if (!terrain_patch) {
            return;
        }

        terrain_patch_proxy->set_patch_num(patch_num_y * terrain_impl::PATCH_XCOUNT + patch_num_x);
        terrain_patch_proxy->set_terrain_patch(terrain_patch);
        terrain_patch_proxy->set_used(true);
    }

    void map_outdoor::update_quad_tree_heights(terrain_quadtree_node *node) {
        assert(nullptr != terrain_patch_proxy_list_ && "CMapOutdoor::UpdateQuadTreeHeights");
        if (!terrain_patch_proxy_list_) {
            return;
        }

        float minx, maxx, miny, maxy, minz, maxz;
        minx = maxx = miny = maxy = minz = maxz = 0;

        if (terrain_patch_proxy_list_[node->patch_num].is_used()) {
            minx = terrain_patch_proxy_list_[node->patch_num].min_x();
            maxx = terrain_patch_proxy_list_[node->patch_num].max_x();
            miny = terrain_patch_proxy_list_[node->patch_num].min_y();
            maxy = terrain_patch_proxy_list_[node->patch_num].max_y();
            minz = terrain_patch_proxy_list_[node->patch_num].min_z();
            maxz = terrain_patch_proxy_list_[node->patch_num].max_z();
        }

        for (int32_t y = node->y0; y <= node->y1; y++) {
            for (int32_t x = node->x0; x <= node->x1; x++) {
                int32_t patch = y * patch_count_ + x;

                if (!terrain_patch_proxy_list_[patch].is_used()) {
                    continue;
                }

                if (terrain_patch_proxy_list_[patch].min_x() < minx) {
                    minx = terrain_patch_proxy_list_[patch].min_x();
                }
                if (terrain_patch_proxy_list_[patch].max_x() > maxx) {
                    maxx = terrain_patch_proxy_list_[patch].max_x();
                }

                if (terrain_patch_proxy_list_[patch].min_y() < miny) {
                    miny = terrain_patch_proxy_list_[patch].min_y();
                }
                if (terrain_patch_proxy_list_[patch].max_y() > maxy) {
                    maxy = terrain_patch_proxy_list_[patch].max_y();
                }

                if (terrain_patch_proxy_list_[patch].min_z() < minz) {
                    minz = terrain_patch_proxy_list_[patch].min_z();
                }
                if (terrain_patch_proxy_list_[patch].max_z() > maxz) {
                    maxz = terrain_patch_proxy_list_[patch].max_z();
                }
            }
        }

        node->center.x = (maxx + minx) * 0.5f;
        node->center.y = (maxy + miny) * 0.5f;
        node->center.z = (maxz + minz) * 0.5f;

        node->radius = sqrtf((maxx - minx) * (maxx - minx) +
                             (maxy - miny) * (maxy - miny) +
                             (maxz - minz) * (maxz - minz)) /
                       2.0f;

        if (node->nw_node != nullptr) {
            update_quad_tree_heights(node->nw_node);
        }

        if (node->ne_node != nullptr) {
            update_quad_tree_heights(node->ne_node);
        }

        if (node->sw_node != nullptr) {
            update_quad_tree_heights(node->sw_node);
        }

        if (node->se_node != nullptr) {
            update_quad_tree_heights(node->se_node);
        }
    }
}
