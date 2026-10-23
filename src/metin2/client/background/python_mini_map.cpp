#include "python_mini_map.h"

#include "python_background.h"

#include <metin2/client/app/abstract_player.h>
#include <metin2/client/app/python_system.h>
#include <metin2/client/characters/python_character_manager.h>
#include <metin2/client/guilds/python_guild.h>
#include <metin2/core/parse.h>
#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/grp_sub_image.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include <metin2/ui/python_window_manager.h>

#include <format>

namespace metin2::inline client::inline background {
    void python_mini_map::set_center_position(float center_x, float center_y) {
        center_x_ = center_x;
        center_y_ = center_y;

        auto &map = get_python_background().map_outdoor_ref();
        for (uint8_t terrain_num = 0; terrain_num < AROUND_AREA_NUM; ++terrain_num) {
            mini_map_texture_[terrain_num] = nullptr;

            terrain_area *terrain;
            if (map.terrain_pointer(terrain_num, &terrain)) {
                mini_map_texture_[terrain_num] = terrain->mini_map_texture();
            }
        }

        const auto &outdoor_map_coord = map.cur_coordinate();

        center_cell_x_ = (center_x_ - (float)(outdoor_map_coord.terrain_coord_x * terrain_impl::TERRAIN_XSIZE)) / (float)(terrain_impl::CELLSCALE);
        center_cell_y_ = (center_y_ - (float)(outdoor_map_coord.terrain_coord_y * terrain_impl::TERRAIN_YSIZE)) / (float)(terrain_impl::CELLSCALE);

        set_position_impl();
    }

    void python_mini_map::update(float center_x, float center_y) {
        if (!get_python_background().is_map_outdoor()) {
            return;
        }

        if (center_x_ != center_x || center_y_ != center_y) {
            set_center_position(center_x, center_y);
        }

        other_pc_position_vector_.clear();
        party_pc_position_vector_.clear();
        npc_position_vector_.clear();
        monster_position_vector_.clear();
        warp_position_vector_.clear();

        float foo_cell_scale = 1.0f / ((float)terrain_impl::CELLSCALE);

        auto &chr_mgr = get_python_character_manager();

        auto *inst_main = chr_mgr.main_instance_ptr();
        if (!inst_main) {
            return;
        }

        python_character_manager::character_iterator i;
        for (i = chr_mgr.character_instance_begin(); i != chr_mgr.character_instance_end(); ++i) {
            instance_base *inst_each = *i;

            pixel_position instance_position;
            inst_each->new_get_pixel_position(&instance_position);

            float distance_from_center_x = (instance_position.x - center_x_) * foo_cell_scale * scale_;
            float distance_from_center_y = (instance_position.y - center_y_) * foo_cell_scale * scale_;
            if (fabs(distance_from_center_x) >= mini_map_radius_ || fabs(distance_from_center_y) >= mini_map_radius_) {
                continue;
            }

            float distance_from_center = sqrtf(distance_from_center_x * distance_from_center_x + distance_from_center_y * distance_from_center_y);
            if (distance_from_center >= mini_map_radius_) {
                continue;
            }

            mark_position mark_position_value;

            if (inst_each->is_pc() && !inst_each->is_invisibility()) {
                if (inst_each == get_python_character_manager().main_instance_ptr()) {
                    continue;
                }

                mark_position_value.x = (width_ - (float)white_mark_.width()) / 2.0f + distance_from_center_x + screen_x_;
                mark_position_value.y = (height_ - (float)white_mark_.height()) / 2.0f + distance_from_center_y + screen_y_;
                mark_position_value.name_color = inst_each->name_color_index();
                if (mark_position_value.name_color == instance_base::NAMECOLOR_PARTY) {
                    party_pc_position_vector_.push_back(mark_position_value);
                } else {
                    other_pc_position_vector_.push_back(mark_position_value);
                }
            } else if (inst_each->is_npc()) {
                mark_position_value.x = (width_ - (float)white_mark_.width()) / 2.0f + distance_from_center_x + screen_x_;
                mark_position_value.y = (height_ - (float)white_mark_.height()) / 2.0f + distance_from_center_y + screen_y_;

                npc_position_vector_.push_back(mark_position_value);
            } else if (inst_each->is_enemy()) {
                mark_position_value.x = (width_ - (float)white_mark_.width()) / 2.0f + distance_from_center_x + screen_x_;
                mark_position_value.y = (height_ - (float)white_mark_.height()) / 2.0f + distance_from_center_y + screen_y_;

                monster_position_vector_.push_back(mark_position_value);
            } else if (inst_each->is_warp()) {
                mark_position_value.x = (width_ - (float)white_mark_.width()) / 2.0f + distance_from_center_x + screen_x_;
                mark_position_value.y = (height_ - (float)white_mark_.height()) / 2.0f + distance_from_center_y + screen_y_;

                warp_position_vector_.push_back(mark_position_value);
            }
        }

        {
            auto itor = atlas_way_point_info_vector_.begin();
            for (; itor != atlas_way_point_info_vector_.end(); ++itor) {
                auto &atlas_mark_info_value = *itor;
                if (TYPE_TARGET != atlas_mark_info_value.type_value) {
                    continue;
                }

                if (atlas_mark_info_value.chr_vid != 0) {
                    auto *instance = get_python_character_manager().instance_ptr(atlas_mark_info_value.chr_vid);
                    if (instance) {
                        pixel_position pixel_position;
                        instance->new_get_pixel_position(&pixel_position);
                        update_way_point(&atlas_mark_info_value, pixel_position.x, pixel_position.y);
                    }
                }

                const float mini_map_window_radius = 55.0f;

                float distance_from_center_x = (atlas_mark_info_value.x - center_x_) * foo_cell_scale * scale_;
                float distance_from_center_y = (atlas_mark_info_value.y - center_y_) * foo_cell_scale * scale_;
                float distance_from_center = sqrtf(distance_from_center_x * distance_from_center_x + distance_from_center_y * distance_from_center_y);

                if (distance_from_center >= mini_map_window_radius) {
                    float radian_x = acosf(distance_from_center_x / distance_from_center);
                    float radian_y = asinf(distance_from_center_y / distance_from_center);

                    distance_from_center_x = 55.0f * cosf(radian_x);
                    distance_from_center_y = 55.0f * sinf(radian_y);
                    atlas_mark_info_value.mini_map_x = (width_ - (float)white_mark_.width()) / 2.0f + distance_from_center_x + screen_x_ + 2.0f;
                    atlas_mark_info_value.mini_map_y = (height_ - (float)white_mark_.height()) / 2.0f + distance_from_center_y + screen_y_ + 2.0f;
                } else {
                    atlas_mark_info_value.mini_map_x = (width_ - (float)white_mark_.width()) / 2.0f + distance_from_center_x + screen_x_;
                    atlas_mark_info_value.mini_map_y = (height_ - (float)white_mark_.height()) / 2.0f + distance_from_center_y + screen_y_;
                }
            }
        }
    }

    void python_mini_map::render(float screen_x, float screen_y) {
        auto &background = get_python_background();
        if (!background.is_map_outdoor()) {
            return;
        }

        if (!show_) {
            return;
        }

        if (!background.is_map_ready()) {
            return;
        }

        if (screen_x_ != screen_x || screen_y_ != screen_y) {
            screen_x_ = screen_x;
            screen_y_ = screen_y;
            set_position_impl();
        }

        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MIP_FILTER, gpu::filter::POINT);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MIN_FILTER, gpu::filter::POINT);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MAG_FILTER, gpu::filter::POINT);

        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);

        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX, gpu::texcoord::CAMERA_SPACE_POSITION);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::COUNT2);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);

        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);

        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);

        STATEMANAGER.save_render_state(gpu::state::TEXTURE_FACTOR, 0xFF000000);

        STATEMANAGER.set_texture(1, mini_map_filter_graphic_image_instance_.texture_pointer()->gpu_texture());
        STATEMANAGER.set_transform(gpu::transform::TEXTURE1, &mini_map_cover_);

        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::TEX1);
        STATEMANAGER.set_stream_source(0, vertex_buffer_.gpu_buffer(), 20);
        STATEMANAGER.set_indices(index_buffer_.gpu_buffer(), 0);
        STATEMANAGER.set_transform(gpu::transform::WORLD, &world_);

        for (uint8_t terrain_num = 0; terrain_num < AROUND_AREA_NUM; ++terrain_num) {
            auto *mini_map_texture = mini_map_texture_[terrain_num];

            STATEMANAGER.set_texture(0, mini_map_texture);
            if (mini_map_texture) {
                state_manager &stt_mgr = get_state_manager();
                stt_mgr.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, terrain_num * 4, 4, terrain_num * 6, 2);
            } else {
                STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TFACTOR);
                STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, terrain_num * 4, 4, terrain_num * 6, 2);
                STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            }
        }

        STATEMANAGER.restore_render_state(gpu::state::TEXTURE_FACTOR);

        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ALPHA_ARG2);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ALPHA_OP);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::COLOR_ARG1);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::COLOR_ARG2);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::COLOR_OP);

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_OP);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG1);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG2);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_OP);

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ADDRESS_U);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ADDRESS_V);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_U);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_V);

        set_diffuse_operation();
        STATEMANAGER.set_transform(gpu::transform::WORLD, &identity_);

        STATEMANAGER.save_render_state(gpu::state::TEXTURE_FACTOR, 0xFFFFFFFF);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TFACTOR);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TEXTURE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TFACTOR);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::TEXTURE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG2);

        instance_position_vector_iterator iterator;

        if (scale_ >= 2.0f) {
            STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, instance_base::indexed_name_color(instance_base::NAMECOLOR_MOB));

            iterator = monster_position_vector_.begin();
            while (iterator != monster_position_vector_.end()) {
                mark_position &position = *iterator;
                white_mark_.set_position(position.x, position.y);
                white_mark_.render();
                ++iterator;
            }

            iterator = other_pc_position_vector_.begin();
            while (iterator != other_pc_position_vector_.end()) {
                mark_position &position = *iterator;
                STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, instance_base::indexed_name_color(position.name_color));
                white_mark_.set_position(position.x, position.y);
                white_mark_.render();
                ++iterator;
            }

            if (!party_pc_position_vector_.empty()) {
                float v = (1 + sinf(get_timer().current_second() * 6)) / 5 + 0.6;

                math::color c(instance_base::indexed_name_color(instance_base::NAMECOLOR_PARTY));
                math::color d(v, v, v, 1);

                c = math::color_modulate(c, d);

                STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, (uint32_t)c);

                iterator = party_pc_position_vector_.begin();
                while (iterator != party_pc_position_vector_.end()) {
                    mark_position &position = *iterator;
                    white_mark_.set_position(position.x, position.y);
                    white_mark_.render();
                    ++iterator;
                }
            }
        }

        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, instance_base::indexed_name_color(instance_base::NAMECOLOR_NPC));

        iterator = npc_position_vector_.begin();
        while (iterator != npc_position_vector_.end()) {
            mark_position &position = *iterator;
            white_mark_.set_position(position.x, position.y);
            white_mark_.render();
            ++iterator;
        }

        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, instance_base::indexed_name_color(instance_base::NAMECOLOR_WARP));

        iterator = warp_position_vector_.begin();
        while (iterator != warp_position_vector_.end()) {
            mark_position &position = *iterator;
            white_mark_.set_position(position.x, position.y);
            white_mark_.render();
            ++iterator;
        }

        STATEMANAGER.restore_render_state(gpu::state::TEXTURE_FACTOR);

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_OP);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG1);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG2);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_OP);

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MIP_FILTER);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MIN_FILTER);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MAG_FILTER);

        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);

        instance_base *inst = get_python_character_manager().main_instance_ptr();

        if (inst) {
            float rotation = (540.0f - inst->rotation());
            while (rotation > 360.0f) {
                rotation -= 360.0f;
            }

            while (rotation < 0.0f) {
                rotation += 360.0f;
            }

            player_mark_.set_rotation(rotation);
            player_mark_.render();
        }

        {
            auto itor = atlas_way_point_info_vector_.begin();
            for (; itor != atlas_way_point_info_vector_.end(); ++itor) {
                auto &atlas_mark_info_value = *itor;
                if (TYPE_TARGET != atlas_mark_info_value.type_value) {
                    continue;
                }

                if (atlas_mark_info_value.mini_map_x <= 0.0f) {
                    continue;
                }

                if (atlas_mark_info_value.mini_map_y <= 0.0f) {
                    continue;
                }

                render_target_mark(atlas_mark_info_value.mini_map_x, atlas_mark_info_value.mini_map_y);
            }
        }

        auto *camera = get_camera_manager().current_camera();
        if (camera) {
            mini_map_cameraraphic_image_instance_.set_rotation(camera->get_roll());
            mini_map_cameraraphic_image_instance_.render();
        }

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MIN_FILTER);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MAG_FILTER);
    }

    void python_mini_map::set_scale(float scale) {
        if (scale >= 4.0f) {
            scale = 4.0f;
        }

        if (scale <= 0.5f) {
            scale = 0.5f;
        }

        scale_ = scale;

        set_position_impl();
    }

    void python_mini_map::scale_up() {
        scale_ *= 2.0f;
        if (scale_ >= 4.0f) {
            scale_ = 4.0f;
        }

        set_position_impl();
    }

    void python_mini_map::scale_down() {
        scale_ *= 0.5f;
        if (scale_ <= 0.5f) {
            scale_ = 0.5f;
        }

        set_position_impl();
    }

    void python_mini_map::set_mini_map_size(float width_value, float height) {
        width_ = width_value;
        height_ = height;
    }

#pragma pack(push)
#pragma pack(1)
    struct minimapvertex {
        float x, y, z;
        float u, v; // texture coordinates
    };
    using lpminimapvertex = minimapvertex *;
#pragma pack(pop)

    auto python_mini_map::create() -> bool {
        const std::string image_root = "D:/ymir work/ui/";
        const std::string image_filter = image_root + "minimap_image_filter.dds";
        const std::string image_camera = image_root + "minimap_camera.dds";
        const std::string player_mark = image_root + "minimap/playermark.sub";
        const std::string white_mark = image_root + "minimap/whitemark.sub";

        auto *image = (graphic_image *)get_resource_manager().resource_pointer(image_filter.c_str());
        mini_map_filter_graphic_image_instance_.set_image_pointer(image);
        image = (graphic_image *)get_resource_manager().resource_pointer(image_camera.c_str());
        mini_map_cameraraphic_image_instance_.set_image_pointer(image);

        mini_map_cover_._11 = 1.0f / ((float)mini_map_filter_graphic_image_instance_.width());
        mini_map_cover_._22 = 1.0f / ((float)mini_map_filter_graphic_image_instance_.height());
        mini_map_cover_._33 = 0.0f;

        auto *sub_image = (graphic_sub_image *)get_resource_manager().resource_pointer(player_mark.c_str());
        player_mark_.set_image_pointer(sub_image);

        sub_image = (graphic_sub_image *)get_resource_manager().resource_pointer(white_mark.c_str());
        white_mark_.set_image_pointer(sub_image);

        char buf[256];
        for (int i = 0; i < MINI_WAYPOINT_IMAGE_COUNT; ++i) {
            snprintf(buf, sizeof(buf), "%sminimap/mini_waypoint%02d.sub", image_root.c_str(), i + 1);
            mini_way_point_graphic_image_instances_[i].set_image_pointer((graphic_sub_image *)get_resource_manager().resource_pointer(buf));
            mini_way_point_graphic_image_instances_[i].set_rendering_mode(graphic_expanded_image_instance::RENDERING_MODE_SCREEN);
        }

        for (int j = 0; j < WAYPOINT_IMAGE_COUNT; ++j) {
            snprintf(buf, sizeof(buf), "%sminimap/waypoint%02d.sub", image_root.c_str(), j + 1);
            way_point_graphic_image_instances_[j].set_image_pointer((graphic_sub_image *)get_resource_manager().resource_pointer(buf));
            way_point_graphic_image_instances_[j].set_rendering_mode(graphic_expanded_image_instance::RENDERING_MODE_SCREEN);
        }

        for (int k = 0; k < TARGET_MARK_IMAGE_COUNT; ++k) {
            snprintf(buf, sizeof(buf), "%sminimap/targetmark%02d.sub", image_root.c_str(), k + 1);
            target_mark_graphic_image_instances_[k].set_image_pointer((graphic_sub_image *)get_resource_manager().resource_pointer(buf));
            target_mark_graphic_image_instances_[k].set_rendering_mode(graphic_expanded_image_instance::RENDERING_MODE_SCREEN);
        }

        guild_area_flag_image_instance_.set_image_pointer((graphic_sub_image *)get_resource_manager().resource_pointer("d:/ymir work/ui/minimap/GuildArea01.sub"));

#pragma pack(push)
#pragma pack(1)
        lpminimapvertex mini_map_vertex;
        lpminimapvertex orig_mini_map_vertex;
#pragma pack(pop)

        if (!vertex_buffer_.create(36, gpu::fvf::XYZ | gpu::fvf::TEX1, gpu::usage::WRITE_ONLY)) {
            return false;
        }

        if (vertex_buffer_.lock((void **)&orig_mini_map_vertex)) {
            char *pch_mini_map_vertex = (char *)orig_mini_map_vertex;
            memset(pch_mini_map_vertex, 0, sizeof(char) * 720);
            mini_map_vertex = (lpminimapvertex)pch_mini_map_vertex;

            for (int i_y = -3; i_y <= 1; ++i_y) {
                if (0 == i_y % 2)
                    continue;
                float f_y = 0.5f * ((float)i_y);
                for (int i_x = -3; i_x <= 1; ++i_x) {
                    if (0 == i_x % 2)
                        continue;
                    float f_x = 0.5f * ((float)i_x);
                    mini_map_vertex = (lpminimapvertex)pch_mini_map_vertex;
                    mini_map_vertex->x = f_x;
                    mini_map_vertex->y = f_y;
                    mini_map_vertex->z = 0.0f;
                    mini_map_vertex->u = 0.0f;
                    mini_map_vertex->v = 0.0f;
                    pch_mini_map_vertex += 20;
                    mini_map_vertex = (lpminimapvertex)pch_mini_map_vertex;
                    mini_map_vertex->x = f_x;
                    mini_map_vertex->y = f_y + 1.0f;
                    mini_map_vertex->z = 0.0f;
                    mini_map_vertex->u = 0.0f;
                    mini_map_vertex->v = 1.0f;
                    pch_mini_map_vertex += 20;
                    mini_map_vertex = (lpminimapvertex)pch_mini_map_vertex;
                    mini_map_vertex->x = f_x + 1.0f;
                    mini_map_vertex->y = f_y;
                    mini_map_vertex->z = 0.0f;
                    mini_map_vertex->u = 1.0f;
                    mini_map_vertex->v = 0.0f;
                    pch_mini_map_vertex += 20;
                    mini_map_vertex = (lpminimapvertex)pch_mini_map_vertex;
                    mini_map_vertex->x = f_x + 1.0f;
                    mini_map_vertex->y = f_y + 1.0f;
                    mini_map_vertex->z = 0.0f;
                    mini_map_vertex->u = 1.0f;
                    mini_map_vertex->v = 1.0f;
                    pch_mini_map_vertex += 20;
                }
            }

            vertex_buffer_.unlock();
        }

        if (!index_buffer_.create(54, gpu::format::INDEX16)) {
            return false;
        }

        uint16_t pw_indices[54] = {
            0, 1, 2, 2, 1, 3,
            4, 5, 6, 6, 5, 7,
            8, 9, 10, 10, 9, 11,

            12, 13, 14, 14, 13, 15,
            16, 17, 18, 18, 17, 19,
            20, 21, 22, 22, 21, 23,

            24, 25, 26, 26, 25, 27,
            28, 29, 30, 30, 29, 31,
            32, 33, 34, 34, 33, 35};

        void *p_indices;

        if (index_buffer_.lock(&p_indices)) {
            memcpy(p_indices, pw_indices, 54 * sizeof(uint16_t));
            index_buffer_.unlock();
        }

        return true;
    }

    void python_mini_map::set_position_impl() {
        mini_map_radius_ = f_min(6400.0f / ((float)terrain_impl::CELLSCALE) * scale_, 64.0f);

        world_._11 = width_ * scale_;
        world_._22 = height_ * scale_;
        world_._41 = (1.0f + scale_) * width_ * 0.5f - center_cell_x_ * scale_ + screen_x_;
        world_._42 = (1.0f + scale_) * height_ * 0.5f - center_cell_y_ * scale_ + screen_y_;

        if (!mini_map_filter_graphic_image_instance_.is_empty()) {
            mini_map_cover_._41 = -(screen_x_) / ((float)mini_map_filter_graphic_image_instance_.width());
            mini_map_cover_._42 = -(screen_y_) / ((float)mini_map_filter_graphic_image_instance_.height());
        }

        if (!player_mark_.is_empty()) {
            player_mark_.set_position(
                (width_ - (float)player_mark_.width()) / 2.0f + screen_x_,
                (height_ - (float)player_mark_.height()) / 2.0f + screen_y_);
        }

        if (!mini_map_cameraraphic_image_instance_.is_empty()) {
            mini_map_cameraraphic_image_instance_.set_position(
                (width_ - (float)mini_map_cameraraphic_image_instance_.width()) / 2.0f + screen_x_,
                (height_ - (float)mini_map_cameraraphic_image_instance_.height()) / 2.0f + screen_y_);
        }
    }

    void python_mini_map::clear_atlas_mark_info() {
        atlas_npc_info_vector_.clear();
        atlas_warp_info_vector_.clear();
    }

    void python_mini_map::register_atlas_mark(uint8_t type, const char *name, int32_t lx, int32_t ly) {
        atlas_mark_info atlas_mark_info_value;

        atlas_mark_info_value.x = float(lx);
        atlas_mark_info_value.y = float(ly);
        atlas_mark_info_value.text = name;

        atlas_mark_info_value.screen_x = atlas_mark_info_value.x / atlas_max_x_ * atlas_image_size_x_ - (float)white_mark_.width() / 2.0f;
        atlas_mark_info_value.screen_y = atlas_mark_info_value.y / atlas_max_y_ * atlas_image_size_y_ - (float)white_mark_.height() / 2.0f;

        switch (type) {
        case actor_instance::TYPE_NPC:
            atlas_mark_info_value.type_value = TYPE_NPC;
            atlas_npc_info_vector_.push_back(atlas_mark_info_value);
            break;

        case actor_instance::TYPE_WARP:
            atlas_mark_info_value.type_value = TYPE_WARP;
            {
                int pos = atlas_mark_info_value.text.find(" ");
                if (pos >= 0)
                    atlas_mark_info_value.text[pos] = 0;
            }
            atlas_warp_info_vector_.push_back(atlas_mark_info_value);
            break;
        }
    }

    void python_mini_map::clear_guild_area() {
        guild_area_info_vector_value_.clear();
    }

    void python_mini_map::register_guild_area(uint32_t id, uint32_t guild_id, int32_t x, int32_t y, int32_t width_value, int32_t height) {
        guild_area_info info;

        info.guild_id = guild_id;
        info.lx = x;
        info.ly = y;
        info.lwidth = width_value;
        info.lheight = height;

        guild_area_info_vector_value_.push_back(info);
    }

    auto python_mini_map::guild_area_id(uint32_t x, uint32_t y) -> uint32_t {
        auto itor = guild_area_info_vector_value_.begin();
        for (; itor != guild_area_info_vector_value_.end(); ++itor) {
            guild_area_info &area_info = *itor;

            if (x >= uint32_t(area_info.lx)) {
                if (y >= uint32_t(area_info.ly)) {
                    if (x <= uint32_t(area_info.lx + area_info.lwidth)) {
                        if (y <= uint32_t(area_info.ly + area_info.lheight)) {
                            return area_info.guild_id;
                        }
                    }
                }
            }
        }

        return 0xffffffff;
    }

    void python_mini_map::create_target(int id, const char *name) {
        add_way_point(TYPE_TARGET, id, 0.0f, 0.0f, name);
    }

    void python_mini_map::update_target(int id, int ix, int iy) {
        atlas_mark_info *info;
        if (!way_point(id, &info)) {
            return;
        }

        if (0 != info->chr_vid) {
            if (get_python_character_manager().instance_ptr(info->chr_vid)) {
                return;
            }
        }

        if (uint32_t(ix) < atlas_base_x_) {
            return;
        }

        if (uint32_t(iy) < atlas_base_y_) {
            return;
        }

        if (uint32_t(ix) > atlas_base_x_ + uint32_t(atlas_max_x_)) {
            return;
        }

        if (uint32_t(iy) > atlas_base_y_ + uint32_t(atlas_max_y_)) {
            return;
        }

        update_way_point(info, ix - int(atlas_base_x_), iy - int(atlas_base_y_));
    }

    void python_mini_map::create_target(int id, const char *name, uint32_t vid) {
        add_way_point(TYPE_TARGET, id, 0.0f, 0.0f, name, vid);
    }

    void python_mini_map::delete_target(int id) {
        remove_way_point(id);
    }

    void python_mini_map::load_atlas_mark_info() {
        clear_atlas_mark_info();
        clear_guild_area();

        auto &background = get_python_background();
        if (!background.is_map_outdoor()) {
            return;
        }

        auto &map = background.map_outdoor_ref();

        const std::string atlas_mark_info_file_name = std::format("{}/map/{}_point.txt", LOCALE_DIR, map.get_name());

        token_vector_map token_vector_map;

        if (!load_multiple_text_data(atlas_mark_info_file_name.c_str(), token_vector_map)) {
            metin2::log::debug("Cannot load the atlas mark info {}", atlas_mark_info_file_name);
            return;
        }

        const std::string str_type[TYPE_COUNT] = {"OPC", "OPCPVP", "OPCPVPSELF", "NPC", "MONSTER", "WARP", "WAYPOINT"};

        for (uint32_t i = 0; i < token_vector_map.size(); ++i) {
            char mark_info_name[32 + 1];
            snprintf(mark_info_name, sizeof(mark_info_name), "%d", i);

            if (token_vector_map.end() == token_vector_map.find(mark_info_name)) {
                continue;
            }

            const token_vector &vector = token_vector_map[mark_info_name];

            const std::string &c_rstr_type = vector[0].c_str();
            const std::string &position_x = vector[1].c_str();
            const std::string &position_y = vector[2].c_str();
            const std::string &text = vector[3].c_str();

            atlas_mark_info atlas_mark_info_value;

            for (int i = 0; i < TYPE_COUNT; ++i) {
                if (0 == c_rstr_type.compare(str_type[i])) {
                    atlas_mark_info_value.type_value = (uint8_t)i;
                }
            }

            atlas_mark_info_value.x = metin2::to_number<double>(position_x.c_str());
            atlas_mark_info_value.y = metin2::to_number<double>(position_y.c_str());
            atlas_mark_info_value.text = text;

            atlas_mark_info_value.screen_x = atlas_mark_info_value.x / atlas_max_x_ * atlas_image_size_x_ - (float)white_mark_.width() / 2.0f;
            atlas_mark_info_value.screen_y = atlas_mark_info_value.y / atlas_max_y_ * atlas_image_size_y_ - (float)white_mark_.height() / 2.0f;

            switch (atlas_mark_info_value.type_value) {
            case TYPE_NPC:
                atlas_npc_info_vector_.push_back(atlas_mark_info_value);
                break;

            case TYPE_WARP:
                atlas_warp_info_vector_.push_back(atlas_mark_info_value);
                break;
            }
        }
    }

    auto python_mini_map::load_atlas() -> bool {
        auto &background = get_python_background();
        if (!background.is_map_outdoor()) {
            return false;
        }

        auto &map = background.map_outdoor_ref();

        const char *player_mark_file_name = "d:/ymir work/ui/minimap/playermark.sub";

        char atlas_file_name[1024 + 1];
        snprintf(atlas_file_name, sizeof(atlas_file_name), "%s/atlas.sub", map.get_name().c_str());
        if (!get_file_manager().is_exist(atlas_file_name)) {
            snprintf(atlas_file_name, sizeof(atlas_file_name), "d:/ymir work/ui/atlas/%s/atlas.sub", map.get_name().c_str());
        }

        atlas_image_instance_.destroy();
        atlas_player_mark_.destroy();

        auto *grp_img_atlas = (graphic_image *)get_resource_manager().resource_pointer(atlas_file_name);
        if (grp_img_atlas) {
            atlas_image_instance_.set_image_pointer(grp_img_atlas);
            atlas_ = !grp_img_atlas->is_empty();
        }

        atlas_player_mark_.set_image_pointer((graphic_sub_image *)get_resource_manager().resource_pointer(player_mark_file_name));

        short terrain_count_x, terrain_count_y;
        map.base_xy(&atlas_base_x_, &atlas_base_y_);
        map.terrain_count(&terrain_count_x, &terrain_count_y);

        atlas_max_x_ = (float)terrain_count_x * static_cast<float>(terrain_impl::TERRAIN_XSIZE);
        atlas_max_y_ = (float)terrain_count_y * static_cast<float>(terrain_impl::TERRAIN_YSIZE);
        atlas_image_size_x_ = (float)atlas_image_instance_.width();
        atlas_image_size_y_ = (float)atlas_image_instance_.height();

        load_atlas_mark_info();

        if (show_atlas_) {
            open_atlas_window();
        }

        return true;
    }

    void python_mini_map::global_position_to_atlas_position(int32_t lx, int32_t ly, float *pfx, float *pfy) {
        *pfx = lx / atlas_max_x_ * atlas_image_size_x_;
        *pfy = ly / atlas_max_y_ * atlas_image_size_y_;
    }

    void python_mini_map::update_atlas() {
        instance_base *inst = get_python_character_manager().main_instance_ptr();

        if (inst) {
            pixel_position inst_pos;
            inst->new_get_pixel_position(&inst_pos);

            auto rotation = (540.0f - inst->rotation());
            while (rotation > 360.0f) {
                rotation -= 360.0f;
            }

            while (rotation < 0.0f) {
                rotation += 360.0f;
            }

            atlas_player_mark_.set_position(
                inst_pos.x / atlas_max_x_ * atlas_image_size_x_ - (float)atlas_player_mark_.width() / 2.0f,
                inst_pos.y / atlas_max_y_ * atlas_image_size_y_ - (float)atlas_player_mark_.height() / 2.0f);
            atlas_player_mark_.set_rotation(rotation);
        }

        {
            auto itor = guild_area_info_vector_value_.begin();
            for (; itor != guild_area_info_vector_value_.end(); ++itor) {
                guild_area_info &info = *itor;

                global_position_to_atlas_position(info.lx, info.ly, &info.fsx_render, &info.fsy_render);
                global_position_to_atlas_position(info.lx + info.lwidth, info.ly + info.lheight, &info.fex_render, &info.fey_render);
            }
        }
    }

    void python_mini_map::render_atlas(float screen_x, float screen_y) {
        if (!show_atlas_) {
            return;
        }

        if (atlas_screen_x_ != screen_x || atlas_screen_y_ != screen_y) {
            world_atlas_._41 = screen_x;
            world_atlas_._42 = screen_y;
            atlas_screen_x_ = screen_x;
            atlas_screen_y_ = screen_y;
        }

        STATEMANAGER.set_transform(gpu::transform::WORLD, &world_atlas_);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MIN_FILTER, gpu::filter::POINT);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MAG_FILTER, gpu::filter::POINT);
        atlas_image_instance_.render();

        STATEMANAGER.save_render_state(gpu::state::TEXTURE_FACTOR, 0xFFFFFFFF);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TFACTOR);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TEXTURE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);

        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, instance_base::indexed_name_color(instance_base::NAMECOLOR_NPC));
        atlas_mark_info_vector_iterator_value_ = atlas_npc_info_vector_.begin();
        while (atlas_mark_info_vector_iterator_value_ != atlas_npc_info_vector_.end()) {
            atlas_mark_info &atlas_mark_info_value = *atlas_mark_info_vector_iterator_value_;
            white_mark_.set_position(atlas_mark_info_value.screen_x, atlas_mark_info_value.screen_y);
            white_mark_.render();
            ++atlas_mark_info_vector_iterator_value_;
        }

        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, instance_base::indexed_name_color(instance_base::NAMECOLOR_WARP));
        atlas_mark_info_vector_iterator_value_ = atlas_warp_info_vector_.begin();
        while (atlas_mark_info_vector_iterator_value_ != atlas_warp_info_vector_.end()) {
            atlas_mark_info &atlas_mark_info_value = *atlas_mark_info_vector_iterator_value_;
            white_mark_.set_position(atlas_mark_info_value.screen_x, atlas_mark_info_value.screen_y);
            white_mark_.render();
            ++atlas_mark_info_vector_iterator_value_;
        }

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, instance_base::indexed_name_color(instance_base::NAMECOLOR_WAYPOINT));
        atlas_mark_info_vector_iterator_value_ = atlas_way_point_info_vector_.begin();
        for (; atlas_mark_info_vector_iterator_value_ != atlas_way_point_info_vector_.end(); ++atlas_mark_info_vector_iterator_value_) {
            atlas_mark_info &atlas_mark_info_value = *atlas_mark_info_vector_iterator_value_;

            if (atlas_mark_info_value.screen_x <= 0.0f) {
                continue;
            }

            if (atlas_mark_info_value.screen_y <= 0.0f) {
                continue;
            }

            if (TYPE_TARGET == atlas_mark_info_value.type_value) {
                render_mini_way_point_mark(atlas_mark_info_value.screen_x, atlas_mark_info_value.screen_y);
            } else {
                render_way_point_mark(atlas_mark_info_value.screen_x, atlas_mark_info_value.screen_y);
            }
        }

        STATEMANAGER.restore_render_state(gpu::state::TEXTURE_FACTOR);

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG1);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG2);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_OP);

        if ((el_timer_get_m_sec() / 500) % 2) {
            atlas_player_mark_.render();
        }

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MIN_FILTER);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MAG_FILTER);
        STATEMANAGER.set_transform(gpu::transform::WORLD, &identity_);

        {
            auto itor = guild_area_info_vector_value_.begin();
            for (; itor != guild_area_info_vector_value_.end(); ++itor) {
                guild_area_info &info = *itor;

                guild_area_flag_image_instance_.set_position(
                    screen_x + (info.fsx_render + info.fex_render) / 2.0f - guild_area_flag_image_instance_.width() / 2,
                    screen_y + (info.fsy_render + info.fey_render) / 2.0f - guild_area_flag_image_instance_.height() / 2);
                guild_area_flag_image_instance_.render();
            }
        }
    }

    auto python_mini_map::picked_instance_info(float screen_x, float screen_y, std::string &return_name, float *return_pos_x, float *return_pos_y, uint32_t *text_color) -> bool {
        float distance_from_mini_map_center_x = screen_x - screen_x_ - width_ * 0.5f;
        float distance_from_mini_map_center_y = screen_y - screen_y_ - height_ * 0.5f;

        if (sqrtf(distance_from_mini_map_center_x * distance_from_mini_map_center_x + distance_from_mini_map_center_y * distance_from_mini_map_center_y) > mini_map_radius_) {
            return false;
        }

        float real_x = center_x_ + distance_from_mini_map_center_x / scale_ * ((float)terrain_impl::CELLSCALE);
        float real_y = center_y_ + distance_from_mini_map_center_y / scale_ * ((float)terrain_impl::CELLSCALE);

        instance_base *inst = get_python_character_manager().main_instance_ptr();

        if (inst) {
            pixel_position inst_pos;
            inst->new_get_pixel_position(&inst_pos);

            if (fabs(inst_pos.x - real_x) < ((float)terrain_impl::CELLSCALE) * 6.0f / scale_ &&
                fabs(inst_pos.y - real_y) < ((float)terrain_impl::CELLSCALE) * 6.0f / scale_) {
                return_name = inst->name_string();
                *return_pos_x = inst_pos.x;
                *return_pos_y = inst_pos.y;
                *text_color = inst->name_color();
                return true;
            }
        }

        if (scale_ < 1.0f)
            return false;

        python_character_manager &chr_mgr = get_python_character_manager();
        python_character_manager::character_iterator i;
        for (i = chr_mgr.character_instance_begin(); i != chr_mgr.character_instance_end(); ++i) {
            instance_base *inst_each = *i;
            if (inst_each->is_invisibility()) {
                continue;
            }

            if (scale_ < 2.0f && (inst_each->is_enemy() || inst_each->is_pc())) {
                continue;
            }

            pixel_position instance_position;
            inst_each->new_get_pixel_position(&instance_position);

            if (fabs(instance_position.x - real_x) < ((float)terrain_impl::CELLSCALE) * 3.0f / scale_ &&
                fabs(instance_position.y - real_y) < ((float)terrain_impl::CELLSCALE) * 3.0f / scale_) {
                return_name = inst_each->name_string();
                *return_pos_x = instance_position.x;
                *return_pos_y = instance_position.y;
                *text_color = inst_each->name_color();
                return true;
            }
        }
        return false;
    }

    auto python_mini_map::atlas_info(float screen_x, float screen_y, std::string &return_string, float *return_pos_x, float *return_pos_y, uint32_t *text_color, uint32_t *guild_id) -> bool {
        float real_x = (screen_x - atlas_screen_x_) * (atlas_max_x_ / atlas_image_size_x_);
        float real_y = (screen_y - atlas_screen_y_) * (atlas_max_y_ / atlas_image_size_y_);

        float check_width = (atlas_max_x_ / atlas_image_size_x_) * 5.0f;
        float check_height = (atlas_max_y_ / atlas_image_size_y_) * 5.0f;

        instance_base *inst = get_python_character_manager().main_instance_ptr();

        if (inst) {
            pixel_position inst_pos;
            inst->new_get_pixel_position(&inst_pos);

            if (inst_pos.x - check_width < real_x && inst_pos.x + check_width > real_x &&
                inst_pos.y - check_height < real_y && inst_pos.y + check_height > real_y) {
                return_string = inst->name_string();
                *return_pos_x = inst_pos.x;
                *return_pos_y = inst_pos.y;
                *text_color = inst->name_color();
                return true;
            }
        }

        atlas_mark_info_vector_iterator_value_ = atlas_npc_info_vector_.begin();
        while (atlas_mark_info_vector_iterator_value_ != atlas_npc_info_vector_.end()) {
            atlas_mark_info &atlas_mark_info_value = *atlas_mark_info_vector_iterator_value_;

            if (atlas_mark_info_value.x - check_width / 2 < real_x && atlas_mark_info_value.x + check_width > real_x &&
                atlas_mark_info_value.y - check_width / 2 < real_y && atlas_mark_info_value.y + check_height > real_y) {
                return_string = atlas_mark_info_value.text;
                *return_pos_x = atlas_mark_info_value.x;
                *return_pos_y = atlas_mark_info_value.y;
                *text_color = instance_base::indexed_name_color(instance_base::NAMECOLOR_NPC);
                return true;
            }

            ++atlas_mark_info_vector_iterator_value_;
        }

        atlas_mark_info_vector_iterator_value_ = atlas_warp_info_vector_.begin();
        while (atlas_mark_info_vector_iterator_value_ != atlas_warp_info_vector_.end()) {
            atlas_mark_info &atlas_mark_info_value = *atlas_mark_info_vector_iterator_value_;
            if (atlas_mark_info_value.x - check_width / 2 < real_x && atlas_mark_info_value.x + check_width > real_x &&
                atlas_mark_info_value.y - check_width / 2 < real_y && atlas_mark_info_value.y + check_height > real_y) {
                return_string = atlas_mark_info_value.text;
                *return_pos_x = atlas_mark_info_value.x;
                *return_pos_y = atlas_mark_info_value.y;
                *text_color = instance_base::indexed_name_color(instance_base::NAMECOLOR_WARP);
                return true;
            }

            ++atlas_mark_info_vector_iterator_value_;
        }

        atlas_mark_info_vector_iterator_value_ = atlas_way_point_info_vector_.begin();
        while (atlas_mark_info_vector_iterator_value_ != atlas_way_point_info_vector_.end()) {
            atlas_mark_info &atlas_mark_info_value = *atlas_mark_info_vector_iterator_value_;
            if (atlas_mark_info_value.screen_x > 0.0f) {
                if (atlas_mark_info_value.screen_y > 0.0f) {
                    if (atlas_mark_info_value.x - check_width / 2 < real_x && atlas_mark_info_value.x + check_width > real_x &&
                        atlas_mark_info_value.y - check_width / 2 < real_y && atlas_mark_info_value.y + check_height > real_y) {
                        return_string = atlas_mark_info_value.text;
                        *return_pos_x = atlas_mark_info_value.x;
                        *return_pos_y = atlas_mark_info_value.y;
                        *text_color = instance_base::indexed_name_color(instance_base::NAMECOLOR_WAYPOINT);
                        return true;
                    }
                }
            }
            ++atlas_mark_info_vector_iterator_value_;
        }

        auto itor = guild_area_info_vector_value_.begin();
        for (; itor != guild_area_info_vector_value_.end(); ++itor) {
            guild_area_info &info = *itor;

            if (screen_x - atlas_screen_x_ >= info.fsx_render) {
                if (screen_y - atlas_screen_y_ >= info.fsy_render) {
                    if (screen_x - atlas_screen_x_ <= info.fex_render) {
                        if (screen_y - atlas_screen_y_ <= info.fey_render) {
                            if (get_python_guild().guild_name(info.guild_id, &return_string)) {
                                *guild_id = info.guild_id;
                            } else {
                                return_string = "empty_guild_area";
                            }

                            *return_pos_x = info.lx + info.lwidth / 2;
                            *return_pos_y = info.ly + info.lheight / 2;
                            *text_color = instance_base::indexed_name_color(instance_base::NAMECOLOR_PARTY);
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    auto python_mini_map::atlas_size(float *size_x, float *size_y) -> bool {
        if (!get_python_background().is_map_outdoor()) {
            return false;
        }

        *size_x = atlas_image_size_x_;
        *size_y = atlas_image_size_y_;

        return true;
    }

    void python_mini_map::add_way_point(uint8_t type, uint32_t id, float x, float y, std::string text, uint32_t chr_vid) {
        atlas_mark_info_vector_iterator_value_ = atlas_way_point_info_vector_.begin();
        while (atlas_mark_info_vector_iterator_value_ != atlas_way_point_info_vector_.end()) {
            atlas_mark_info &r_atlas_mark_info = *atlas_mark_info_vector_iterator_value_;
            if (r_atlas_mark_info.id == id) {
                return;
            }

            ++atlas_mark_info_vector_iterator_value_;
        }

        atlas_mark_info a_atlas_mark_info;

        a_atlas_mark_info.type_value = type;
        a_atlas_mark_info.id = id;
        a_atlas_mark_info.x = x;
        a_atlas_mark_info.y = y;
        a_atlas_mark_info.screen_x = 0.0f;
        a_atlas_mark_info.screen_y = 0.0f;
        a_atlas_mark_info.mini_map_x = 0.0f;
        a_atlas_mark_info.mini_map_y = 0.0f;
        a_atlas_mark_info.text = text;
        a_atlas_mark_info.chr_vid = chr_vid;

        update_way_point(&a_atlas_mark_info, x, y);

        atlas_way_point_info_vector_.push_back(a_atlas_mark_info);
    }

    void python_mini_map::remove_way_point(uint32_t id) {
        atlas_mark_info_vector_iterator_value_ = atlas_way_point_info_vector_.begin();
        while (atlas_mark_info_vector_iterator_value_ != atlas_way_point_info_vector_.end()) {
            atlas_mark_info &atlas_mark_info_value = *atlas_mark_info_vector_iterator_value_;
            if (atlas_mark_info_value.id == id) {
                atlas_mark_info_vector_iterator_value_ = atlas_way_point_info_vector_.erase(atlas_mark_info_vector_iterator_value_);
                return;
            }

            ++atlas_mark_info_vector_iterator_value_;
        }
    }

    auto python_mini_map::way_point(uint32_t id, atlas_mark_info **ppk_info) -> bool {
        auto itor = atlas_way_point_info_vector_.begin();
        for (; itor != atlas_way_point_info_vector_.end(); ++itor) {
            atlas_mark_info &r_info = *itor;
            if (id == r_info.id) {
                *ppk_info = &r_info;
                return true;
            }
        }

        return false;
    }

    void python_mini_map::update_way_point(atlas_mark_info *info, int ix, int iy) {
        info->x = float(ix);
        info->y = float(iy);
        info->screen_x = info->x / atlas_max_x_ * atlas_image_size_x_;
        info->screen_y = info->y / atlas_max_y_ * atlas_image_size_y_;
    }

    void python_mini_map::render_way_point_mark(int ix_center, int iy_center) {
        int num = (el_timer_get_m_sec() / 67) % WAYPOINT_IMAGE_COUNT;

        auto &instance = way_point_graphic_image_instances_[num];
        instance.set_position(ix_center - instance.width() / 2, iy_center - instance.height() / 2);
        instance.render();
    }

    void python_mini_map::render_mini_way_point_mark(int ix_center, int iy_center) {
        int num = (el_timer_get_m_sec() / 67) % MINI_WAYPOINT_IMAGE_COUNT;

        auto &instance = mini_way_point_graphic_image_instances_[num];
        instance.set_position(ix_center - instance.width() / 2, iy_center - instance.height() / 2);
        instance.render();
    }

    void python_mini_map::render_target_mark(int ix_center, int iy_center) {
        int num = (el_timer_get_m_sec() / 80) % TARGET_MARK_IMAGE_COUNT;

        auto &instance = target_mark_graphic_image_instances_[num];
        instance.set_position(ix_center - instance.width() / 2, iy_center - instance.height() / 2);
        instance.render();
    }

    void python_mini_map::add_signal_point(float x, float y) {
        static unsigned int g_id = 255;

        signal_point sp;
        sp.id = g_id;
        sp.pos.x = x;
        sp.pos.y = y;

        signal_point_vector_.push_back(sp);

        add_way_point(TYPE_WAYPOINT, g_id, x, y, "");

        g_id++;
    }

    void python_mini_map::clear_all_signal_point() {
        std::vector<signal_point>::iterator it;
        for (it = signal_point_vector_.begin(); it != signal_point_vector_.end(); ++it) {
            remove_way_point(it->id);
        }
        signal_point_vector_.clear();
    }

    void python_mini_map::register_atlas_window(PyObject *handler) {
        handler_ = handler;
    }

    void python_mini_map::unregister_atlas_window() {
        handler_ = nullptr;
    }

    void python_mini_map::open_atlas_window() {
        if (handler_) {
            py_call_class_member_func(handler_, "show", Py_BuildValue("()"));
        }
    }

    void python_mini_map::set_atlas_center_position(int x, int y) {
        if (handler_) {
            py_call_class_member_func(handler_, "set_center_position_adjust", Py_BuildValue("(ii)", x, y));
        }
    }

    auto python_mini_map::is_atlas() -> bool {
        return atlas_;
    }

    void python_mini_map::show_atlas() {
        show_atlas_ = true;
    }

    void python_mini_map::hide_atlas() {
        show_atlas_ = false;
    }

    auto python_mini_map::can_show() -> bool {
        return show_;
    }

    void python_mini_map::show() {
        show_ = true;
    }

    void python_mini_map::hide() {
        show_ = false;
    }

    void python_mini_map::initialize_impl() {
        handler_ = nullptr;

        set_mini_map_size(128.0f, 128.0f);

        scale_ = 2.0f;

        center_x_ = width_ * 0.5f;
        center_y_ = height_ * 0.5f;

        screen_x_ = 0.0f;
        screen_y_ = 0.0f;

        atlas_screen_x_ = 0.0f;
        atlas_screen_y_ = 0.0f;

        atlas_base_x_ = 0;
        atlas_base_y_ = 0;

        atlas_max_x_ = 0.0f;
        atlas_max_y_ = 0.0f;

        atlas_image_size_x_ = 0.0f;
        atlas_image_size_y_ = 0.0f;

        atlas_ = false;

        show_ = false;
        show_atlas_ = false;

        identity_ = math::matrix_identity();
        world_ = math::matrix_identity();
        mini_map_cover_ = math::matrix_identity();
        world_atlas_ = math::matrix_identity();
    }

    void python_mini_map::destroy() {
        clear_all_signal_point();

        handler_ = nullptr;

        vertex_buffer_.destroy();
        index_buffer_.destroy();

        player_mark_.destroy();

        mini_map_filter_graphic_image_instance_.destroy();
        mini_map_cameraraphic_image_instance_.destroy();

        atlas_way_point_info_vector_.clear();
        atlas_image_instance_.destroy();
        atlas_player_mark_.destroy();
        white_mark_.destroy();

        for (int i = 0; i < MINI_WAYPOINT_IMAGE_COUNT; ++i) {
            mini_way_point_graphic_image_instances_[i].destroy();
        }

        for (int j = 0; j < WAYPOINT_IMAGE_COUNT; ++j) {
            way_point_graphic_image_instances_[j].destroy();
        }

        for (int k = 0; k < TARGET_MARK_IMAGE_COUNT; ++k) {
            target_mark_graphic_image_instances_[k].destroy();
        }

        guild_area_flag_image_instance_.destroy();

        initialize_impl();
    }

    python_mini_map::python_mini_map() {
        initialize_impl();
    }

    python_mini_map::~python_mini_map() {
        destroy();
    }
}
