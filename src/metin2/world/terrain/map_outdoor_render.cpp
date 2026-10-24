#include "map_outdoor.h"

#include "area_terrain.h"
#include "terrain_patch.h"
#include "terrain_quadtree.h"

#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>

#include <algorithm>

namespace metin2::inline world::inline terrain {

#define MAX_RENDER_SPALT 150
    area::crc_with_number_vector rendered_crc_with_number_vector;

    void map_outdoor::render_terrain() {
        if (!is_visible_part(PART_TERRAIN)) {
            return;
        }

        if (!setting_terrain_visible_) {
            return;
        }

        if (!terrain_patch_proxy_list_) {
            return;
        }

        camera *camera = get_camera_manager().current_camera();
        if (!camera) {
            return;
        }

        auto vv = view_ * proj_;
        build_view_frustum(vv);

        math::vector3 eye = camera->eye();
        xfor_distance_caculation_ = -eye.x;
        yfor_distance_caculation_ = -eye.y;

        patch_vector_.clear();

        render_terrain_recurse_render_quad_tree(root_node_);

        std::sort(patch_vector_.begin(), patch_vector_.end());

        render_terrain_render_hardware_transform_patch();
    }

    void map_outdoor::render_terrain_recurse_render_quad_tree(terrain_quadtree_node *node, bool cull_check_need) {
        if (cull_check_need) {
            switch (render_terrain_recurse_render_quad_tree_check_bounding_circle(node->center, node->radius)) {
            case VIEW_ALL:
                // Fully visible, so the children need no cull check.
                cull_check_need = false;
                break;
            case VIEW_PART:
                break;
            case VIEW_NONE:
                return;
            }
        }

        if (node->size == 1) {
            math::vector3 center = node->center;
            float distance = f_max(fabs(center.x + xfor_distance_caculation_), fabs(-center.y + yfor_distance_caculation_));
            render_terrain_append_patch(center, distance, node->patch_num);
        } else {
            if (node->nw_node != nullptr) {
                render_terrain_recurse_render_quad_tree(node->nw_node, cull_check_need);
            }

            if (node->ne_node != nullptr) {
                render_terrain_recurse_render_quad_tree(node->ne_node, cull_check_need);
            }

            if (node->sw_node != nullptr) {
                render_terrain_recurse_render_quad_tree(node->sw_node, cull_check_need);
            }

            if (node->se_node != nullptr) {
                render_terrain_recurse_render_quad_tree(node->se_node, cull_check_need);
            }
        }
    }

    auto map_outdoor::render_terrain_recurse_render_quad_tree_check_bounding_circle(const math::vector3 &c_v3_center, const float &radius) -> int {
        const int count = 6;

        math::vector3 center = c_v3_center;
        center.y = -center.y;

        int i;

        float distance[count];
        for (i = 0; i < count; ++i) {
            distance[i] = math::plane_dot_coord(plane_[i], center);
            if (distance[i] <= -radius) {
                return VIEW_NONE;
            }
        }

        for (i = 0; i < count; ++i) {
            if (distance[i] <= radius) {
                return VIEW_PART;
            }
        }

        return VIEW_ALL;
    }

    void map_outdoor::render_terrain_append_patch(const math::vector3 &center, float distance, int32_t patch_num) {
        assert(nullptr != terrain_patch_proxy_list_ && "CMapOutdoor::__RenderTerrain_AppendPatch");
        if (!terrain_patch_proxy_list_[patch_num].is_used()) {
            return;
        }

        terrain_patch_proxy_list_[patch_num].set_center_position(center);
        patch_vector_.push_back(std::make_pair(distance, patch_num));
    }

    void map_outdoor::apply_light(uintptr_t version, const gpu::light &light) {
        STATEMANAGER.set_light(0, &light);
    }

    void map_outdoor::initialize_visible_parts() {
        visible_part_flags_ = 0xffffffff;
    }

    void map_outdoor::set_visible_part(int part_value, bool is_visible) {
        uint32_t mask = (1 << part_value);
        if (is_visible) {
            visible_part_flags_ |= mask;
        } else {
            uint32_t reverse_mask = ~mask;
            visible_part_flags_ &= reverse_mask;
        }
    }

    auto map_outdoor::is_visible_part(int part_value) -> bool {
        uint32_t mask = (1 << part_value);
        if (mask & visible_part_flags_) {
            return true;
        }

        return false;
    }

    void map_outdoor::set_splat_limit(int splat_num) {
        splat_limit_ = splat_num;
    }

    auto map_outdoor::rendered_splat_num(int *patch, int *splat, float *splat_ratio) -> std::vector<int> & {
        *patch = rendered_patch_num_;
        *splat = rendered_splat_num_;
        *splat_ratio = rendered_splat_num_sq_sum_ / float(rendered_patch_num_);

        return rendered_texture_num_vector_;
    }

    auto map_outdoor::rendered_graphic_thing_instance_num(uint32_t *graphic_thing_instance_num, uint32_t *crc_num) -> area::crc_with_number_vector & {
        *graphic_thing_instance_num = rendered_graphic_thing_instance_num_;
        *crc_num = rendered_crc_num_;

        return rendered_crc_with_number_vector;
    }

    void map_outdoor::render_before_lens_flare() {
        lens_flare_value_.draw_before_flare();

        if (!mc_environment_data_) {
            metin2::log::error("No environment data before rendering the lens flare");
            return;
        }

        lens_flare_value_.compute(mc_environment_data_->dir_lights[ENV_DIRLIGHT_BACKGROUND].direction);
    }

    void map_outdoor::render_after_lens_flare() {
        screen_filter_value_.render();
        lens_flare_value_.adjust_brightness();
        lens_flare_value_.draw_flare();
    }

    void map_outdoor::render_collision() {
        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            area *area;
            if (area_pointer(i, &area)) {
                area->render_collision();
            }
        }
    }

    void map_outdoor::render_sky() {
        if (is_visible_part(PART_SKY)) {
            sky_box_value_.render();
        }
    }

    void map_outdoor::render_cloud() {
        if (is_visible_part(PART_CLOUD)) {
            sky_box_value_.render_cloud();
        }
    }

    void map_outdoor::render_tree() {
        if (is_visible_part(PART_TREE)) {
            get_speed_tree_forest_direct_x8().render();
        }
    }

    void map_outdoor::set_inverse_view_and_dynamic_shaodw_matrices() {
        auto *camera = get_camera_manager().current_camera();
        if (!camera) {
            return;
        }

        view_inverse_ = camera->inverse_view_matrix();

        math::vector3 target = camera->target();

        const auto vv = math::vector3(0.0f, 0.0f, 1.0f);
        light_view_ = math::matrix_look_at_rh(shadow_light_eye(target), target, vv);
        dynamic_shadow_ = view_inverse_ * light_view_ * dynamic_shadow_scale_;
    }

    auto map_outdoor::shadow_light_eye(const math::vector3 &target) const -> math::vector3 {
        math::vector3 to_light(-0.433f, -0.25f, 0.866f);
        if (mc_environment_data_ && mc_environment_data_->dir_lights_enable[ENV_DIRLIGHT_BACKGROUND]) {
            const math::vector3 &direction = mc_environment_data_->dir_lights[ENV_DIRLIGHT_BACKGROUND].direction;
            if (math::length_sq(direction) > 0.0f) {
                to_light = math::normalize(-direction);
            }
        }

        if (to_light.z < 0.5f) {
            to_light.z = 0.5f;
            to_light = math::normalize(to_light);
        }

        // A sun straight overhead would make the look-at (up is +z) degenerate; keep a little tilt.
        if (to_light.x * to_light.x + to_light.y * to_light.y < 0.01f) {
            to_light.x = -0.1f;
            to_light.y = -0.1f;
            to_light = math::normalize(to_light);
        }

        return target + to_light * 5000.0f;
    }

    void map_outdoor::on_render() {
        set_inverse_view_and_dynamic_shaodw_matrices();

        set_blend_operation();
        render_area();
        render_tree();

        if (!enable_terrain_only_for_height_) {
            render_terrain();
            render_grass();
        }

        render_blend_area();
    }

    struct area_render_shadow_fn {
        void operator()(graphic_object_instance *instance) {
            instance->render_shadow();
            instance->hide();
        }
    };

    struct pc_blocker_hide_fn {
        void operator()(graphic_object_instance *instance) {
            instance->hide();
        }
    };

    struct render_pc_blocker_fn {
        void operator()(graphic_object_instance *instance) {
            instance->show();
            auto *thing_instance = dynamic_cast<graphic_thing_instance *>(instance);
            if (thing_instance != nullptr) {
                if (thing_instance->have_blend_thing()) {
                    STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
                    thing_instance->blend_render();
                    return;
                }
            }

            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);

            instance->render_pc_blocker();
        }
    };

    void map_outdoor::render_effect() {
        if (!is_visible_part(PART_OBJECT)) {
            return;
        }

        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            area *area;
            if (area_pointer(i, &area)) {
                area->render_effect();
            }
        }
    }

    struct map_outdoor_less_thing_instance_ptr_render_order {
        auto operator()(graphic_thing_instance *left, graphic_thing_instance *right) -> bool {
            camera *current_camera = get_camera_manager().current_camera();
            const math::vector3 &camera_pos = current_camera->eye();
            const math::vector3 &left_pos = left->get_position();
            const math::vector3 &right_pos = right->get_position();
            const auto vv = math::vector3(camera_pos - right_pos);
            const auto vv2 = math::vector3(camera_pos - left_pos);

            return math::length_sq(vv2) < math::length_sq(vv);
        }
    };

    struct map_outdoor_opaque_thing_instance_render {
        inline void operator()(graphic_thing_instance *thing_inst) {
            thing_inst->render();
        }
    };

    struct map_outdoor_blend_thing_instance_render {
        inline void operator()(graphic_thing_instance *thing_inst) {
            thing_inst->blend_render();
        }
    };

    void map_outdoor::render_area(bool render_ambience) {
        if (!is_visible_part(PART_OBJECT)) {
            return;
        }

        rendered_crc_num_ = 0;
        rendered_graphic_thing_instance_num_ = 0;
        rendered_crc_with_number_vector.clear();

        for (int j = 0; j < AROUND_AREA_NUM; ++j) {
            area *area;
            if (area_pointer(j, &area)) {
                area->render_dungeon();
            }
        }

        std::ranges::for_each(pc_blocker_vector_, pc_blocker_hide_fn());

        if (draw_shadow_ && draw_chr_shadow_) {
            if (mc_environment_data_ != nullptr) {
                STATEMANAGER.set_render_state(gpu::state::FOG_COLOR, 0xFFFFFFFF);
            }

            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
            STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX, gpu::texcoord::CAMERA_SPACE_POSITION);
            STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::COUNT2);

            STATEMANAGER.save_transform(gpu::transform::TEXTURE1, &dynamic_shadow_);
            STATEMANAGER.set_texture(1, character_shadow_target_.texture());

            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
            STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::BORDER);
            STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::BORDER);
            STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::BORDER_COLOR, 0xFFFFFFFF);

            std::ranges::for_each(shadow_receiver_vector_, area_render_shadow_fn());

            STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX);
            STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);
            STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_U);
            STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_V);
            STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::BORDER_COLOR);

            STATEMANAGER.restore_transform(gpu::transform::TEXTURE1);

            if (mc_environment_data_ != nullptr) {
                STATEMANAGER.set_render_state(gpu::state::FOG_COLOR, mc_environment_data_->fog_color);
            }
        }

        STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, true);

        bool is_disable_sort_rendering = false;

        if (is_disable_sort_rendering) {
            for (int i = 0; i < AROUND_AREA_NUM; ++i) {
                area *area;
                if (area_pointer(i, &area)) {
                    area->render();

                    rendered_crc_num_ += area->debug_get_rendered_crc_num();
                    rendered_graphic_thing_instance_num_ += area->debug_get_rendered_grapphic_thing_instance_num();

                    area::crc_with_number_vector &crc_with_number_vector = area->debug_get_rendered_crc_with_num_vector();

                    auto iterator = crc_with_number_vector.begin();
                    while (iterator != crc_with_number_vector.end()) {
                        uint32_t crc = (*iterator++).crc;

                        auto crc_with_number_vector_iterator =
                            std::find_if(rendered_crc_with_number_vector.begin(), rendered_crc_with_number_vector.end(), area::find_if_crc_fn(crc));

                        if (rendered_crc_with_number_vector.end() == crc_with_number_vector_iterator) {
                            area::crc_with_number a_crc_with_number;
                            a_crc_with_number.crc = crc;
                            a_crc_with_number.number = 1;
                            rendered_crc_with_number_vector.push_back(a_crc_with_number);
                        } else {
                            area::crc_with_number &r_crc_with_number = *crc_with_number_vector_iterator;
                            r_crc_with_number.number += 1;
                        }
                    }
                }
            }

            std::sort(rendered_crc_with_number_vector.begin(), rendered_crc_with_number_vector.end(), area::rc_num_comp());
        } else {
            static std::vector<graphic_thing_instance *> vct_opaque_thing_inst_sort;
            vct_opaque_thing_inst_sort.clear();

            for (int i = 0; i < AROUND_AREA_NUM; ++i) {
                area *area;
                if (area_pointer(i, &area)) {
                    area->collect_rendering_object(vct_opaque_thing_inst_sort);
                }
            }

            std::sort(vct_opaque_thing_inst_sort.begin(), vct_opaque_thing_inst_sort.end(), map_outdoor_less_thing_instance_ptr_render_order());
            std::for_each(vct_opaque_thing_inst_sort.begin(), vct_opaque_thing_inst_sort.end(), map_outdoor_opaque_thing_instance_render());
        }

        STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);

        if (draw_shadow_ && draw_chr_shadow_) {
            std::for_each(shadow_receiver_vector_.begin(), shadow_receiver_vector_.end(), std::mem_fn(&graphic_object_instance::show));
        }
    }

    void map_outdoor::render_blend_area() {
        if (!is_visible_part(PART_OBJECT)) {
            return;
        }

        static std::vector<graphic_thing_instance *> vct_blend_thing_inst_sort;
        vct_blend_thing_inst_sort.clear();

        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            area *area;
            if (area_pointer(i, &area)) {
                area->collect_blend_rendering_object(vct_blend_thing_inst_sort);
            }
        }

        if (vct_blend_thing_inst_sort.size() != 0) {

            // Blended objects must be drawn farthest first, the opposite of the opaque order.
            std::sort(vct_blend_thing_inst_sort.begin(), vct_blend_thing_inst_sort.end(), [](graphic_thing_instance *left, graphic_thing_instance *right) -> bool {
                return map_outdoor_less_thing_instance_ptr_render_order()(right, left);
            });

            STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, true);
            STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
            STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
            STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

            std::ranges::for_each(vct_blend_thing_inst_sort, map_outdoor_blend_thing_instance_render());

            STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
            STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
            STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
            STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);
        }
    }
    void map_outdoor::render_dungeon() {
        for (int i = 0; i < AROUND_AREA_NUM; ++i) {
            area *area;
            if (!area_pointer(i, &area)) {
                continue;
            }

            area->render_dungeon();
        }
    }

    void map_outdoor::render_pc_blocker() {
        if (pc_blocker_vector_.size() != 0) {
            STATEMANAGER.set_texture(0, nullptr);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

            STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
            STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX, gpu::texcoord::CAMERA_SPACE_POSITION);
            STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::COUNT2);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
            STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);

            STATEMANAGER.save_transform(gpu::transform::TEXTURE1, &building_transparent_);
            STATEMANAGER.set_texture(1, building_transparent_image_instance_.texture_pointer()->gpu_texture());

            std::for_each(pc_blocker_vector_.begin(), pc_blocker_vector_.end(), render_pc_blocker_fn());

            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.restore_transform(gpu::transform::TEXTURE1);

            STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX);
            STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
            STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_U);
            STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_V);
            STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        }
    }

    void map_outdoor::select_index_buffer(uint8_t lod_level, uint16_t *primitive_count, gpu::primitive *pe_primitive_type) {
        if (0 == lod_level) {
            *primitive_count = num_indices_[lod_level] - 2;
            *pe_primitive_type = gpu::primitive::TRIANGLE_STRIP;
        } else {
            *primitive_count = num_indices_[lod_level] / 3;
            *pe_primitive_type = gpu::primitive::TRIANGLE_LIST;
        }

        STATEMANAGER.set_indices(index_buffer_[lod_level].gpu_buffer(), 0);
    }

    auto map_outdoor::no_fog_distance() -> float {
        return (float)(terrain_impl::CELLSCALE * view_radius_) * 0.5f;
    }

    auto map_outdoor::fog_distance() -> float {
        return (float)(terrain_impl::CELLSCALE * view_radius_) * 0.75f;
    }

    struct patch_num_match_fn {
        int32_t patch_num_to_check;

        patch_num_match_fn(int32_t patch_num) {
            patch_num_to_check = patch_num;
        }

        auto operator()(std::pair<int32_t, uint8_t> pair) -> bool {
            return patch_num_to_check == pair.first;
        }
    };

    void map_outdoor::render_marked_area() {
        if (!terrain_patch_proxy_list_) {
            return;
        }

        world_for_common_use_._41 = 0.0f;
        world_for_common_use_._42 = 0.0f;
        STATEMANAGER.set_transform(gpu::transform::WORLD, &world_for_common_use_);

        uint16_t primitive_count;
        gpu::primitive type;
        select_index_buffer(0, &primitive_count, &type);

        math::matrix tex_transform, tex_transform_temp;

        tex_transform = math::matrix_scaling(terrain_tex_coord_base_ * 32.0f, -terrain_tex_coord_base_ * 32.0f, 0.0f);
        tex_transform = view_inverse_ * tex_transform;
        STATEMANAGER.save_transform(gpu::transform::TEXTURE0, &tex_transform);
        STATEMANAGER.save_transform(gpu::transform::TEXTURE1, &tex_transform);

        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);

        static int32_t start_time = el_timer_get_m_sec();
        float time = float((el_timer_get_m_sec() - start_time) % 3000) / 3000.0f;
        float alpha = fabs(time - 0.5f) / 2.0f + 0.1f;
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, math::color(1.0f, 1.0f, 1.0f, alpha));
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TFACTOR);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG2);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::TFACTOR);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG2);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::TEXCOORD_INDEX, gpu::texcoord::CAMERA_SPACE_POSITION);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::COUNT2);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX, gpu::texcoord::CAMERA_SPACE_POSITION);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::COUNT2);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::MIN_FILTER, gpu::filter::POINT);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::MAG_FILTER, gpu::filter::POINT);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::MIP_FILTER, gpu::filter::POINT);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);

        STATEMANAGER.set_texture(0, attr_image_instance_.texture_pointer()->gpu_texture());

        recurse_render_attr(root_node_);

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::TEXCOORD_INDEX);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::MIN_FILTER);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::MAG_FILTER);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::MIP_FILTER);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_U);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_V);

        STATEMANAGER.restore_transform(gpu::transform::TEXTURE0);
        STATEMANAGER.restore_transform(gpu::transform::TEXTURE1);

        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
    }

    void map_outdoor::recurse_render_attr(terrain_quadtree_node *node, bool cull_enable) {
        if (cull_enable) {
            if (render_terrain_recurse_render_quad_tree_check_bounding_circle(node->center, node->radius) == VIEW_NONE) {
                return;
            }
        }

        if (node->size == 1) {
            draw_patch_attr(node->patch_num);
        } else {
            if (node->nw_node != nullptr) {
                recurse_render_attr(node->nw_node, cull_enable);
            }

            if (node->ne_node != nullptr) {
                recurse_render_attr(node->ne_node, cull_enable);
            }

            if (node->sw_node != nullptr) {
                recurse_render_attr(node->sw_node, cull_enable);
            }

            if (node->se_node != nullptr) {
                recurse_render_attr(node->se_node, cull_enable);
            }
        }
    }

    void map_outdoor::draw_patch_attr(int32_t patchnum) {
        terrain_patch_proxy *terrain_patch_proxy = &terrain_patch_proxy_list_[patchnum];
        if (!terrain_patch_proxy->is_used()) {
            return;
        }

        int32_t patch_num = terrain_patch_proxy->patch_num();
        if (patch_num < 0) {
            return;
        }

        uint8_t terrain_num = terrain_patch_proxy->terrain_num();
        if (0xFF == terrain_num) {
            return;
        }

        terrain_area *terrain;
        if (!terrain_pointer(terrain_num, &terrain)) {
            return;
        }

        if (!terrain->is_marked()) {
            return;
        }

        uint16_t coord_x, coord_y;
        terrain->coordinate(&coord_x, &coord_y);

        world_for_common_use_._41 = -(float)(coord_x * terrain_impl::XSIZE * terrain_impl::CELLSCALE);
        world_for_common_use_._42 = (float)(coord_y * terrain_impl::YSIZE * terrain_impl::CELLSCALE);

        math::matrix tex_transform, tex_transform_temp;
        tex_transform = view_inverse_ * world_for_common_use_;
        tex_transform = tex_transform * static_shadow_;
        STATEMANAGER.set_transform(gpu::transform::TEXTURE1, &tex_transform);

        terrain_splat_patch &attr_splat_patch = terrain->marked_splat_patch();
        STATEMANAGER.set_texture(1, attr_splat_patch.splats[0].gpu_texture);

        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::NORMAL);
        STATEMANAGER.set_stream_source(0, terrain_patch_proxy->hardware_transform_patch_get_vertex_buffer_ptr()->gpu_buffer(), patch_terrain_vertex_size_);

        STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_STRIP, 0, patch_terrain_vertex_count_, 0, num_indices_[0] - 2);
    }
}
