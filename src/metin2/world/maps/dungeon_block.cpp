#include "dungeon_block.h"
#include <metin2/log/log.h>

#include <metin2/core/stl.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/state_manager.h>

namespace metin2::inline world::inline maps {

    class dungeon_model_instance : public model_instance {
      public:
        dungeon_model_instance() = default;
        ~dungeon_model_instance() override = default;

        void render_dungeon_block() {
            if (is_empty())
                return;

            STATEMANAGER.set_vertex_shader(pnt2vs_);
            gpu::vertex_buffer *rigid_vertex_buffer = model_value_->gpu_pnt_vertex_buffer();
            if (rigid_vertex_buffer) {
                STATEMANAGER.set_stream_source(0, rigid_vertex_buffer, sizeof(tpnt2_vertex));
                render_mesh_node_list_with_two_texture(mesh::TYPE_RIGID, material::TYPE_BLEND_PNT);
            }
        }

        void render_dungeon_block_shadow() {
            if (is_empty())
                return;

            STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, 0xffffffff);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TFACTOR);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
            STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
            STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::ZERO);
            STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::SRC_COLOR);

            STATEMANAGER.set_vertex_shader(pnt2vs_);
            gpu::vertex_buffer *rigid_vertex_buffer = model_value_->gpu_pnt_vertex_buffer();
            if (rigid_vertex_buffer) {
                STATEMANAGER.set_stream_source(0, rigid_vertex_buffer, sizeof(tpnt2_vertex));
                render_mesh_node_list_without_texture(mesh::TYPE_RIGID, material::TYPE_BLEND_PNT);
            }

            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG1);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_OP);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_OP);
            STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
            STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
            STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
        }
    };

    struct update_fn {
        float elapsed_time;
        math::matrix *pmat_world;
        void operator()(model_instance *instance) {
            instance->update(model_instance::ANIFPS_MIN);
            instance->update_local_time(elapsed_time);
            instance->deform(pmat_world);
        }
    };

    void dungeon_block::update() {
        transform();

        update_fn update;
        update.elapsed_time = 0.0f;
        update.pmat_world = &world_matrix_;
        for_each(model_instance_container_value_.begin(), model_instance_container_value_.end(), update);
    }

    struct render_fn {
        void operator()(dungeon_model_instance *instance) {
            instance->render_dungeon_block();
        }
    };

    void dungeon_block::render() {

        for_each(model_instance_container_value_.begin(), model_instance_container_value_.end(), render_fn());
    }

    struct render_shadow_fn {
        void operator()(dungeon_model_instance *instance) {
            instance->render_dungeon_block_shadow();
        }
    };

    void dungeon_block::on_render_shadow() {
        for_each(model_instance_container_value_.begin(), model_instance_container_value_.end(), render_shadow_fn());
    }

    struct bound_box_fn {
        math::vector3 *pv3_min;
        math::vector3 *pv3_max;

        bound_box_fn(math::vector3 *min_value, math::vector3 *max_value) {
            pv3_min = min_value;
            pv3_max = max_value;
        }
        void operator()(model_instance *instance) {
            instance->get_bound_box(pv3_min, pv3_max);
        }
    };

    auto dungeon_block::bounding_sphere(math::vector3 &center, float &radius) -> bool {
        center = center_;
        radius = radius_;
        center = math::transform_coord(center, get_transform());
        return true;
    }

    void dungeon_block::on_update_collision_data(const static_collision_data_vector *pscd_vector) {
        assert(pscd_vector);
        static_collision_data_vector::const_iterator it;
        for (it = pscd_vector->begin(); it != pscd_vector->end(); ++it) {
            add_collision(&(*it), &get_transform());
        }
    }

    void dungeon_block::on_update_heigh_instance(attribute_instance *attribute_instance) {
        assert(attribute_instance);
        set_height_instance(attribute_instance);
    }

    auto dungeon_block::on_get_object_height(float x, float y, float *height) -> bool {
        if (height_attribute_instance_ && height_attribute_instance_->height(x, y, height))
            return true;
        return false;
    }

    void dungeon_block::build_bounding_sphere() {
        math::vector3 min_value, max_value;
        for_each(model_instance_container_value_.begin(), model_instance_container_value_.end(), bound_box_fn(&min_value, &max_value));

        center_ = (min_value + max_value) * 0.5f;
        const auto vv = (max_value - min_value);
        radius_ = math::length(vv) * 0.5f + 150.0f; // extra length for attached objects
    }

    auto dungeon_block::intersect(float *pfu, float *pfv, float *pft) -> bool {
        auto itor = model_instance_container_value_.begin();
        for (; itor != model_instance_container_value_.end(); ++itor) {
            dungeon_model_instance *instance = *itor;
            if (instance->intersect(&graphic_object_instance::get_transform(), pfu, pfv, pft))
                return true;
        }

        return false;
    }

    void dungeon_block::get_bound_box(math::vector3 *pv3_min, math::vector3 *pv3_max) {
        pv3_min->x = +10000000.0f;
        pv3_min->y = +10000000.0f;
        pv3_min->z = +10000000.0f;
        pv3_max->x = -10000000.0f;
        pv3_max->y = -10000000.0f;
        pv3_max->z = -10000000.0f;

        auto itor = model_instance_container_value_.begin();
        for (; itor != model_instance_container_value_.end(); ++itor) {
            dungeon_model_instance *instance = *itor;

            math::vector3 v3_min;
            math::vector3 v3_max;
            instance->get_bound_box(&v3_min, &v3_max);

            pv3_min->x = std::min(v3_min.x, pv3_min->x);
            pv3_min->y = std::min(v3_min.y, pv3_min->y);
            pv3_min->z = std::min(v3_min.z, pv3_min->z);
            pv3_max->x = std::max(v3_max.x, pv3_max->x);
            pv3_max->y = std::max(v3_max.y, pv3_max->y);
            pv3_max->z = std::max(v3_max.z, pv3_max->z);
        }
    }

    auto dungeon_block::load(const char *file_name) -> bool {
        destroy();

        thing_ = (graphic_thing *)get_resource_manager().resource_pointer(file_name);

        thing_->add_reference();
        if (thing_->model_count() <= 0) {
            metin2::log::error("Dungeon block {}: model count is {}", file_name, thing_->model_count());
            return false;
        }

        model_instance_container_value_.reserve(thing_->model_count());

        for (int i = 0; i < thing_->model_count(); ++i) {
            auto *model_instance = new dungeon_model_instance;
            model_instance->set_main_model_pointer(thing_->model_pointer(i), &deformable_vertex_buffer_);
            uint32_t vertex_count = model_instance->vertex_count();
            deformable_vertex_buffer_.destroy();
            deformable_vertex_buffer_.create(
                vertex_count,
                gpu::fvf::XYZ | gpu::fvf::NORMAL | gpu::fvf::TEX1,
                gpu::usage::WRITE_ONLY);
            model_instance_container_value_.push_back(model_instance);
        }

        return true;
    }

    void dungeon_block::initialize_impl() {
        center_ = math::vector3(0.0f, 0.0f, 0.0f);
        radius_ = 0.0f;

        thing_ = nullptr;
    }

    void dungeon_block::destroy() {
        if (thing_) {
            thing_->release();
            thing_ = nullptr;
        }

        stl_wipe(model_instance_container_value_);

        initialize_impl();
    }

    dungeon_block::dungeon_block() {
        initialize_impl();
    }
    dungeon_block::~dungeon_block() {
        destroy();
    }
}
