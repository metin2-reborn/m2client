#include <metin2/graphics/camera.h>
#include <metin2/graphics/state_manager.h>

#include <metin2/core/timer.h>
#include <metin2/gpu/types.h>
#include "fly_trace.h"
#include "flying_data.h"

namespace metin2::inline world::inline flying {

    dynamic_pool<fly_trace> fly_trace::pool_value;

    void fly_trace::destroy_system() {
        pool_value.destroy();
    }

    auto fly_trace::new_value() -> fly_trace * {
        return pool_value.alloc();
    }

    void fly_trace::delete_value(fly_trace *inst) {
        inst->destroy();
        pool_value.free(inst);
    }

    fly_trace::fly_trace() {
        initialize_impl();
    }

    fly_trace::~fly_trace() {
        destroy();
    }

    void fly_trace::initialize_impl() {
        rect_shape_ = false;
        color_value_ = 0;
        size_ = 0.0f;
        tail_length_ = 0.0f;
    }

    void fly_trace::destroy() {
        time_position_deque_value_.clear();

        initialize_impl();
    }

    void fly_trace::update_new_position(const math::vector3 &position) {
        time_position_deque_value_.push_front(time_position(get_timer().current_second(), position));
        while (!time_position_deque_value_.empty() && time_position_deque_value_.back().first + tail_length_ < get_timer().current_second()) {
            time_position_deque_value_.pop_back();
        }
    }

    void fly_trace::create(const flying_data::flying_attach_data &flying_attach_data) {
        color_value_ = flying_attach_data.tail_color;
        tail_length_ = flying_attach_data.tail_length;
        size_ = flying_attach_data.tail_size;
        rect_shape_ = flying_attach_data.rect_shape;
    }

    void fly_trace::update() {
    }

    // With alpha only a colour can be used; with a texture there is no alpha.

    struct fly_vertex {
        math::vector3 p;
        uint32_t c;
        math::vector2 t;
        fly_vertex() = default;
        fly_vertex(const math::vector3 &p_value, uint32_t c_value, const math::vector2 &t_value) : p(p_value), c(c_value), t(t_value) {}
    };

    struct fly_vertex_set {
        fly_vertex v[6];
        fly_vertex_set(fly_vertex *pv) {
            memcpy(v, pv, sizeof(v));
        }
        auto operator<(const fly_vertex_set &) const -> bool {
            return false;
        }
    };

    using fly_vertex_set_vector = std::vector<std::pair<float, fly_vertex_set>>;

    void fly_trace::render() {
        if (time_position_deque_value_.size() <= 1)
            return;
        fly_vertex_set_vector vs_vector;

        STATEMANAGER.save_render_state(gpu::state::Z_FUNC, gpu::compare::LESS);

        math::matrix world;
        world = math::matrix_identity();

        STATEMANAGER.save_transform(gpu::transform::WORLD, &world);
        STATEMANAGER.save_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);

        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::ONE);

        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_FUNC, gpu::compare::GREATER);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_REF, 0x00000000);

        STATEMANAGER.save_render_state(gpu::state::BLEND_OP, gpu::blend_op::ADD);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture(1, nullptr);

        math::matrix m;
        screen s;
        s.update_view_matrix();
        camera *current_camera = get_camera_manager().current_camera();
        if (!current_camera)
            return;

        const math::matrix &m_2 = current_camera->view_matrix();
        m = math::matrix_identity();
        math::vector3 f(current_camera->view());
        m._31 = f.x;
        m._32 = f.y;
        m._33 = f.z;

        frustum &frustum = s.get_frustum();

        time_position_deque::iterator it1, it2;
        it2 = it1 = time_position_deque_value_.begin();
        ++it2;
        for (; it2 != time_position_deque_value_.end(); ++it2, ++it1) {
            const math::vector3 &old = it1->second;
            const math::vector3 &new_value = it2->second;
            math::vector3 b = new_value - old;

            float radius = std::max(fabs(b.x), std::max(fabs(b.y), fabs(b.z))) / 2;
            vector3d c(it1->second.x + b.x * 0.5f,
                it1->second.y + b.y * 0.5f,
                it1->second.z + b.z * 0.5f);
            if (frustum.view_volume_test(c, radius) == VS_OUTSIDE)
                continue;

            float rate1 = (1 - (get_timer().current_second() - it1->first) / tail_length_);
            float rate2 = (1 - (get_timer().current_second() - it2->first) / tail_length_);
            float size1 = size_;
            float size2 = size_;
            if (!rect_shape_) {
                size1 *= rate1;
                size2 *= rate2;
            }
            fly_vertex v[6] =
                {
                    fly_vertex(math::vector3(0.0f, size1, 0.0f), color_value_, math::vector2(0.0f, 0.0f)),
                    fly_vertex(math::vector3(-size1, 0.0f, 0.0f), color_value_, math::vector2(0.0f, 0.5f)),
                    fly_vertex(math::vector3(size1, 0.0f, 0.0f), color_value_, math::vector2(0.5f, 0.0f)),
                    fly_vertex(math::vector3(-size2, 0.0f, 0.0f), color_value_, math::vector2(0.5f, 1.0f)),
                    fly_vertex(math::vector3(size2, 0.0f, 0.0f), color_value_, math::vector2(1.0f, 0.5f)),
                    fly_vertex(math::vector3(0.0f, -size2, 0.0f), color_value_, math::vector2(1.0f, 1.0f)),
            };

            math::vector3 e(m_2._41, m_2._42, m_2._43);
            e = current_camera->eye();
            e -= it1->second;

            math::vector3 p;
            p = math::cross(b, e);

            math::vector3 u;
            u = math::cross(f, p);
            u = math::normalize(u);
            math::vector3 r;
            r = math::cross(f, u);
            m._21 = u.x;
            m._22 = u.y;
            m._23 = u.z;
            m._11 = r.x;
            m._12 = r.y;
            m._13 = r.z;
            int i;
            for (i = 0; i < 6; i++)
                v[i].p = math::transform_normal(v[i].p, m);
            for (i = 0; i < 3; i++)
                v[i].p += it1->second;
            for (; i < 6; i++)
                v[i].p += it2->second;

            vs_vector.push_back(std::make_pair(-math::dot(e, current_camera->view()), fly_vertex_set(v)));
        }

        std::sort(vs_vector.begin(), vs_vector.end());

        for (auto it = vs_vector.begin(); it != vs_vector.end(); ++it) {
            STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 4, it->second.v, sizeof(vertex));
        }
        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
        STATEMANAGER.restore_vertex_shader();
        STATEMANAGER.restore_transform(gpu::transform::WORLD);
        STATEMANAGER.restore_render_state(gpu::state::Z_FUNC);
        STATEMANAGER.restore_render_state(gpu::state::BLEND_OP);

        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_FUNC);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_REF);
    }
}
