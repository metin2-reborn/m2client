#include "python_character_manager.h"
#include <metin2/client/app/abstract_player.h>
#include <metin2/client/background/python_background.h>
#include <metin2/client/protocol/packet.h>
#include <metin2/log/log.h>
#include "python_non_player.h"

#include <metin2/gpu/types.h>
#include <metin2/graphics/camera.h>
#include <metin2/ui/python_graphic.h>

namespace metin2::inline client::inline characters {

    int char_stage_view_bound = 200 * 100;

    struct character_manager_character_instance_update_fn {
        inline void operator()(const std::pair<uint32_t, instance_base *> &cr_pair) {
            cr_pair.second->update();
        }
    };

    void python_character_manager::adjust_collision_with_other_objects(actor_instance *inst) {
        if (!inst->is_pc())
            return;

        python_character_manager &chr_mgr = get_python_character_manager();
        for (python_character_manager::character_iterator i = chr_mgr.character_instance_begin(); i != chr_mgr.character_instance_end(); ++i) {
            instance_base *inst_each = *i;
            actor_instance *actor_each = inst_each->graphic_thing_instance_ptr();

            if (actor_each == inst)
                continue;

            if (actor_each->is_pc() || actor_each->is_npc() || actor_each->is_enemy())
                continue;

            if (inst->test_physics_blending_collision(*actor_each)) {
                // For now, snap back to the previous position.
                // TODO: step the physics movement in smaller increments instead.
                pixel_position cur_pos;
                inst->get_pixel_position(&cur_pos);
                inst->set_blending_position(cur_pos);
                break;
            }
        }
    }

    void python_character_manager::insert_pvp_key(uint32_t vid_src, uint32_t vid_dst) {
        instance_base::insert_pvp_key(vid_src, vid_dst);

        instance_base *inst_src = instance_ptr(vid_src);
        if (inst_src)
            inst_src->refresh_text_tail();

        instance_base *inst_dst = instance_ptr(vid_dst);
        if (inst_dst)
            inst_dst->refresh_text_tail();
    }

    void python_character_manager::remove_pvp_key(uint32_t vid_src, uint32_t vid_dst) {
        instance_base::remove_pvp_key(vid_src, vid_dst);

        instance_base *inst_src = instance_ptr(vid_src);
        if (inst_src)
            inst_src->refresh_text_tail();

        instance_base *inst_dst = instance_ptr(vid_dst);
        if (inst_dst)
            inst_dst->refresh_text_tail();
    }

    void python_character_manager::change_gvg(uint32_t src_guild_id, uint32_t dst_guild_id) {
        character_instance_map::iterator itor;
        for (itor = alive_inst_map_.begin(); itor != alive_inst_map_.end(); itor++) {
            instance_base *instance = itor->second;

            uint32_t instance_guild_id = instance->guild_id();
            if (src_guild_id == instance_guild_id || dst_guild_id == instance_guild_id) {
                instance->refresh_text_tail();
            }
        }
    }

    void python_character_manager::clear_main_instance() {
        inst_main_ = nullptr;
    }

    auto python_character_manager::set_main_instance(uint32_t vid) -> bool {
        inst_main_ = instance_ptr(vid);

        if (!inst_main_)
            return false;

        return true;
    }

    auto python_character_manager::main_instance_ptr() -> instance_base * {
        return inst_main_;
    }

    void python_character_manager::get_info(std::string *pst_info) {
        pst_info->append("Actor: ");

        instance_base::get_info(pst_info);

        char sz_info[256];
        snprintf(sz_info, sizeof(sz_info), "Container - Live %zu, Dead %zu", alive_inst_map_.size(), dead_inst_list_.size());
        pst_info->append(sz_info);
    }

    void python_character_manager::update() {
        instance_base::reset_performance_counter();

        instance_base *inst_main = main_instance_ptr();

        auto i = alive_inst_map_.begin();
        while (alive_inst_map_.end() != i) {
            auto c = i++;

            instance_base *inst_each = c->second;
            inst_each->update();

            if (inst_main) {
                if (inst_each->is_force_visible()) {
                    continue;
                }

                int distance = int(inst_each->new_get_distance_from_dest_instance(*inst_main));
                if (distance > char_stage_view_bound + 10) {
                    delete_blend_out_instance(inst_each);
                    alive_inst_map_.erase(c);
                }
            }
        }
        update_transform();

        update_deleting();

        new_pick();
    }

    void python_character_manager::show_point_effect(uint32_t point, uint32_t vid) {
        instance_base *inst_sel = (vid == 0xffffffff) ? main_instance_ptr() : instance_ptr(vid);

        if (!inst_sel)
            return;

        switch (point) {
        case POINT_LEVEL:
            inst_sel->level_up();
            break;
        case POINT_LEVEL_STEP:
            inst_sel->skill_up();
            break;
        }
    }

    void python_character_manager::update_transform() {

        instance_base *main_instance = main_instance_ptr();
        if (main_instance) {
            python_background &bg = get_python_background();
            for (auto i = alive_inst_map_.begin(); i != alive_inst_map_.end(); ++i) {
                instance_base *src_instance = i->second;

                src_instance->check_advancing();

                // IsAttacked is also true for dead actors,
                // so only check IsPushing, where the distance really changes
                if (src_instance->is_pushing())
                    bg.check_advancing(src_instance);
            }

            bg.check_advancing(inst_main_);
        }

        {
            for (auto itor = alive_inst_map_.begin(); itor != alive_inst_map_.end(); ++itor) {
                instance_base *instance = itor->second;
                instance->transform();
            }
        }
    }
    void python_character_manager::update_deleting() {
        auto itor = dead_inst_list_.begin();
        for (; itor != dead_inst_list_.end();) {
            instance_base *instance = *itor;

            if (instance->update_deleting()) {
                ++itor;
            } else {
                instance_base::delete_value(instance);
                itor = dead_inst_list_.erase(itor);
            }
        }
    }

    struct character_manager_character_instance_deform_fn {
        inline void operator()(const std::pair<uint32_t, instance_base *> &cr_pair) {
            cr_pair.second->deform();
        }
    };
    struct character_manager_character_instance_list_deform_fn {
        inline void operator()(instance_base *instance) {
            instance->deform();
        }
    };

    void python_character_manager::deform() {
        std::for_each(alive_inst_map_.begin(), alive_inst_map_.end(), character_manager_character_instance_deform_fn());
        std::for_each(dead_inst_list_.begin(), dead_inst_list_.end(), character_manager_character_instance_list_deform_fn());
    }

    auto python_character_manager::old_get_picked_instance_vid(uint32_t *picked_actor_id) -> bool {
        if (!inst_pick_)
            return false;

        *picked_actor_id = inst_pick_->virtual_id();
        return true;
    }

    auto python_character_manager::old_get_picked_instance_ptr() -> instance_base * {
        return inst_pick_;
    }

    auto python_character_manager::is_registered_vid(uint32_t vid) -> bool {
        if (alive_inst_map_.end() == alive_inst_map_.find(vid))
            return false;

        return true;
    }

    auto python_character_manager::is_alive_vid(uint32_t vid) -> bool {
        return alive_inst_map_.find(vid) != alive_inst_map_.end();
    }

    auto python_character_manager::is_dead_vid(uint32_t vid) -> bool {
        for (auto f = dead_inst_list_.begin(); f != dead_inst_list_.end(); ++f) {
            if ((*f)->virtual_id() == vid)
                return true;
        }

        return false;
    }

    struct less_character_instance_ptr_render_order {
        auto operator()(instance_base *left, instance_base *right) -> bool {
            return left->less_render_order(right);
        }
    };

    struct character_manager_character_instance_render_fn {
        inline void operator()(const std::pair<uint32_t, instance_base *> &cr_pair) {
            cr_pair.second->render();
            cr_pair.second->render_trace();
        }
    };
    struct character_instance_render_fn {
        inline void operator()(instance_base *instance) {
            instance->render();
        }
    };
    struct character_instance_render_trace_fn {
        inline void operator()(instance_base *instance) {
            instance->render_trace();
        }
    };

    void python_character_manager::render_sorted_alive_actor_list() {
        static std::vector<instance_base *> vct_inst_alive_sort;
        vct_inst_alive_sort.clear();

        character_instance_map &map_inst_alive = alive_inst_map_;
        character_instance_map::iterator i;
        for (i = map_inst_alive.begin(); i != map_inst_alive.end(); ++i)
            vct_inst_alive_sort.push_back(i->second);

        std::sort(vct_inst_alive_sort.begin(), vct_inst_alive_sort.end(), less_character_instance_ptr_render_order());
        std::for_each(vct_inst_alive_sort.begin(), vct_inst_alive_sort.end(), character_instance_render_fn());
        std::for_each(vct_inst_alive_sort.begin(), vct_inst_alive_sort.end(), character_instance_render_trace_fn());
    }

    void python_character_manager::render_sorted_dead_actor_list() {
        static std::vector<instance_base *> vct_inst_dead_sort;
        vct_inst_dead_sort.clear();

        character_instance_list &lst_inst_dead = dead_inst_list_;
        character_instance_list::iterator i;
        for (i = lst_inst_dead.begin(); i != lst_inst_dead.end(); ++i)
            vct_inst_dead_sort.push_back(*i);

        std::sort(vct_inst_dead_sort.begin(), vct_inst_dead_sort.end(), less_character_instance_ptr_render_order());
        std::for_each(vct_inst_dead_sort.begin(), vct_inst_dead_sort.end(), character_instance_render_fn());
    }

    void python_character_manager::render() {
        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);

        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        render_sorted_alive_actor_list();
        render_sorted_dead_actor_list();

        instance_base *picked_inst = old_get_picked_instance_ptr();
        if (picked_inst) {
            const math::vector3 &position = picked_inst->graphic_thing_instance_ref().get_position();
            get_python_graphic().project_position(position.x, position.y, position.z, &picked_inst_proj_pos_.x, &picked_inst_proj_pos_.y);
        }
    }

    void python_character_manager::render_shadow_main_instance() {
        instance_base *inst_main = main_instance_ptr();
        if (inst_main)
            inst_main->render_to_shadow_map();
    }

    struct character_manager_character_instance_render_to_shadow_map_fn {
        inline void operator()(const std::pair<uint32_t, instance_base *> &cr_pair) {
            cr_pair.second->render_to_shadow_map();
        }
    };

    void python_character_manager::render_shadow_all_instances() {
        std::for_each(alive_inst_map_.begin(), alive_inst_map_.end(), character_manager_character_instance_render_to_shadow_map_fn());
    }

    struct character_manager_character_instance_render_collision_fn {
        inline void operator()(const std::pair<uint32_t, instance_base *> &cr_pair) {
            cr_pair.second->render_collision();
        }
    };

    void python_character_manager::render_collision() {
        std::for_each(alive_inst_map_.begin(), alive_inst_map_.end(), character_manager_character_instance_render_collision_fn());
    }

    auto python_character_manager::create_instance(const instance_base::create_data &create_data) -> instance_base * {
        instance_base *character_instance = register_instance(create_data.vid);
        if (!character_instance) {
            metin2::log::error("Character (vid {}) already exists", create_data.vid);
            return nullptr;
        }

        if (!character_instance->create(create_data)) {
            metin2::log::error("Cannot create character (vid {}, race {})", create_data.vid, create_data.race);
            delete_instance(create_data.vid);
            return nullptr;
        }

        if (create_data.is_main)
            select_instance(create_data.vid);

        return (character_instance);
    }

    auto python_character_manager::register_instance(uint32_t virtual_id) -> instance_base * {
        auto itor = alive_inst_map_.find(virtual_id);

        if (alive_inst_map_.end() != itor) {
            return nullptr;
        }

        instance_base *character_instance = instance_base::new_value();
        alive_inst_map_.insert(character_instance_map::value_type(virtual_id, character_instance));

        return (character_instance);
    }

    void python_character_manager::delete_instance(uint32_t del_vid) {
        auto itor = alive_inst_map_.find(del_vid);

        if (alive_inst_map_.end() == itor) {
            metin2::log::debug("Cannot delete character (vid {}): not found", del_vid);
            return;
        }

        instance_base *inst_del = itor->second;

        if (inst_del == inst_bind_)
            inst_bind_ = nullptr;

        if (inst_del == inst_main_)
            inst_main_ = nullptr;

        if (inst_del == inst_pick_)
            inst_pick_ = nullptr;

        instance_base::delete_value(inst_del);

        alive_inst_map_.erase(itor);
    }

    void python_character_manager::delete_blend_out_instance(instance_base *inst_del) {
        inst_del->delete_blend_out();
        dead_inst_list_.push_back(inst_del);

        abstract_player &player = get_abstract_player();
        player.notify_character_dead(inst_del->virtual_id());
    }

    void python_character_manager::delete_instance_by_fade(uint32_t vid) {
        auto f = alive_inst_map_.find(vid);
        if (alive_inst_map_.end() == f) {
            return;
        }
        delete_blend_out_instance(f->second);
        alive_inst_map_.erase(f);
    }

    void python_character_manager::select_instance(uint32_t virtual_id) {
        auto itor = alive_inst_map_.find(virtual_id);

        if (alive_inst_map_.end() == itor) {
            metin2::log::debug("Cannot select character (vid {}): not found", virtual_id);
            return;
        }

        inst_bind_ = itor->second;
    }

    auto python_character_manager::instance_ptr(uint32_t virtual_id) -> instance_base * {
        auto itor = alive_inst_map_.find(virtual_id);

        if (alive_inst_map_.end() == itor)
            return nullptr;

        return itor->second;
    }

    auto python_character_manager::selected_instance_ptr() -> instance_base * {
        return inst_bind_;
    }

    void python_character_manager::update_sort_picked_actor_list() {
        update_picked_actor_list();
        sort_picked_actor_list();
    }

    void python_character_manager::update_picked_actor_list() {
        vct_inst_picked_.clear();

        character_instance_map::iterator i;
        for (i = alive_inst_map_.begin(); i != alive_inst_map_.end(); ++i) {
            instance_base *inst_each = i->second;
            if (inst_each->can_pick_instance()) {
                if (inst_each->is_dead()) {
                    if (inst_each->intersect_bounding_box())
                        vct_inst_picked_.push_back(inst_each);
                } else {
                    if (inst_each->intersect_defending_sphere())
                        vct_inst_picked_.push_back(inst_each);
                }
            }
        }
    }

    struct instance_base_less_camera_distance {
        pixel_position p_pos_eye;

        auto operator()(instance_base *inst_left, instance_base *inst_right) -> bool {
            int left_dead_point = inst_left->is_dead();
            int right_dead_point = inst_right->is_dead();

            if (left_dead_point < right_dead_point)
                return true;

            if (inst_left->calculate_distance_sq3d(p_pos_eye) < inst_right->calculate_distance_sq3d(p_pos_eye))
                return true;

            return false;
        }
    };

    void python_character_manager::sort_picked_actor_list() {
        camera *camera = get_camera_manager().current_camera();
        const math::vector3 &eye_pos = camera->eye();

        instance_base_less_camera_distance less;
        less.p_pos_eye = pixel_position(+eye_pos.x, -eye_pos.y, +eye_pos.z);

        std::sort(vct_inst_picked_.begin(), vct_inst_picked_.end(), less);
    }

    void python_character_manager::new_pick() {
        update_sort_picked_actor_list();

        instance_base *inst_main = main_instance_ptr();

        {
            std::vector<instance_base *>::iterator f;
            for (f = vct_inst_picked_.begin(); f != vct_inst_picked_.end(); ++f) {
                instance_base *inst_each = *f;
                if (inst_each != inst_main && inst_each->intersect_bounding_box()) {
                    if (inst_pick_)
                        if (inst_pick_ != inst_each)
                            inst_pick_->on_unselected();

                    if (inst_each->can_pick_instance()) {
                        inst_pick_ = inst_each;
                        inst_pick_->on_selected();
                        return;
                    }
                }
            }
        }

        // No exact hit: take the first pickable instance in order.
        {
            std::vector<instance_base *>::iterator f;
            for (f = vct_inst_picked_.begin(); f != vct_inst_picked_.end(); ++f) {
                instance_base *inst_each = *f;
                if (inst_each != inst_main) {
                    if (inst_pick_)
                        if (inst_pick_ != inst_each)
                            inst_pick_->on_unselected();

                    if (inst_each->can_pick_instance()) {
                        inst_pick_ = inst_each;
                        inst_pick_->on_selected();
                        return;
                    }
                }
            }
        }

        if (inst_main)
            if (inst_main->can_pick_instance())
                if (vct_inst_picked_.end() != std::find(vct_inst_picked_.begin(), vct_inst_picked_.end(), inst_main)) {
                    if (inst_pick_)
                        if (inst_pick_ != inst_main)
                            inst_pick_->on_unselected();

                    inst_pick_ = inst_main;
                    inst_pick_->on_selected();
                    return;
                }

        if (inst_pick_) {
            inst_pick_->on_unselected();
            inst_pick_ = nullptr;
        }
    }

    void python_character_manager::refresh_all_pc_text_tail() {
        python_character_manager::character_iterator itor = character_instance_begin();
        python_character_manager::character_iterator itor_end = character_instance_end();
        for (; itor != itor_end; ++itor) {
            instance_base *instance = *itor;
            if (!instance->is_pc())
                continue;

            instance->refresh_text_tail();
        }
    }

    void python_character_manager::refresh_all_guild_mark() {
        python_character_manager::character_iterator itor = character_instance_begin();
        python_character_manager::character_iterator itor_end = character_instance_end();
        for (; itor != itor_end; ++itor) {
            instance_base *instance = *itor;
            if (!instance->is_pc())
                continue;

            instance->change_guild(instance->guild_id());
            instance->refresh_text_tail();
        }
    }

    void python_character_manager::delete_all_instances() {
        destroy_alive_instance_map();
        destroy_dead_instance_list();
    }

    void python_character_manager::destroy_alive_instance_map() {
        for (auto i = alive_inst_map_.begin(); i != alive_inst_map_.end(); ++i)
            instance_base::delete_value(i->second);

        alive_inst_map_.clear();
    }

    void python_character_manager::destroy_dead_instance_list() {
        std::for_each(dead_inst_list_.begin(), dead_inst_list_.end(), instance_base::delete_value);
        dead_inst_list_.clear();
    }

    void python_character_manager::destroy() {
        delete_all_instances();

        instance_base::destroy_system();

        initialize_impl();
    }

    void python_character_manager::initialize_impl() {
        memset(point_effect_, 0, sizeof(point_effect_));
        inst_main_ = nullptr;
        inst_bind_ = nullptr;
        inst_pick_ = nullptr;
        picked_inst_proj_pos_ = math::vector2(0.0f, 0.0f);
    }

    python_character_manager::python_character_manager() {
        initialize_impl();
    }

    python_character_manager::~python_character_manager() {
        destroy();
    }
}

namespace metin2::inline client::inline app {

    auto get_abstract_character_manager() -> abstract_character_manager & {
        return get_python_character_manager();
    }
}
