#include "snow_environment.h"

#include <metin2/core/stl.h>
#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/graphics/state_manager.h>
#include "snow_particle.h"

namespace metin2::inline world::inline maps {

    void snow_environment::enable() {
        if (!snow_enable_) {
            create();
        }

        snow_enable_ = true;
    }

    void snow_environment::disable() {
        snow_enable_ = false;
    }

    void snow_environment::update(const math::vector3 &pos) {
        if (!snow_enable_) {
            if (vct_particle_snow_.empty())
                return;
        }

        center_ = pos;
    }

    void snow_environment::deform() {
        if (!snow_enable_) {
            if (vct_particle_snow_.empty())
                return;
        }

        const math::vector3 &pos = center_;

        // The frame step, not a static clock: that one was not advanced while snow was off and
        // gave the first frame after it came back a huge step.
        const float elapsed_time = get_timer().elapsed_second();

        camera *camera = get_camera_manager().current_camera();
        if (!camera)
            return;

        const math::vector3 &view = camera->view();

        math::vector3 changed_pos = view * 3500.0f + pos;
        changed_pos.z = pos.z;

        auto itor = vct_particle_snow_.begin();
        for (; itor != vct_particle_snow_.end();) {
            snow_particle *snow = *itor;
            snow->update(elapsed_time, changed_pos);

            if (!snow->is_activate()) {
                snow_particle::delete_value(snow);

                itor = vct_particle_snow_.erase(itor);
            } else {
                ++itor;
            }
        }

        if (snow_enable_) {
            const size_t count = vct_particle_snow_.size();
            const size_t free = particle_max_num_ > count ? particle_max_num_ - count : 0;
            for (size_t p = 0; p < std::min<size_t>(10, free); ++p) {
                snow_particle *snow_particle = snow_particle::new_value();
                snow_particle->init(changed_pos);
                vct_particle_snow_.push_back(snow_particle);
            }
        }
    }

    void snow_environment::begin_blur() {
        if (!blur_enable_)
            return;

        if (!snow_target_.begin())
            return;
        device_->clear({}, gpu::clear::TARGET | gpu::clear::Z_BUFFER, 0x00000000, 1.0f, 0);

        STATEMANAGER.set_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.set_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        STATEMANAGER.set_render_state(gpu::state::DEST_BLEND, gpu::blend::DEST_ALPHA);
    }

    void snow_environment::apply_blur() {
        if (!blur_enable_)
            return;

        {
            snow_target_.end();

            STATEMANAGER.set_texture(0, snow_target_.texture());
            STATEMANAGER.set_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);

            const gpu::texture *target = device_->render_target();
            auto sx = static_cast<float>(target ? target->width() : device_->back_buffer_width());
            auto sy = static_cast<float>(target ? target->height() : device_->back_buffer_height());

            blur_vertex v[4] = {blur_vertex(math::vector3(0.0f, 0.0f, 0.0f), 1.0f, 0xFFFFFF, 0, 0),
                blur_vertex(math::vector3(sx, 0.0f, 0.0f), 1.0f, 0xFFFFFF, 1, 0),
                blur_vertex(math::vector3(0.0f, sy, 0.0f), 1.0f, 0xFFFFFF, 0, 1),
                blur_vertex(math::vector3(sx, sy, 0.0f), 1.0f, 0xFFFFFF, 1, 1)};

            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZRHW | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, v, sizeof(blur_vertex));
        }
    }

    void snow_environment::render() {
        if (!snow_enable_) {
            if (vct_particle_snow_.empty())
                return;
        }

        begin_blur();

        uint32_t particle_count = uint32_t(std::min<size_t>(particle_max_num_, vct_particle_snow_.size()));

        camera *camera = get_camera_manager().current_camera();
        if (!camera)
            return;

        const math::vector3 &up = camera->up();
        const math::vector3 &cross = camera->cross();

        void *data;
        if (vb_->lock(0, sizeof(particle_vertex) * particle_count * 4, &data, gpu::lock::DISCARD)) {
            auto *verticies = static_cast<particle_vertex *>(data);
            uint32_t i = 0;
            auto itor = vct_particle_snow_.begin();
            for (; i < particle_count && itor != vct_particle_snow_.end(); ++i, ++itor) {
                snow_particle *snow = *itor;
                snow->set_camera_vertex(up, cross);
                snow->verticies(verticies[i * 4 + 0],
                    verticies[i * 4 + 1],
                    verticies[i * 4 + 2],
                    verticies[i * 4 + 3]);
            }
            vb_->unlock();
        }

        STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);
        STATEMANAGER.set_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        STATEMANAGER.set_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        image_instance_->graphic_image_pointer()->texture_reference().set_texture_stage(0);
        STATEMANAGER.set_indices(ib_.get(), 0);
        STATEMANAGER.set_stream_source(0, vb_.get(), sizeof(particle_vertex));
        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::TEX1);
        STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, particle_count * 4, 0, particle_count * 2);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);

        apply_blur();
    }

    auto snow_environment::create_blur_texture() -> bool {
        if (!blur_enable_)
            return true;

        return snow_target_.create(blur_texture_size_, blur_texture_size_, gpu::format::X8R8G8B8, gpu::format::D16);
    }

    auto snow_environment::create_geometry() -> bool {
        vb_ = device_->create_vertex_buffer(sizeof(particle_vertex) * particle_max_num_ * 4, gpu::usage::DYNAMIC | gpu::usage::WRITE_ONLY);
        ib_ = device_->create_index_buffer(sizeof(uint16_t) * particle_max_num_ * 6, gpu::usage::WRITE_ONLY, gpu::format::INDEX16);
        if (!vb_ || !ib_)
            return false;

        void *data;
        if (!ib_->lock(0, sizeof(uint16_t) * particle_max_num_ * 6, &data))
            return false;

        auto *dst_indices = static_cast<uint16_t *>(data);
        const uint16_t fill_rect_indices[6] = {
            0,
            2,
            1,
            2,
            3,
            1,
        };
        for (uint32_t i = 0; i < particle_max_num_; ++i) {
            for (int j = 0; j < 6; ++j) {
                dst_indices[i * 6 + j] = i * 4 + fill_rect_indices[j];
            }
        }

        ib_->unlock();
        return true;
    }

    auto snow_environment::create() -> bool {
        destroy();

        if (!create_blur_texture())
            return false;

        if (!create_geometry())
            return false;

        auto *image = (graphic_image *)get_resource_manager().resource_pointer("d:/ymir work/special/snow.dds");
        image_instance_ = graphic_image_instance::new_value();
        image_instance_->set_image_pointer(image);

        return true;
    }

    void snow_environment::destroy() {
        snow_target_.destroy();
        vb_.reset();
        ib_.reset();

        stl_wipe(vct_particle_snow_);
        snow_particle::destroy_pool();

        if (image_instance_) {
            graphic_image_instance::delete_value(image_instance_);
            image_instance_ = nullptr;
        }

        initialize_impl();
    }

    void snow_environment::initialize_impl() {
        snow_enable_ = false;
        image_instance_ = nullptr;

        vct_particle_snow_.reserve(particle_max_num_);
    }

    snow_environment::snow_environment() {
        blur_enable_ = false;
        particle_max_num_ = 3000;
        blur_texture_size_ = 512;

        initialize_impl();
    }
    snow_environment::~snow_environment() {
        destroy();
    }
}
