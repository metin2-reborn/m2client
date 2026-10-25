#include "actor_instance.h"
#include <metin2/trees/speed_tree_forest_direct_x8.h>
#include <metin2/trees/speed_tree_wrapper.h>
#include <metin2/world/terrain/area_terrain.h>
#include "race_data.h"

#include <cmath>

namespace metin2::inline world::inline actors {

    enum {
        MAIN_RACE_MAX_NUM = 8,
    };

    void actor_instance::instancebase_deform() {
        deform();
        trace_process();
    }

    void actor_instance::instancebase_transform() {
        if (horse_) {
            horse_->instancebase_transform();

            x_ = horse_->new_get_cur_pixel_position_ref().x;
            y_ = -horse_->new_get_cur_pixel_position_ref().y;
            z_ = horse_->new_get_cur_pixel_position_ref().z;
            need_update_collision_ = true;
        }

        update();
        transform_process();
        transform();
        update_point_instance();
        shake_process();
        update_bounding_sphere();
        update_attribute();
    }

    void actor_instance::on_update() {
        if (!is_paralysis())
            graphic_thing_instance::on_update();

        update_attaching_instances();

        blend_alpha_update();
    }

    auto actor_instance::get_background() -> background_interface & {
        return get_background_interface();
    }

    void actor_instance::set_main_instance() {
        is_main_ = true;
    }

    void actor_instance::set_sleep(bool is_sleep) {
        is_sleep_ = is_sleep;

        stop();
    }

    void actor_instance::set_resist_fallen(bool is_resist_fallen) {
        is_resist_fallen_ = is_resist_fallen;
    }

    void actor_instance::set_reach_scale(float scale) {
        reach_scale_ = scale;
    }

    auto actor_instance::reach_scale() -> float {
        return reach_scale_;
    }

    auto actor_instance::attack_speed() -> float {
        return atk_spd_;
    }

    auto actor_instance::current_combo_type() -> uint16_t {
        if (is_bow_mode())
            return 0;
        if (is_hand_mode())
            return 0;
        if (is_mounting_horse_impl())
            return 0;

        return wcur_combo_type_;
    }

    void actor_instance::set_combo_type(uint16_t combo_type) {
        wcur_combo_type_ = combo_type;
    }

    void actor_instance::set_attack_speed(float atk_spd) {
        atk_spd_ = atk_spd;
    }

    void actor_instance::set_move_speed(float mov_spd) {
        if (mov_spd_ == mov_spd)
            return;

        mov_spd_ = mov_spd;

        // Re-issued straight from the move motion: going through the wait motion fired on_stop and
        // on_move, whose packets ended and restarted the sprint that changed the speed.
        if (is_move_motion()) {
            move();
        }
    }

    void actor_instance::set_fishing_position(math::vector3 &position) {
        fishing_position_ = position;
    }

    void actor_instance::move() {
        if (is_walking_) {
            set_loop_motion(race_motion_data::NAME_WALK, 0.15f, mov_spd_);
        } else {
            set_loop_motion(race_motion_data::NAME_RUN, 0.15f, mov_spd_);
        }
    }

    void actor_instance::stop(float blending_time) {
        clear_motion();
        set_loop_motion(race_motion_data::NAME_WAIT, blending_time);
    }

    void actor_instance::set_owner(uint32_t owner_vid) {
        owner_base_time_ = local_time();
        owner_vid_ = owner_vid;
    }

    void actor_instance::set_actor_type(uint32_t type_value) {
        actor_type_ = type_value;
    }

    auto actor_instance::actor_type() const -> uint32_t {
        return actor_type_;
    }

    auto actor_instance::is_hand_mode() -> bool {
        if (race_motion_data::MODE_GENERAL == motion_mode())
            return true;

        if (race_motion_data::MODE_HORSE == motion_mode())
            return true;

        return false;
    }

    auto actor_instance::is_two_hand_mode() -> bool {
        if (race_motion_data::MODE_TWOHAND_SWORD == motion_mode())
            return true;

        return false;
    }

    auto actor_instance::is_bow_mode() -> bool {
        if (race_motion_data::MODE_BOW == motion_mode())
            return true;

        if (race_motion_data::MODE_HORSE_BOW == motion_mode())
            return true;

        return false;
    }

    auto actor_instance::is_poly() -> bool {
        if (TYPE_POLY == actor_type_)
            return true;

        if (TYPE_PC == actor_type_)
            if (race_ >= MAIN_RACE_MAX_NUM)
                return true;

        return false;
    }

    auto actor_instance::is_pc() -> bool {
        if (TYPE_PC == actor_type_)
            return true;

        return false;
    }

    auto actor_instance::is_npc() -> bool {
        if (TYPE_NPC == actor_type_)
            return true;

        return false;
    }

    auto actor_instance::is_enemy() -> bool {
        if (TYPE_ENEMY == actor_type_)
            return true;

        return false;
    }

    auto actor_instance::is_stone() -> bool {
        if (TYPE_STONE == actor_type_)
            return true;

        return false;
    }

    auto actor_instance::is_warp() -> bool {
        if (TYPE_WARP == actor_type_)
            return true;

        return false;
    }

    auto actor_instance::is_goto() -> bool {
        if (TYPE_GOTO == actor_type_)
            return true;

        return false;
    }

    auto actor_instance::is_building() -> bool {
        if (TYPE_BUILDING == actor_type_)
            return true;

        return false;
    }

    auto actor_instance::is_door() -> bool {
        if (TYPE_DOOR == actor_type_)
            return true;

        return false;
    }

    auto actor_instance::is_object() -> bool {
        if (TYPE_OBJECT == actor_type_)
            return true;

        return false;
    }

    void actor_instance::destroy_system() {
    }

    void actor_instance::die_end() {
        die();

        graphic_thing_instance::set_motion_at_end();
    }

    void actor_instance::die() {
        if (is_real_dead_)
            return;

        if (is_move_motion())
            stop();

        set_advancing_rotation(rotation());

        if (is_stone()) {
            intercept_once_motion(race_motion_data::NAME_DEAD);
        } else {
            if (!is_die_motion()) {
                intercept_once_motion(race_motion_data::NAME_DEAD);
            }
        }

        is_real_dead_ = true;
    }

    auto actor_instance::is_sleep() -> bool {
        return is_sleep_;
    }

    auto actor_instance::is_paralysis() -> bool {
        return is_paralysis_;
    }

    auto actor_instance::is_faint() -> bool {
        return is_faint_;
    }

    auto actor_instance::is_resist_fallen() -> bool {
        return is_resist_fallen_;
    }

    auto actor_instance::is_moving() -> bool {
        return is_move_motion();
    }

    auto actor_instance::is_waiting() -> bool {
        return is_wait_motion();
    }

    auto actor_instance::is_dead() -> bool {
        return is_real_dead_;
    }

    auto actor_instance::is_knock_down() -> bool {
        return is_knock_down_motion();
    }

    auto actor_instance::is_attacked() -> bool {
        if (is_pushing())
            return true;

        if (is_damage_motion())
            return true;

        if (is_knock_down_motion())
            return true;

        if (is_die_motion())
            return true;

        return false;
    }

    void actor_instance::physics_process() {
        physics_object_value_.update(second_elapsed_);
        add_movement(physics_object_value_.x_movement(), physics_object_value_.y_movement(), 0.0f);
    }

    void actor_instance::accumulation_movement_impl(float rot) {
        // The wait motion is skipped to prevent sliding. Race motion data should eventually carry
        // a flag telling whether a motion moves.
        if (race_motion_data::NAME_WAIT == current_motion_index())
            return;

        // The same step the animation advanced by, so the feet and the ground displacement agree.
        math::matrix rotation_z;
        rotation_z = math::matrix_rotation_z(math::to_radian(rot));
        update_transform(&rotation_z, second_elapsed_);

        add_movement(rotation_z._41, rotation_z._42, rotation_z._43);
    }

    void actor_instance::accumulation_movement() {
        if (tree_)
            return;

        if (horse_) {
            horse_->accumulation_movement_impl(fcur_rotation_);
            return;
        }

        accumulation_movement_impl(advancing_rotation_);
    }

    void actor_instance::transform_process() {
        if (!is_paralysis()) {
            x_ += movement_.x;
            y_ += movement_.y;
            z_ += movement_.z;
        }

        initialize_movement();

        set_position(x_, y_, z_);
    }

    void actor_instance::on_update_collision_data(const static_collision_data_vector *pscd_vector) {
        assert(pscd_vector);
        static_collision_data_vector::const_iterator it;
        for (it = pscd_vector->begin(); it != pscd_vector->end(); ++it) {
            const static_collision_data &colli_data = *it;
            const math::matrix &matrix = get_transform();
            add_collision(&colli_data, &matrix);
        }
    }

    void actor_instance::on_update_heigh_instance(attribute_instance *attribute_instance) {
        assert(attribute_instance);
        set_height_instance(attribute_instance);
    }

    auto actor_instance::on_get_object_height(float x, float y, float *height) -> bool {
        if (!height_attribute_instance_)
            return false;

        if (TYPE_BUILDING != get_type())
            return false;

        return height_attribute_instance_->height(x, y, height) == 1 ? true : false;
    }

    void actor_instance::revive() {
        is_sleep_ = false;
        is_paralysis_ = false;
        is_faint_ = false;
        is_real_dead_ = false;
        is_stun_ = false;
        is_walking_ = false;
        is_main_ = false;
        is_resist_fallen_ = false;

        initialize_collision_data();
    }

    auto actor_instance::is_stun() -> bool {
        return is_stun_;
    }

    void actor_instance::stun() {
        is_stun_ = true;
    }

    void actor_instance::set_walk_mode() {
        is_walking_ = true;
        if (race_motion_data::NAME_RUN == GET_MOTION_INDEX(cur_mot_node_.motion_key))
            set_loop_motion(race_motion_data::NAME_WALK, 0.15f, mov_spd_);
    }

    void actor_instance::set_run_mode() {
        is_walking_ = false;
        if (race_motion_data::NAME_WALK == GET_MOTION_INDEX(cur_mot_node_.motion_key))
            set_loop_motion(race_motion_data::NAME_RUN, 0.15f, mov_spd_);
    }

    auto actor_instance::normal_attack_index() -> MOTION_KEY {
        uint16_t motion_index;
        cur_race_data_->normal_attack_index(motion_mode(), &motion_index);

        return MAKE_MOTION_KEY(motion_mode(), motion_index);
    }

    void actor_instance::initialize_movement() {
        movement_.x = 0.0f;
        movement_.y = 0.0f;
        movement_.z = 0.0f;
    }

    void actor_instance::add_movement(float fx, float fy, float fz) {
        movement_.x += fx;
        movement_.y += fy;
        movement_.z += fz;
    }

    void actor_instance::adjust_dynamic_collision_movement(const actor_instance *actor_instance) {
        if (horse_) {
            horse_->adjust_dynamic_collision_movement(actor_instance);
            return;
        }

        uint32_t actor_type = actor_instance->actor_type();
        if ((actor_type == TYPE_BUILDING) || (actor_type == TYPE_OBJECT) || (actor_type == TYPE_DOOR) || (actor_type == TYPE_STONE)) {
            block_movement();
            return;
        }

        // Slide along other characters: only the part of the move that heads into a body is dropped,
        // so a character pressed by several monsters can still walk along or away from them.
        for (auto &main_point : body_point_instance_list_) {
            for (const dynamic_sphere_instance &main_sphere : main_point.sphere_instance_vector) {
                for (const auto &opp_point : actor_instance->body_point_instance_list_) {
                    if (opp_point.sphere_instance_vector.empty()) {
                        continue;
                    }
                    const dynamic_sphere_instance &opp_sphere = opp_point.sphere_instance_vector[0];
                    const float contact = main_sphere.radius + opp_sphere.radius;
                    const float ahead_x = main_sphere.last_position.x + movement_.x - opp_sphere.position_value.x;
                    const float ahead_y = main_sphere.last_position.y + movement_.y - opp_sphere.position_value.y;
                    if (ahead_x * ahead_x + ahead_y * ahead_y >= contact * contact) {
                        continue;
                    }
                    const float off_x = main_sphere.last_position.x - opp_sphere.position_value.x;
                    const float off_y = main_sphere.last_position.y - opp_sphere.position_value.y;
                    const float off_length = std::sqrt(off_x * off_x + off_y * off_y);
                    if (off_length < contact) {
                        continue;
                    }
                    const float toward = (movement_.x * off_x + movement_.y * off_y) / off_length;
                    if (toward >= 0.0f) {
                        continue;
                    }
                    movement_.x -= off_x / off_length * toward;
                    movement_.y -= off_y / off_length * toward;
                }
            }
        }

        for (auto &main_point : body_point_instance_list_) {
            for (dynamic_sphere_instance &main_sphere : main_point.sphere_instance_vector) {
                main_sphere.position_value = main_sphere.last_position + movement_;
            }
        }
    }

    const float gc_actor_slide_move_speed = 5.0f;

    void actor_instance::adjust_collision_movement(const graphic_object_instance *graphic_object_instance) {
        if (horse_) {
            horse_->adjust_collision_movement(graphic_object_instance);
            return;
        }

        // Assumes a single body.

        if (movement_.x == 0.0f && movement_.y == 0.0f && movement_.z == 0.0f)
            return;

        float move_length = math::length(movement_);
        if (move_length > gc_actor_slide_move_speed)
            movement_ *= gc_actor_slide_move_speed / move_length;

        auto it_main = body_point_instance_list_.begin();
        for (; it_main != body_point_instance_list_.end(); ++it_main) {
            dynamic_sphere_instance_vector &main_sphere_vector = (*it_main).sphere_instance_vector;
            for (uint32_t i = 0; i < main_sphere_vector.size(); ++i) {
                dynamic_sphere_instance &main_sphere = main_sphere_vector[i];

                math::vector3 delta = graphic_object_instance->collision_movement_adjust(main_sphere);
                movement_ += delta;
                main_sphere.position_value += delta;

                if (delta.x != 0.0f || delta.y != 0.0f || delta.z != 0.0f) {
                    move_length = math::length(movement_);
                    if (move_length > gc_actor_slide_move_speed) {
                        movement_ *= gc_actor_slide_move_speed / move_length;
                        main_sphere.position_value = main_sphere.last_position;
                        main_sphere.position_value += movement_;
                    }
                }
            }
        }
    }

    auto actor_instance::is_movement() -> bool {
        if (horse_)
            if (horse_->is_movement())
                return true;

        if (0.0f != movement_.x)
            return true;
        if (0.0f != movement_.y)
            return true;
        if (0.0f != movement_.z)
            return true;

        return false;
    }

    auto actor_instance::height() -> float {
        return graphic_thing_instance::height();
    }

    auto actor_instance::intersect_defending_sphere() -> bool {
        for (auto it = defending_point_instance_list_.begin(); it != defending_point_instance_list_.end(); ++it) {
            dynamic_sphere_instance_vector &sphere_instance_vector = (*it).sphere_instance_vector;

            auto it2 = sphere_instance_vector.begin();
            for (; it2 != sphere_instance_vector.end(); ++it2) {
                dynamic_sphere_instance &instance = *it2;
                math::vector3 sphere_position = instance.position_value;
                float radius = instance.radius;

                math::vector3 orig;
                math::vector3 dir;
                float range;
                ray_value_.start_point(&orig);
                ray_value_.get_direction(&dir, &range);

                math::vector3 distance = orig - sphere_position;
                float b = math::dot(dir, distance);
                float c = math::dot(distance, distance) - radius * radius;

                if (b * b - c >= 0)
                    return true;
            }
        }
        return false;
    }

    auto actor_instance::is_mounting_horse_impl() -> bool {
        return nullptr != horse_;
    }

    void actor_instance::mount_horse(actor_instance *horse) {
        horse_ = horse;

        if (horse_) {
            horse_->set_cur_pixel_position(new_get_cur_pixel_position_ref());
            horse_->set_rotation(rotation());
            horse_->set_advancing_rotation(rotation());
        }
    }

    void actor_instance::create_tree(const char *file_name) {
        destroy_tree();

        speed_tree_forest_direct_x8 &forest = get_speed_tree_forest_direct_x8();
        tree_ = forest.create_instance(x_, y_, z_, get_case_crc32(file_name, strlen(file_name)), file_name);
        tree_->set_position(x_, y_, z_);
        tree_->update_bounding_sphere();
        tree_->update_collision_data();
    }

    void actor_instance::destroy_tree() {
        if (!tree_)
            return;

        get_speed_tree_forest_direct_x8().delete_instance(tree_);
    }

    void actor_instance::set_tree_position(float fx, float fy, float fz) {
        if (!tree_)
            return;
        if (x_ == fx && y_ == fy && z_ == fz)
            return;

        tree_->set_position(fx, fy, fz);
        tree_->update_bounding_sphere();
        tree_->update_collision_data();
    }

    void actor_instance::clear_attaching_effect() {
        clear_attaching_effect_impl();
    }

    void actor_instance::destroy() {
        clear_fly_targeter();

        hit_data_map_value_.clear();
        motion_deque_value_.clear();

        if (attribute_instance_value_) {
            attribute_instance_value_->clear();
            attribute_instance::delete_value(attribute_instance_value_);
            attribute_instance_value_ = nullptr;
        }

        clear_attaching_effect_impl();

        graphic_thing_instance::clear();

        destroy_weapon_trace();
        destroy_tree();

        initialize_impl();
    }

    void actor_instance::initialize_rotation_data() {
        atk_dir_rot_ = 0.0f;
        fcur_rotation_ = 0.0f;
        rot_begin_ = 0.0f;
        rot_end_ = 0.0f;
        rot_end_time_ = 0.0f;
        rot_begin_time_ = 0.0f;
        rot_blend_time_ = 0.0f;
        advancing_rotation_ = 0.0f;
        rot_x_ = 0.0f;
        rot_y_ = 0.0f;
    }

    void actor_instance::initialize_state_data() {
        effect_initialized_ = false;

        is_pre_input_ = false;
        is_next_pre_input_ = false;

        is_sleep_ = false;
        is_paralysis_ = false;
        is_faint_ = false;
        is_real_dead_ = false;
        is_walking_ = false;
        is_main_ = false;
        is_stun_ = false;
        is_hiding_ = false;
        is_resist_fallen_ = false;

        render_mode_value_ = RENDER_MODE_NORMAL;
        alpha_value_ = 0.0f;
        add_color_ = math::color(0.0f, 0.0f, 0.0f, 1.0f);

        mtrl_color_ = 0xffffffff;
        mtrl_alpha_ = 0xff000000;

        battle_hit_effect_id_ = 0;
        battle_attach_effect_id_ = 0;
    }

    void actor_instance::initialize_motion_data() {
        wcur_motion_mode_ = race_motion_data::MODE_GENERAL;
        wcur_combo_type_ = 0;

        reach_scale_ = 1.0f;
        mov_spd_ = 1.0f;
        atk_spd_ = 1.0f;

        invisible_time_ = 0.0f;

        splash_area_value_.is_enable_hit_process = true;
        splash_area_value_.skill = 0;
        splash_area_value_.motion_key = 0;
        splash_area_value_.disappearing_time = 0.0f;
        splash_area_value_.sphere_instance_vector.clear();
        splash_area_value_.hitted_instance_map_value.clear();

        memset(&cur_mot_node_, 0, sizeof(cur_mot_node_));

        clear_combo();
    }

    void actor_instance::initialize_impl() {
        cur_race_motion_data_ = nullptr;
        cur_race_data_ = nullptr;
        horse_ = nullptr;
        tree_ = nullptr;

        owner_base_time_ = 0.0f;

        actor_type_ = TYPE_PC;
        race_ = 0;

        shape_value_ = 0;
        hair_value_ = 0;

        self_vid_ = 0;
        owner_vid_ = 0;

        event_handler_value_ = nullptr;

        physics_object_value_.initialize();

        attribute_instance_value_ = nullptr;

        fly_event_handler_value_ = nullptr;

        fishing_position_ = math::vector3(0.0f, 0.0f, 0.0f);
        fishing_effect_id_ = -1;

        horse_ = nullptr;

        initialize_position_data();
        initialize_rotation_data();
        initialize_motion_data();
        initialize_state_data();
        initialize_collision_data();

        blend_alpha_initialize();

        clear_fly_targeter();
    }

    actor_instance::actor_instance() {
        initialize_impl();
        physics_object_value_.set_actor_instance(this);
    }

    actor_instance::~actor_instance() {
        destroy();
    }
}
