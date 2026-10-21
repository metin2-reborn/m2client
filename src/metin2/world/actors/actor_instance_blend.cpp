#include "actor_instance.h"

namespace metin2::inline world::inline actors {

    void actor_instance::blend_alpha_value(float dst_alpha, float duration) {
        blend_alpha_apply(dst_alpha, duration);
    }

    void actor_instance::set_blend_render_mode() {
        render_mode_value_ = RENDER_MODE_BLEND;
    }

    void actor_instance::set_alpha_value(float alpha) {
        alpha_value_ = alpha;
    }

    auto actor_instance::alpha_value() -> float {
        return alpha_value_;
    }

    void actor_instance::blend_alpha_initialize() {
        blend_alpha_value_.is_blending = false;
        blend_alpha_value_.base_time = 0.0f;
        blend_alpha_value_.duration = 0.0f;
        blend_alpha_value_.base_alpha = 0.0f;
        blend_alpha_value_.dst_alpha = 0.0f;
        blend_alpha_value_.old_render_mode = RENDER_MODE_NORMAL;
    }

    void actor_instance::blend_alpha_apply(float dst_alpha, float duration) {
        blend_alpha_value_.is_blending = true;
        blend_alpha_value_.base_alpha = alpha_value();
        blend_alpha_value_.base_time = local_time();
        blend_alpha_value_.duration = duration;
        blend_alpha_value_.dst_alpha = dst_alpha;
        blend_alpha_value_.old_render_mode = render_mode_value_;
    }

    void actor_instance::blend_alpha_update() {
        if (!blend_alpha_value_.is_blending)
            return;

        float elapsed_time = blend_alpha_get_elapsed_time();

        if (elapsed_time < blend_alpha_value_.duration) {
            float cur_alpha = blend_alpha_value_.base_alpha + (blend_alpha_value_.dst_alpha - blend_alpha_value_.base_alpha) * elapsed_time / blend_alpha_value_.duration;
            set_blend_render_mode();
            set_alpha_value(cur_alpha);
        } else {
            if (1.0f > blend_alpha_value_.dst_alpha) {
                set_blend_render_mode();
            } else {
                render_mode_value_ = blend_alpha_value_.old_render_mode;
            }

            set_alpha_value(blend_alpha_value_.dst_alpha);

            blend_alpha_update_complete();
        }
    }

    void actor_instance::blend_alpha_update_complete() {
        blend_alpha_value_.is_blending = false;
    }

    auto actor_instance::blend_alpha_get_elapsed_time() -> float {
        float cur_time = local_time();
        return cur_time - blend_alpha_value_.base_time;
    }
}
