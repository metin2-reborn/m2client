#include <metin2/graphics/resource_manager.h>
#include <metin2/graphics/state_manager.h>

#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include "weapon_trace.h"

namespace metin2::inline world::inline actors {

    dynamic_pool<weapon_trace> weapon_trace::pool_value_;

    void weapon_trace::destroy_system() {
        pool_value_.destroy();
    }

    void weapon_trace::delete_value(weapon_trace *wt_del) {
        assert(wt_del != nullptr && "CWeaponTrace::Delete");

        wt_del->clear();
        pool_value_.free(wt_del);
    }

    auto weapon_trace::new_value() -> weapon_trace * {
        return pool_value_.alloc();
    }

    void weapon_trace::update(float reach_scale) {
        float elapsed_time = get_timer().current_second() - last_update_;
        last_update_ = get_timer().current_second();

        if (!instance_)
            return;
        {
            // Keep only the points inside the trail's time window.
            time_point_list::iterator it;
            for (it = short_time_point_list_.begin(); it != short_time_point_list_.end(); ++it) {
                it->first += elapsed_time;
                if (it->first > life_time_) {
                    it++;
                    break;
                }
            }
            if (it != short_time_point_list_.end())
                short_time_point_list_.erase(it, short_time_point_list_.end());
            for (it = long_time_point_list_.begin(); it != long_time_point_list_.end(); ++it) {
                it->first += elapsed_time;
                if (it->first > life_time_) {
                    it++;
                    break;
                }
            }
            if (it != long_time_point_list_.end())
                long_time_point_list_.erase(it, long_time_point_list_.end());
        }

        if (is_playing_ && fz_ >= 0.0001f) {
            math::matrix *matrix;
            if (instance_->composite_bone_matrix(model_instance_index_, bone_index_, &matrix)) {
                math::matrix *bone_mat;
                instance_->bone_matrix(model_instance_index_, bone_index_, &bone_mat);
                math::matrix mat = *matrix;
                mat._41 = bone_mat->_41;
                mat._42 = bone_mat->_42;
                mat._43 = bone_mat->_43;
                math::matrix point;
                math::matrix translation;
                math::matrix rotation;

                translation = math::matrix_translation(0.0f, 0.0f, length_ * reach_scale);
                rotation = math::matrix_rotation_z(math::to_radian(rotation_));

                point = mat * rotation;
                short_time_point_list_.push_front(
                    time_point(
                        0.0f,
                        math::vector3(
                            fx_ + point._41,
                            fy_ + point._42,
                            fz_ + point._43)));

                point = translation * point;
                long_time_point_list_.push_front(
                    time_point(
                        0.0f,
                        math::vector3(
                            fx_ + point._41,
                            fy_ + point._42,
                            fz_ + point._43)));
            }
        }
    }

    auto weapon_trace::build_vertex() -> bool {
        const int max_size = 300;
        float h[max_size];
        float stk[max_size];
        int sp = 0;
        math::vector3 r[max_size];

        if (long_time_point_list_.size() <= 1)
            return false;

        std::vector<pdt_vertex> short_vertex_vector, long_vertex_vector;

        float length = std::min(life_time_, long_time_point_list_.back().first);

        int n = long_time_point_list_.size() - 1;
        assert(n < max_size - 1);

        // Cubic spline.

        for (int loop = 0; loop <= 1; ++loop) {
            time_point_list &input = (loop) ? long_time_point_list_ : short_time_point_list_;
            std::vector<pdt_vertex> &output = (loop) ? long_vertex_vector : short_vertex_vector;
            time_point_list::iterator it;
            int i;

            for (i = 0; i < n; ++i) {
                h[i] = input[i + 1].first - input[i].first;
                r[i] = (input[i + 1].second - input[i].second) * (3 / h[i]);
            }
            r[n] = math::vector3(0.0f, 0.0f, 0.0f);
            for (i = n; i > 0; i--) {
                r[i] += r[i - 1];
            }

            float rate = 0.5f;
            r[0] *= 0.5f;
            stk[sp++] = rate;
            for (i = 1; i < n; i++) {
                r[i] -= r[i - 1];
                rate = 1 / (4 - rate);
                r[i] *= rate;
                stk[sp++] = rate;
            }
            r[n] -= r[n - 1];
            rate = 1 / (2 - rate);
            r[n] *= rate;

            for (i = n - 1; i >= 0; i--) {
                r[i] -= stk[--sp] * r[i + 1];
            }

            int base = 0;
            math::vector3 a, b, c, d;
            math::vector3 tmp = input[base + 1].second - input[base].second;
            float timebase = 0, timenext = h[base], dt = sampling_time_;
            a = input[base].second;
            b = r[base];
            c = (3 * tmp - r[base + 1] * h[base] - (2 * h[base]) * r[base]) * (1 / (h[base] * h[base]));
            d = (-2 * tmp + (r[base + 1] + r[base]) * h[base]) * (1 / (h[base] * h[base] * h[base]));

            for (float t = 0; t <= length; t += dt) {
                while (t > timenext) {
                    timebase = timenext;
                    base++;
                    if (base >= n) break;
                    math::vector3 tmp = input[base + 1].second - input[base].second;
                    a = input[base].second;
                    b = r[base];
                    c = (3 * tmp - r[base + 1] * h[base] - (2 * h[base]) * r[base]) * (1 / (h[base] * h[base]));
                    d = (-2 * tmp + (r[base + 1] + r[base]) * h[base]) * (1 / (h[base] * h[base] * h[base]));

                    timenext += h[base];
                    if (loop) {
                    }
                }
                if (base > n) break;
                float cc = t - timebase;

                pdt_vertex v;
                float ttt = std::min(std::max((t + input[0].first) / life_time_, 0.0f), 1.0f);
                v.diffuse_value = math::color(0.3f, 0.8f, 1.0f, (loop) ? std::min(std::max((1.0f - ttt) * (1.0f - ttt) / 2.5f - 0.1f, 0.0f), 1.0f) : 0.0f);
                v.position_value = a + cc * (b + cc * (c + cc * d));
                v.tex_coord.x = t / life_time_;
                v.tex_coord.y = loop ? 0 : 1;
                output.push_back(v);
                if (loop) {
                }
            }
        }

        pdt_vertex_vector_.clear();

        std::vector<pdt_vertex>::iterator lit, sit;
        for (lit = long_vertex_vector.begin(), sit = short_vertex_vector.begin();
            lit != long_vertex_vector.end();
            ++lit, ++sit) {
            pdt_vertex_vector_.push_back(*lit);
            pdt_vertex_vector_.push_back(*sit);
        }

        return true;
    }

    void weapon_trace::render() {

        if (!build_vertex())
            return;

        if (pdt_vertex_vector_.size() < 4)
            return;

        gpu::texture *texture = nullptr;

        math::matrix world;
        world = math::matrix_identity();

        STATEMANAGER.save_transform(gpu::transform::WORLD, &world);
        STATEMANAGER.save_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);

        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);

        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_REF, 0x00000011);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_FUNC, gpu::compare::GREATER);

        STATEMANAGER.save_render_state(gpu::state::Z_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::Z_FUNC, gpu::compare::LESS_EQUAL);
        STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, false);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, (use_texture_) ? gpu::texture_op::SELECT_ARG2 : gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, (use_texture_) ? gpu::texture_op::SELECT_ARG2 : gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.set_texture(0, texture);
        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP,
            int(pdt_vertex_vector_.size() - 2),
            &pdt_vertex_vector_[0],
            sizeof(pdt_vertex));

        STATEMANAGER.set_render_state(gpu::state::LIGHTING, true);

        STATEMANAGER.restore_render_state(gpu::state::Z_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::Z_FUNC);
        STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);

        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_REF);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_FUNC);

        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);

        STATEMANAGER.restore_transform(gpu::transform::WORLD);
        STATEMANAGER.restore_vertex_shader();
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
    }

    void weapon_trace::use_alpha() {
        use_texture_ = false;
    }

    void weapon_trace::use_texture() {
        use_texture_ = true;
    }

    void weapon_trace::set_texture(const char *file_name) {
        auto *image = (graphic_image *)get_resource_manager().resource_pointer("lot_ade10-2.tga");
        image_instance_.set_image_pointer(image);
    }

    auto weapon_trace::set_weapon_instance(graphic_thing_instance *instance, uint32_t model_index, const char *bone_name) -> bool {
        instance->update();
        instance->deform_no_skin();

        math::vector3 min_value;
        math::vector3 max_value;
        if (!instance->get_bound_box(model_index, &min_value, &max_value))
            return false;

        bone_index_ = 0;
        model_instance_index_ = model_index;

        instance_ = instance;
        math::matrix *pmat;
        instance->bone_matrix(model_index, 0, &pmat);
        math::vector3 bone(pmat->_41, pmat->_42, pmat->_43);

        const auto vv1 = (bone - min_value);
        const auto vv2 = (bone - max_value);
        length_ =
            sqrtf(
                f_max(
                    math::length_sq(vv1),
                    math::length_sq(vv2)));

        return true;
    }

    void weapon_trace::set_position(float fx, float fy, float fz) {
        fx_ = fx;
        fy_ = fy;
        fz_ = fz;
    }

    void weapon_trace::set_rotation(float rotation) {
        rotation_ = rotation;
    }

    void weapon_trace::turn_on() {
        is_playing_ = true;
    }
    void weapon_trace::turn_off() {
        is_playing_ = false;
    }

    void weapon_trace::clear() {

        short_time_point_list_.clear();
        long_time_point_list_.clear();
        initialize();
    }

    void weapon_trace::initialize() {
        instance_ = nullptr;
        model_instance_index_ = 0;

        fx_ = 0.0f;
        fy_ = 0.0f;
        fz_ = 0.0f;
        rotation_ = 0.0f;

        life_time_ = 0.18f;
        sampling_time_ = 0.003f;

        is_playing_ = false;

        use_texture_ = false;

        bone_index_ = 0;

        last_update_ = get_timer().current_second();
    }

    weapon_trace::weapon_trace() {
        initialize();
    }
    weapon_trace::~weapon_trace() = default;
}
