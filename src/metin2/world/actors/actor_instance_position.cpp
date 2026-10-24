#include "actor_instance.h"

namespace metin2::inline world::inline actors {

    auto actor_instance::new_get_last_pixel_position_ref() -> const pixel_position & {
        blending_position(&p_pos_last_);
        p_pos_last_.y = -p_pos_last_.y;

        return p_pos_last_;
    }

    auto actor_instance::movement_vector_ref() -> const math::vector3 & {
        if (horse_)
            return horse_->movement_vector_ref();

        return movement_;
    }

    void actor_instance::new_set_atk_pixel_position(const pixel_position &p_pos_atk) {
        p_pos_atk_ = p_pos_atk;
    }

    void actor_instance::set_cur_pixel_position(const pixel_position &p_pos_cur) {
        math::vector3 pos_cur;
        pos_cur.x = +p_pos_cur.x;
        pos_cur.y = -p_pos_cur.y;
        pos_cur.z = +p_pos_cur.z;

        set_pixel_position(pos_cur);
    }

    void actor_instance::new_set_src_pixel_position(const pixel_position &p_pos_src) {
        p_pos_src_ = p_pos_src;
    }

    void actor_instance::new_set_dst_pixel_position_z(float z) {
        p_pos_dst_.z = z;
    }

    void actor_instance::new_set_dst_pixel_position(const pixel_position &p_pos_dst) {
        p_pos_dst_ = p_pos_dst;
    }

    auto actor_instance::new_get_atk_pixel_position_ref() -> const pixel_position & {
        return p_pos_atk_;
    }

    auto actor_instance::new_get_src_pixel_position_ref() -> const pixel_position & {
        return p_pos_src_;
    }

    auto actor_instance::new_get_dst_pixel_position_ref() -> const pixel_position & {
        return p_pos_dst_;
    }

    auto actor_instance::new_get_cur_pixel_position_ref() -> const pixel_position & {
        p_pos_cur_.x = +x_;
        p_pos_cur_.y = -y_;
        p_pos_cur_.z = +z_;

        return p_pos_cur_;
    }

    void actor_instance::get_pixel_position(pixel_position *pixel_position) {
        pixel_position->x = x_;
        pixel_position->y = y_;
        pixel_position->z = z_;
    }

    void actor_instance::set_pixel_position(const pixel_position &pixel_pos) {
        if (tree_) {
            set_tree_position(pixel_pos.x, pixel_pos.y, pixel_pos.z);
        }

        if (horse_)
            horse_->set_pixel_position(pixel_pos);

        x_ = pixel_pos.x;
        y_ = pixel_pos.y;
        z_ = pixel_pos.z;
        need_update_collision_ = true;
    }

    void actor_instance::initialize_position_data() {
        shake_time_ = 0;
        flash_time_ = 0;

        x_ = 0.0f;
        y_ = 0.0f;
        z_ = 0.0f;
        need_update_collision_ = false;

        p_pos_atk_ = p_pos_last_ = p_pos_dst_ = p_pos_cur_ = p_pos_src_ = pixel_position(0.0f, 0.0f, 0.0f);

        initialize_movement();
    }
}
