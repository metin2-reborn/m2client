
#include <metin2/core/parse.h>
#include <metin2/core/random.h>
#include <metin2/core/text_file_loader.h>
#include <metin2/core/timer.h>
#include <metin2/core/utf8.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>
#include <metin2/trees/speed_tree_forest_direct_x8.h>

#include <metin2/core/crc32.h>
#include <metin2/core/file_name.h>
#include <metin2/gpu/types.h>
#include "area.h"
#include "property.h"
#include "property_manager.h"

namespace metin2::inline world::inline maps {

    dynamic_pool<area::object_instance> area::object_instance_pool_;
    dynamic_pool<attribute_instance> area::attribute_instance_pool_;
    dynamic_pool<area::ambience_instance> area::ambience_instance_pool_;
    dynamic_pool<dungeon_block> area::dungeon_block_instance_pool_;
    dynamic_pool<area> area::pool_value;

    void area::object_data::initialize_rotation() {
        yaw = pitch = roll = 0.0f;
    }

    auto area::new_value() -> area * {
        return pool_value.alloc();
    }

    void area::delete_value(area *area) {
        area->clear();
        pool_value.free(area);
    }

    void area::destroy_system() {
        pool_value.destroy();

        object_instance_pool_.destroy();
        attribute_instance_pool_.destroy();
        ambience_instance_pool_.destroy();
        dungeon_block_instance_pool_.destroy();
    }

    void area::update_ani_thing_list() {
        {
            graphic_thing_instance *thing_inst;

            auto i = thing_clone_instace_vector_.begin();
            while (i != thing_clone_instace_vector_.end()) {
                thing_inst = *i++;
                if (thing_inst->is_show()) {
                    thing_inst->update_lod_level();
                }
            }
        }

        {
            graphic_thing_instance *thing_inst;

            auto i = ani_thing_clone_instance_vector_.begin();
            while (i != ani_thing_clone_instance_vector_.end()) {
                thing_inst = *i++;
                thing_inst->update();
            }
        }
    }

    void area::update_effect_list() {

        effect_manager &eft_mgr = get_effect_manager();

        effect_instance_iterator i;
        for (i = effect_instance_map_value_.begin(); i != effect_instance_map_value_.end();) {
            effect_instance *effect_instance = i->second;

            effect_instance->update();

            if (!effect_instance->is_alive()) {
                i = effect_instance_map_value_.erase(i);
                eft_mgr.destroy_unsafe_effect_instance(effect_instance);
            } else
                ++i;
        }
    }

    void area::update() {
        update_ani_thing_list();
    }

    void area::update_around_ambience(float x, float y, float z) {
        ambience_instance_vector::iterator i;
        for (i = ambience_clone_instance_vector_.begin(); i != ambience_clone_instance_vector_.end(); ++i) {
            ambience_instance *instance = *i;
            instance->update_impl(x, y, z);
        }
    }

    struct area_less_effect_instance_ptr_render_order {
        auto operator()(effect_instance *left, effect_instance *right) -> bool {
            return left->less_render_order(right);
        }
    };

    struct area_effect_instance_render {
        inline void operator()(effect_instance *eft_inst) {
            eft_inst->render();
        }
    };

    void area::render_effect() {
        update_effect_list();

        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture(1, nullptr);

        bool is_disable_sort_rendering = false;

        if (is_disable_sort_rendering) {
            effect_instance_iterator i;
            for (i = effect_instance_map_value_.begin(); i != effect_instance_map_value_.end();) {
                effect_instance *effect_instance = i->second;
                effect_instance->render();
                ++i;
            }
        } else {
            static std::vector<effect_instance *> vct_eft_inst_sort;
            vct_eft_inst_sort.clear();

            effect_instance_map &map_eft_inst_src = effect_instance_map_value_;
            effect_instance_map::iterator i;
            for (i = map_eft_inst_src.begin(); i != map_eft_inst_src.end(); ++i)
                vct_eft_inst_sort.push_back(i->second);

            std::sort(vct_eft_inst_sort.begin(), vct_eft_inst_sort.end(), area_less_effect_instance_ptr_render_order());
            std::for_each(vct_eft_inst_sort.begin(), vct_eft_inst_sort.end(), area_effect_instance_render());
        }
    }

    auto area::debug_get_rendered_crc_num() -> uint32_t { return rendered_thing_instance_crc_with_number_vector_.size(); }

    auto area::debug_get_rendered_crc_with_num_vector() -> area::crc_with_number_vector & { return rendered_thing_instance_crc_with_number_vector_; }

    auto area::debug_get_rendered_grapphic_thing_instance_num() -> uint32_t { return rendered_grapphic_thing_instance_vector_.size(); }

    void area::collect_rendering_object(std::vector<graphic_thing_instance *> &vct_opaque_thing_inst) {
        thing_instance_vector::iterator i;
        for (i = thing_clone_instace_vector_.begin(); i != thing_clone_instace_vector_.end(); ++i) {
            graphic_thing_instance *thing_inst = *i;
            if (thing_inst->is_show()) {
                if (!thing_inst->have_blend_thing())
                    vct_opaque_thing_inst.push_back(*i);
            }
        }
    }

    void area::collect_blend_rendering_object(std::vector<graphic_thing_instance *> &vct_blend_thing_inst) {
        thing_instance_vector::iterator i;
        for (i = thing_clone_instace_vector_.begin(); i != thing_clone_instace_vector_.end(); ++i) {
            graphic_thing_instance *thing_inst = *i;
            if (thing_inst->is_show()) {
                if (thing_inst->have_blend_thing())
                    vct_blend_thing_inst.push_back(*i);
            }
        }
    }

    void area::render() {
        {
            graphic_thing_instance *thing_inst;

            auto i = ani_thing_clone_instance_vector_.begin();
            while (i != ani_thing_clone_instance_vector_.end()) {
                thing_inst = *i++;
                thing_inst->deform();
            }
        }

        graphic_thing_instance *thing_inst;

        auto i = thing_clone_instace_vector_.begin();

        rendered_thing_instance_crc_with_number_vector_.clear();
        rendered_grapphic_thing_instance_vector_.clear();
        graphic_thing_instance_crc_map::iterator graphic_thing_instance_crc_map_iterator;

        while (i != thing_clone_instace_vector_.end()) {
            thing_inst = *i++;
            if (thing_inst->render()) {
                graphic_thing_instance_crc_map_iterator = graphic_thing_instance_crc_map_value_.find(thing_inst);
                uint32_t crc = (*graphic_thing_instance_crc_map_iterator).second;

                rendered_grapphic_thing_instance_vector_.push_back(thing_inst);

                auto crc_with_number_vector_iterator = std::find_if(rendered_thing_instance_crc_with_number_vector_.begin(), rendered_thing_instance_crc_with_number_vector_.end(), find_if_crc_fn(crc));

                if (rendered_thing_instance_crc_with_number_vector_.end() == crc_with_number_vector_iterator) {
                    crc_with_number a_crc_with_number;
                    a_crc_with_number.crc = crc;
                    a_crc_with_number.number = 1;
                    rendered_thing_instance_crc_with_number_vector_.push_back(a_crc_with_number);
                } else {
                    crc_with_number &r_crc_with_number = *crc_with_number_vector_iterator;
                    r_crc_with_number.number += 1;
                }
            }
        }
        std::sort(rendered_thing_instance_crc_with_number_vector_.begin(), rendered_thing_instance_crc_with_number_vector_.end(), rc_num_comp());
    }

    void area::render_collision() {
        uint32_t i;

        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture(1, nullptr);

        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, 0xff000000);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        for (i = 0; i < object_instance_count(); i++) {
            const object_instance *po;
            if (object_instance_pointer(i, &po)) {
                if (po->tree && po->tree->is_show()) {
                    uint32_t j;
                    for (j = 0; j < po->tree->collision_instance_count(); j++) {
                        po->tree->collision_instance_data(j)->render();
                    }
                }
                if (po->thing_instance && po->thing_instance->is_show()) {
                    uint32_t j;
                    for (j = 0; j < po->thing_instance->collision_instance_count(); j++) {
                        po->thing_instance->collision_instance_data(j)->render();
                    }
                }
                if (po->dungeon_block_value && po->dungeon_block_value->is_show()) {
                    uint32_t j;
                    for (j = 0; j < po->dungeon_block_value->collision_instance_count(); j++) {
                        po->dungeon_block_value->collision_instance_data(j)->render();
                    }
                }
            }
        }

        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, true);
    }

    void area::render_dungeon() {
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);

        auto itor = dungeon_block_clone_instance_vector_.begin();
        for (; itor != dungeon_block_clone_instance_vector_.end(); ++itor) {
            (*itor)->render();
        }

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
    }

    void area::refresh() {
        tree_clone_instace_vector_.clear();
        thing_clone_instace_vector_.clear();
        dungeon_block_clone_instance_vector_.clear();
        ani_thing_clone_instance_vector_.clear();
        shadow_thing_clone_instace_vector_.clear();
        ambience_clone_instance_vector_.clear();

        object_instance_vector::iterator it;

        for (it = object_instance_vector_value_.begin(); it != object_instance_vector_value_.end(); ++it) {
            object_instance *object_instance_value = *it;

            if (property_types::PROPERTY_TYPE_TREE == object_instance_value->type_value) {
                if (object_instance_value->tree) {
                    tree_clone_instace_vector_.push_back(object_instance_value->tree);

                    object_instance_value->tree->update_bounding_sphere();
                    object_instance_value->tree->update_collision_data();
                }
            } else if (property_types::PROPERTY_TYPE_BUILDING == object_instance_value->type_value) {
                object_instance_value->thing_instance->update();
                object_instance_value->thing_instance->transform();
                object_instance_value->thing_instance->show();
                object_instance_value->thing_instance->deform_all();
                thing_clone_instace_vector_.push_back(object_instance_value->thing_instance);

                object_instance_value->thing_instance->build_bounding_sphere();
                object_instance_value->thing_instance->update_bounding_sphere();

                if (object_instance_value->thing_instance->is_motion_thing()) {
                    ani_thing_clone_instance_vector_.push_back(object_instance_value->thing_instance);
                    object_instance_value->thing_instance->set_motion(0);
                }

                if (object_instance_value->is_shadow_flag) {
                    shadow_thing_clone_instace_vector_.push_back(object_instance_value->thing_instance);
                }

                if (object_instance_value->attribute_instance_value) {
                    object_instance_value->thing_instance->update_collision_data(&object_instance_value->attribute_instance_value->object_pointer()->get_collision_data_vector());
                    object_instance_value->attribute_instance_value->refresh_object(object_instance_value->thing_instance->get_transform());
                    object_instance_value->thing_instance->update_height_instance(object_instance_value->attribute_instance_value);
                }
            } else if (property_types::PROPERTY_TYPE_EFFECT == object_instance_value->type_value) {
            } else if (property_types::PROPERTY_TYPE_AMBIENCE == object_instance_value->type_value) {
                ambience_clone_instance_vector_.push_back(object_instance_value->ambience_instance_value);
            } else if (property_types::PROPERTY_TYPE_DUNGEON_BLOCK == object_instance_value->type_value) {
                object_instance_value->dungeon_block_value->update();
                object_instance_value->dungeon_block_value->deform();
                object_instance_value->dungeon_block_value->update_bounding_sphere();
                dungeon_block_clone_instance_vector_.push_back(object_instance_value->dungeon_block_value);

                if (object_instance_value->attribute_instance_value) {
                    object_instance_value->dungeon_block_value->update_collision_data(&object_instance_value->attribute_instance_value->object_pointer()->get_collision_data_vector());
                    object_instance_value->attribute_instance_value->refresh_object(object_instance_value->dungeon_block_value->get_transform());
                    object_instance_value->dungeon_block_value->update_height_instance(object_instance_value->attribute_instance_value);
                }
            }
        }
    }

    void area::load_build_object_instances() {
        object_instance_vector_value_.clear();
        object_instance_vector_value_.resize(object_data_count());

        graphic_thing_instance_crc_map_value_.clear();

        std::sort(object_data_vector_value_.begin(), object_data_vector_value_.end(), object_data_comp());

        uint32_t i = 0;
        object_instance_vector::iterator it;
        for (it = object_instance_vector_value_.begin(); it != object_instance_vector_value_.end(); ++it, ++i) {
            *it = object_instance_pool_.alloc();
            (*it)->clear();

            const object_data *object_data_value;

            if (!object_data_pointer(i, &object_data_value))
                continue;

            set_object_instance(*it, object_data_value);

            if ((*it)->type_value == property_types::PROPERTY_TYPE_BUILDING)
                graphic_thing_instance_crc_map_value_.insert(graphic_thing_instance_crc_map::value_type((*it)->thing_instance, object_data_value->crc));
        }

        refresh();
    }

    void area::set_object_instance(object_instance *object_instance_value, const object_data *data) {
        property *property;
        if (!get_property_manager().get(data->crc, &property))
            return;

        switch (property->type()) {
        case property_types::PROPERTY_TYPE_TREE:
            set_object_instance_set_tree(object_instance_value, data, property);
            break;

        case property_types::PROPERTY_TYPE_BUILDING:
            set_object_instance_set_building(object_instance_value, data, property);
            break;

        case property_types::PROPERTY_TYPE_EFFECT:
            set_object_instance_set_effect(object_instance_value, data, property);
            break;

        case property_types::PROPERTY_TYPE_AMBIENCE:
            set_object_instance_set_ambience(object_instance_value, data, property);
            break;

        case property_types::PROPERTY_TYPE_DUNGEON_BLOCK:
            set_object_instance_set_dungeon_block(object_instance_value, data, property);
            break;

        case property_types::PROPERTY_TYPE_NONE:
        case property_types::PROPERTY_TYPE_MAX_NUM:
            break;
        }
    }

    void area::set_object_instance_set_effect(object_instance *object_instance_value, const object_data *c_p_data, const property *property) {
        const property_types::property_effect &data = property->effect();
        object_instance_value->type_value = property_types::PROPERTY_TYPE_EFFECT;
        object_instance_value->effect_id = get_case_crc32(data.file_name.c_str(), data.file_name.size());
        effect_manager &rem = get_effect_manager();
        effect_data *p_data;
        if (!rem.get_effect_data(object_instance_value->effect_id, &p_data)) {
            if (!rem.register_effect(data.file_name.c_str())) {
                object_instance_value->effect_id = 0xffffffff;
                metin2::log::warn("Cannot register area effect {}", data.file_name.c_str());
                return;
            }
        }

        effect_instance *effect_instance;
        rem.create_unsafe_effect_instance(object_instance_value->effect_id, &effect_instance);

        math::matrix mat;
        mat = math::matrix_rotation_yaw_pitch_roll(math::to_radian(c_p_data->yaw), math::to_radian(c_p_data->pitch), math::to_radian(c_p_data->roll));

        mat._41 = c_p_data->position_value.x;
        mat._42 = c_p_data->position_value.y;
        mat._43 = c_p_data->position_value.z + c_p_data->height_bias;

        effect_instance->set_global_matrix(mat);

        object_instance_value->effect_instance_index = effect_instance_map_value_.size();
        effect_instance_map_value_.insert(effect_instance_map::value_type(object_instance_value->effect_instance_index, effect_instance));
    }

    void area::set_object_instance_set_tree(object_instance *object_instance_value, const object_data *data, const property *property) {
        object_instance_value->set_tree(
            data->position_value.x,
            data->position_value.y,
            data->position_value.z + data->height_bias,
            data->crc,
            property->tree().file_name.c_str());
    }

    void area::object_instance::set_tree(float x, float y, float z, uint32_t tree_crc, const char *tree_name) {
        speed_tree_forest_direct_x8 &forest = get_speed_tree_forest_direct_x8();
        tree = forest.create_instance(x, y, z, tree_crc, tree_name);
        type_value = property_types::PROPERTY_TYPE_TREE;
    }

    void area::set_object_instance_set_building(object_instance *object_instance_value, const object_data *c_p_data, const property *property) {
        const property_types::property_building &data = property->building();
        resource_manager &res_mgr = get_resource_manager();

        auto *thing = (graphic_thing *)res_mgr.resource_pointer(data.file_name.c_str());
        thing->add_reference();

        if (thing->is_empty()) {
            return;
        }

        int model_count = thing->model_count();
        int motion_count = thing->motion_count();

        object_instance_value->type_value = property_types::PROPERTY_TYPE_BUILDING;
        object_instance_value->thing_instance = graphic_thing_instance::new_value();
        object_instance_value->thing_instance->initialize();
        object_instance_value->thing_instance->reserve_model_thing(model_count);
        object_instance_value->thing_instance->reserve_model_instance(model_count);
        object_instance_value->thing_instance->register_model_thing(0, thing);
        for (int j = 0; j < PORTAL_ID_MAX_NUM; ++j)
            if (0 != c_p_data->portal_id[j])
                object_instance_value->thing_instance->set_portal(j, c_p_data->portal_id[j]);

        {
            std::string src_model_file_name = data.file_name;
            std::string lod_model_file_name;

            char lod_model_file_name_end[256];
            for (uint32_t lod_index = 1; lod_index <= 3; ++lod_index) {
                snprintf(lod_model_file_name_end, sizeof(lod_model_file_name_end), "_lod_%.2d.gr2", lod_index);
                lod_model_file_name = file_name_helper::no_extension(src_model_file_name) + lod_model_file_name_end;
                if (!res_mgr.is_file_exist(lod_model_file_name.c_str()))
                    break;

                auto *lod_model_thing = (graphic_thing *)res_mgr.resource_pointer(lod_model_file_name.c_str());
                if (!lod_model_thing)
                    break;

                object_instance_value->thing_instance->register_lod_thing(0, lod_model_thing);
            }
        }

        for (int i = 0; i < model_count; ++i)
            object_instance_value->thing_instance->set_model_instance(i, 0, i);

        if (motion_count) {
            object_instance_value->thing_instance->register_motion_thing(0, thing);
        }

        object_instance_value->thing_instance->set_position(c_p_data->position_value.x, c_p_data->position_value.y, c_p_data->position_value.z + c_p_data->height_bias);
        object_instance_value->thing_instance->set_rotation(
            c_p_data->yaw,
            c_p_data->pitch,
            c_p_data->roll);
        object_instance_value->is_shadow_flag = data.is_shadow_flag;
        object_instance_value->thing_instance->register_bounding_sphere();
        load_attribute(object_instance_value, data.attribute_data_file_name.c_str());
        thing->release();
    }

    void area::set_object_instance_set_ambience(object_instance *object_instance_value, const object_data *data, const property *property) {
        object_instance_value->ambience_instance_value = ambience_instance_pool_.alloc();
        object_instance_value->ambience_instance_value->ambience_data = property->ambience();

        object_instance_value->type_value = property_types::PROPERTY_TYPE_AMBIENCE;

        ambience_instance *ambience_instance_value = object_instance_value->ambience_instance_value;
        ambience_instance_value->fx = data->position_value.x;
        ambience_instance_value->fy = data->position_value.y;
        ambience_instance_value->fz = data->position_value.z + data->height_bias;
        ambience_instance_value->range = data->range;
        ambience_instance_value->max_volume_area_percentage = data->max_volume_area_percentage;

        switch (ambience_instance_value->ambience_data.play_type) {
        case property_types::AMBIENCE_PLAY_TYPE_ONCE:
            ambience_instance_value->update_fn_value = &ambience_instance::update_once_sound;
            break;
        case property_types::AMBIENCE_PLAY_TYPE_STEP:
            ambience_instance_value->update_fn_value = &ambience_instance::update_step_sound;
            break;
        case property_types::AMBIENCE_PLAY_TYPE_LOOP:
            ambience_instance_value->update_fn_value = &ambience_instance::update_loop_sound;
            break;
        case property_types::AMBIENCE_PLAY_TYPE_MAX_NUM:
            break;
        }
    }

    void area::set_object_instance_set_dungeon_block(object_instance *object_instance_value, const object_data *c_p_data, const property *property) {
        const property_types::property_dungeon_block &data = property->dungeon_block();
        object_instance_value->type_value = property_types::PROPERTY_TYPE_DUNGEON_BLOCK;
        object_instance_value->dungeon_block_value = dungeon_block_instance_pool_.alloc();
        object_instance_value->dungeon_block_value->load(data.file_name.c_str());
        object_instance_value->dungeon_block_value->set_position(c_p_data->position_value.x, c_p_data->position_value.y, c_p_data->position_value.z + c_p_data->height_bias);
        object_instance_value->dungeon_block_value->set_rotation(
            c_p_data->yaw,
            c_p_data->pitch,
            c_p_data->roll);
        object_instance_value->dungeon_block_value->update();
        object_instance_value->dungeon_block_value->build_bounding_sphere();
        object_instance_value->dungeon_block_value->register_bounding_sphere();
        for (int j = 0; j < PORTAL_ID_MAX_NUM; ++j)
            if (0 != c_p_data->portal_id[j])
                object_instance_value->dungeon_block_value->set_portal(j, c_p_data->portal_id[j]);
        load_attribute(object_instance_value, data.attribute_data_file_name.c_str());
    }

    void area::load_attribute(object_instance *object_instance_value, const char *attribute_file_name) {
        // Objects without an attribute file get a dummy OBB collision built from their bounding box.
        const bool file_exist = get_resource_manager().is_file_exist(attribute_file_name);

        auto *attribute_data_value = (attribute_data *)get_resource_manager().resource_pointer(attribute_file_name);

        attribute_instance *attr_instance = attribute_instance_pool_.alloc();
        attr_instance->clear();
        attr_instance->set_object_pointer(attribute_data_value);

        if (false == file_exist) {
            std::string attr_file_name(attribute_file_name);
            std::transform(attr_file_name.begin(), attr_file_name.end(), attr_file_name.begin(), [](unsigned char c) -> int { return std::tolower(c); });
            const bool is_dungeon_object = (std::string::npos != attr_file_name.find("/dungeon/")) || (std::string::npos != attr_file_name.find("\\dungeon\\"));

            // Dungeon objects get no dummy collision; it caused problems in dungeons several times.
            if (attribute_data_value->is_empty() && false == is_dungeon_object) {
                if (nullptr != object_instance_value && nullptr != object_instance_value->thing_instance) {
                    graphic_thing_instance *object = object_instance_value->thing_instance;

                    math::vector3 min_value, max_value;

                    object->bounding_aabb(min_value, max_value);

                    static_collision_data collision;
                    collision.type_value = COLLISION_TYPE_OBB;
                    collision.quat_rotation = math::quaternion_rotation_yaw_pitch_roll(object->yaw(), object->get_pitch(), object->get_roll());
                    utf8::copy_field(collision.name_value, "DummyCollisionOBB");
                    collision.position_value = (min_value + max_value) * 0.5f;

                    math::vector3 v_delta = (max_value - min_value);
                    collision.dimensions[0] = v_delta.x * 0.5f;
                    collision.dimensions[1] = v_delta.y * 0.5f;
                    collision.dimensions[2] = v_delta.z * 0.5f;

                    attribute_data_value->add_collision_data(collision);
                }
            }
        }

        if (!attribute_data_value->is_empty()) {
            object_instance_value->attribute_instance_value = attr_instance;
        } else {
            attr_instance->clear();
            attribute_instance_pool_.free(attr_instance);
        }
    }

    auto area::load(const char *path_name) -> bool {
        clear();

        std::string object_data_file_name = path_name + std::string("AreaData.txt");
        std::string ambience_data_file_name = path_name + std::string("AreaAmbienceData.txt");

        load_load_object(object_data_file_name.c_str());
        load_load_ambience(ambience_data_file_name.c_str());
        load_build_object_instances();

        return true;
    }

    auto area::load_load_object(const char *file_name) -> bool {
        token_vector_map token_vector_map;

        if (!load_multiple_text_data(file_name, token_vector_map)) {
            metin2::log::error("Cannot load area objects {}", file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("areadatafile")) {
            metin2::log::error("Area objects {}: no areadatafile key", file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("objectcount")) {
            metin2::log::error("Area objects {}: no objectcount key", file_name);
            return false;
        }

        const std::string &c_rstr_count = token_vector_map["objectcount"][0];

        uint32_t dw_count = metin2::to_number<int>(c_rstr_count.c_str());

        char object_name[32 + 1];

        for (uint32_t i = 0; i < dw_count; ++i) {
            snprintf(object_name, sizeof(object_name), "object%03d", i);

            if (token_vector_map.end() == token_vector_map.find(object_name))
                continue;

            const token_vector &vector = token_vector_map[object_name];

            const std::string &rstrx_position = vector[0].c_str();
            const std::string &rstry_position = vector[1].c_str();
            const std::string &rstrz_position = vector[2].c_str();
            const std::string &crc = vector[3].c_str();

            object_data object_data_value;
            memset(static_cast<void *>(&object_data_value), 0, sizeof(object_data_value));
            object_data_value.position_value.x = metin2::to_number<double>(rstrx_position.c_str());
            object_data_value.position_value.y = metin2::to_number<double>(rstry_position.c_str());
            object_data_value.position_value.z = metin2::to_number<double>(rstrz_position.c_str());
            object_data_value.crc = metin2::to_number<int>(crc.c_str());

            object_data_value.initialize_rotation();
            if (vector.size() > 4) {
                std::string::size_type s = vector[4].find('#');
                if (s != vector[4].npos) {
                    object_data_value.yaw = metin2::to_number<int>(vector[4].substr(0, s - 1).c_str());
                    int p = s + 1;
                    s = vector[4].find('#', p);
                    object_data_value.pitch = metin2::to_number<int>(vector[4].substr(p, s - 1 - p + 1).c_str());
                    object_data_value.roll = metin2::to_number<int>(vector[4].substr(s + 1).c_str());
                } else {
                    object_data_value.yaw = 0.0f;
                    object_data_value.pitch = 0.0f;
                    object_data_value.roll = metin2::to_number<int>(vector[4].c_str());
                }
            }

            object_data_value.height_bias = 0.0f;
            if (vector.size() > 5) {
                object_data_value.height_bias = metin2::to_number<double>(vector[5].c_str());
            }

            if (vector.size() > 6) {
                for (size_t portal_idx = 0; portal_idx < std::min<size_t>(vector.size() - 6, PORTAL_ID_MAX_NUM); ++portal_idx) {
                    object_data_value.portal_id[portal_idx] = metin2::to_number<int>(vector[6 + portal_idx].c_str());
                }
            }

            property *property;
            if (!get_property_manager().get(object_data_value.crc, &property)) {
                metin2::log::warn("Area object property {} cannot be loaded", object_data_value.crc);
                continue;
            }

            object_data_vector_value_.push_back(object_data_value);
        }

        return true;
    }

    auto area::load_load_ambience(const char *file_name) -> bool {
        token_vector_map token_vector_map;

        if (!load_multiple_text_data(file_name, token_vector_map)) {
            metin2::log::error("Cannot load area ambience {}", file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("areaambiencedatafile")) {
            metin2::log::error("Area ambience {}: no areaambiencedatafile key", file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("objectcount")) {
            metin2::log::error("Area ambience {}: no objectcount key", file_name);
            return false;
        }

        const std::string &c_rstr_count = token_vector_map["objectcount"][0];

        uint32_t dw_count = metin2::to_number<int>(c_rstr_count.c_str());

        char object_name[32 + 1];

        for (uint32_t i = 0; i < dw_count; ++i) {
            snprintf(object_name, sizeof(object_name), "object%03d", i);

            if (token_vector_map.end() == token_vector_map.find(object_name))
                continue;

            const token_vector &vector = token_vector_map[object_name];

            const std::string &rstrx_position = vector[0].c_str();
            const std::string &rstry_position = vector[1].c_str();
            const std::string &rstrz_position = vector[2].c_str();
            const std::string &crc = vector[3].c_str();
            const std::string &range = vector[4].c_str();

            object_data object_data_value;
            memset(static_cast<void *>(&object_data_value), 0, sizeof(object_data_value));
            object_data_value.position_value.x = metin2::to_number<double>(rstrx_position.c_str());
            object_data_value.position_value.y = metin2::to_number<double>(rstry_position.c_str());
            object_data_value.position_value.z = metin2::to_number<double>(rstrz_position.c_str());
            object_data_value.crc = metin2::to_number<int>(crc.c_str());
            object_data_value.range = metin2::to_number<int>(range.c_str());

            object_data_value.initialize_rotation();
            object_data_value.height_bias = 0.0f;
            object_data_value.max_volume_area_percentage = 0.0f;

            if (vector.size() >= 6) {
                const std::string &percentage = vector[5].c_str();
                object_data_value.max_volume_area_percentage = metin2::to_number<double>(percentage.c_str());
            }

            property *property;
            if (!get_property_manager().get(object_data_value.crc, &property)) {
                metin2::log::warn("Area ambience property {} cannot be loaded", object_data_value.crc);
                continue;
            }

            object_data_vector_value_.push_back(object_data_value);
        }

        return true;
    }

    auto area::check_object_index(uint32_t index) const -> bool {
        if (index >= object_data_vector_value_.size())
            return false;

        return true;
    }

    auto area::object_data_count() -> uint32_t {
        return object_data_vector_value_.size();
    }

    auto area::object_data_pointer(uint32_t index, const object_data **object_data_value) const -> bool {
        if (!check_object_index(index)) {
            assert(!"Setting Object Index is corrupted!");
            return false;
        }

        *object_data_value = &object_data_vector_value_[index];
        return true;
    }

    auto area::object_instance_count() const -> uint32_t {
        return object_instance_vector_value_.size();
    }

    auto area::object_instance_pointer(const uint32_t &index, const object_instance **object_instance_value) const -> bool {
        if (index >= object_instance_vector_value_.size())
            return false;

        *object_instance_value = object_instance_vector_value_[index];
        return true;
    }

    void area::enable_portal(bool flag) {
        if (portal_enable_ == flag)
            return;

        portal_enable_ = flag;
    }

    void area::clear_portal() {
        set_showing_portal_id_.clear();
    }

    void area::add_showing_portal_id(int num) {
        set_showing_portal_id_.insert(num);
    }

    void area::refresh_portal() {
        std::set<object_instance *> set_showing_object_instance;
        set_showing_object_instance.clear();
        for (uint32_t i = 0; i < object_data_vector_value_.size(); ++i) {
            object_data &data = object_data_vector_value_[i];
            object_instance *instance = object_instance_vector_value_[i];

            for (int j = 0; j < PORTAL_ID_MAX_NUM; ++j) {
                uint8_t portal_id = data.portal_id[j];
                if (0 == portal_id)
                    break;

                if (set_showing_portal_id_.end() == set_showing_portal_id_.find(portal_id))
                    continue;

                set_showing_object_instance.insert(instance);
                break;
            }
        }

        tree_clone_instace_vector_.clear();
        thing_clone_instace_vector_.clear();
        dungeon_block_clone_instance_vector_.clear();

        object_instance_vector::iterator it;
        for (it = object_instance_vector_value_.begin(); it != object_instance_vector_value_.end(); ++it) {
            object_instance *object_instance_value = *it;

            if (portal_enable_) {
                if (set_showing_object_instance.end() == set_showing_object_instance.find(object_instance_value))
                    continue;
            }

            if (property_types::PROPERTY_TYPE_TREE == object_instance_value->type_value) {
                assert(object_instance_value->tree);
                tree_clone_instace_vector_.push_back(object_instance_value->tree);
            } else if (property_types::PROPERTY_TYPE_BUILDING == object_instance_value->type_value) {
                assert(object_instance_value->thing_instance);
                thing_clone_instace_vector_.push_back(object_instance_value->thing_instance);
            } else if (property_types::PROPERTY_TYPE_DUNGEON_BLOCK == object_instance_value->type_value) {
                assert(object_instance_value->dungeon_block_value);
                dungeon_block_clone_instance_vector_.push_back(object_instance_value->dungeon_block_value);
            }
        }
    }

    void area::clear() {
        object_instance_vector::iterator it;
        for (it = object_instance_vector_value_.begin(); it != object_instance_vector_value_.end(); ++it)
            clear_destroy_object_instance(*it);

        object_data_vector_value_.clear();
        object_instance_vector_value_.clear();

        tree_clone_instace_vector_.clear();
        thing_clone_instace_vector_.clear();
        dungeon_block_clone_instance_vector_.clear();
        ani_thing_clone_instance_vector_.clear();
        shadow_thing_clone_instace_vector_.clear();
        ambience_clone_instance_vector_.clear();
        graphic_thing_instance_crc_map_value_.clear();
        rendered_thing_instance_crc_with_number_vector_.clear();
        rendered_grapphic_thing_instance_vector_.clear();

        portal_enable_ = false;
        clear_portal();

        effect_manager &eft_mgr = get_effect_manager();

        effect_instance_iterator i;
        for (i = effect_instance_map_value_.begin(); i != effect_instance_map_value_.end(); ++i) {
            effect_instance *effect_instance = i->second;
            eft_mgr.destroy_unsafe_effect_instance(effect_instance);
        }
        effect_instance_map_value_.clear();
    }

    void area::clear_destroy_object_instance(object_instance *object_instance_value) {
        if (object_instance_value->effect_instance_index != 0xffffffff) {
            auto f = effect_instance_map_value_.find(object_instance_value->effect_instance_index);
            if (effect_instance_map_value_.end() != f) {
                effect_instance *effect_instance = f->second;
                effect_instance_map_value_.erase(f);

                get_effect_manager().destroy_unsafe_effect_instance(effect_instance);
            }
            object_instance_value->effect_instance_index = 0xffffffff;
        }

        if (object_instance_value->attribute_instance_value) {
            object_instance_value->attribute_instance_value->clear();
            attribute_instance_pool_.free(object_instance_value->attribute_instance_value);
            object_instance_value->attribute_instance_value = nullptr;
        }

        if (object_instance_value->tree) {
            object_instance_value->tree->clear();
            get_speed_tree_forest_direct_x8().delete_instance(object_instance_value->tree);
            object_instance_value->tree = nullptr;
        }

        if (object_instance_value->thing_instance) {
            graphic_thing_instance::delete_value(object_instance_value->thing_instance);
            object_instance_value->thing_instance = nullptr;
        }

        if (object_instance_value->ambience_instance_value) {
            ambience_instance_pool_.free(object_instance_value->ambience_instance_value);
            object_instance_value->ambience_instance_value = nullptr;
        }

        if (object_instance_value->dungeon_block_value) {
            dungeon_block_instance_pool_.free(object_instance_value->dungeon_block_value);
            object_instance_value->dungeon_block_value = nullptr;
        }

        object_instance_value->clear();

        object_instance_pool_.free(object_instance_value);
    }

    void area::coordinate(unsigned short *us_coord_x, unsigned short *us_coord_y) {
        *us_coord_x = x_;
        *us_coord_y = y_;
    }

    void area::set_coordinate(const unsigned short &us_coord_x, const unsigned short &us_coord_y) {
        x_ = us_coord_x;
        y_ = us_coord_y;
    }

    void area::set_map_out_door(map_outdoor *owner_outdoor_map) {
        owner_outdoor_map_ = owner_outdoor_map;
    }

    area::area() {
        x_ = y_ = 0xFF;
    }

    area::~area() {
        clear();
    }

    void area::ambience_instance::update_impl(float fx_center, float fy_center, float fz_center) {
        if (0 == range)
            return;

        (this->*update_fn_value)(fx_center, fy_center, fz_center);
    }

    void area::ambience_instance::update_once_sound(float fx_center, float fy_center, float fz_center) {
        float distance = sqrtf((fx - fx_center) * (fx - fx_center) + (fy - fy_center) * (fy - fy_center) + (fz - fz_center) * (fz - fz_center));
        if (uint32_t(distance) < range) {
            if (-1 == play_sound_index) {
                if (ambience_data.ambience_sound_vector.empty())
                    return;

                const char *file_name = ambience_data.ambience_sound_vector[0].c_str();
                play_sound_index = get_sound_manager().play_ambience_sound3d(fx, fy, fz, file_name);
            }
        } else {
            play_sound_index = -1;
        }
    }

    void area::ambience_instance::update_step_sound(float fx_center, float fy_center, float fz_center) {
        float distance = sqrtf((fx - fx_center) * (fx - fx_center) + (fy - fy_center) * (fy - fy_center) + (fz - fz_center) * (fz - fz_center));
        if (uint32_t(distance) < range) {
            float fcur_time = get_timer().current_second();

            if (fcur_time > next_play_time) {
                if (ambience_data.ambience_sound_vector.empty())
                    return;

                const char *file_name = ambience_data.ambience_sound_vector[0].c_str();
                play_sound_index = get_sound_manager().play_ambience_sound3d(fx, fy, fz, file_name);

                next_play_time = get_timer().current_second();
                next_play_time += ambience_data.play_interval + metin2::random::fnumber(0.0f, ambience_data.play_interval_variation);
            }
        } else {
            play_sound_index = -1;
            next_play_time = 0.0f;
        }
    }

    void area::ambience_instance::update_loop_sound(float fx_center, float fy_center, float fz_center) {
        float distance = sqrtf((fx - fx_center) * (fx - fx_center) + (fy - fy_center) * (fy - fy_center) + (fz - fz_center) * (fz - fz_center));
        if (uint32_t(distance) < range) {
            if (-1 == play_sound_index) {
                if (ambience_data.ambience_sound_vector.empty())
                    return;

                const char *file_name = ambience_data.ambience_sound_vector[0].c_str();
                play_sound_index = get_sound_manager().play_ambience_sound3d(fx, fy, fz, file_name, 0);
            }

            if (-1 != play_sound_index) {
                get_sound_manager().set_sound_volume3d(play_sound_index, volume_from_distance(distance));
            }
        } else {
            if (-1 != play_sound_index) {
                get_sound_manager().stop_sound3d(play_sound_index);
                play_sound_index = -1;
            }
        }
    }

    auto area::ambience_instance::volume_from_distance(float distance) -> float {
        float max_volume_area_radius = float(range) * max_volume_area_percentage;
        if (max_volume_area_radius <= 0.0f)
            return 1.0f;
        if (distance <= max_volume_area_radius)
            return 1.0f;

        return 1.0f - ((distance - max_volume_area_radius) / (range - max_volume_area_radius));
    }

    void area::ambience_instance::render() {
        float box_size = 10.0f;
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, 0xff00ff00);
        render_cube(fx - box_size, fy - box_size, fz - box_size, fx + box_size, fy + box_size, fz + box_size);
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, 0xffffffff);
        render_sphere(nullptr, fx, fy, fz, float(range) * max_volume_area_percentage, gpu::fill::POINT);
        render_sphere(nullptr, fx, fy, fz, float(range), gpu::fill::POINT);
        render_circle2d(fx, fy, fz, float(range) * max_volume_area_percentage);
        render_circle2d(fx, fy, fz, float(range));

        for (int i = 0; i < 4; ++i) {
            float fx_add = cosf(float(i) * math::PI / 4.0f) * float(range) / 2.0f;
            float fy_add = sinf(float(i) * math::PI / 4.0f) * float(range) / 2.0f;

            if (i % 2) {
                fx_add /= 2.0f;
                fy_add /= 2.0f;
            }

            render_line2d(fx + fx_add, fy + fy_add, fx - fx_add, fy - fy_add, fz);
        }
    }

    auto area::ambience_instance::picking() -> bool {
        return graphic_collision_object::intersect_sphere(math::vector3(fx, fy, fz), range);
    }

    area::ambience_instance::ambience_instance() {
        fx = 0.0f;
        fy = 0.0f;
        fz = 0.0f;
        range = 0;
        play_sound_index = -1;
        next_play_time = 0.0f;
    }
}
