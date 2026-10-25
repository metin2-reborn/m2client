#include <metin2/effects/effect_manager.h>
#include <metin2/log/log.h>

#include <metin2/core/crc32.h>
#include <metin2/core/text_file_loader.h>
#include <metin2/core/utils.h>
#include "flying_data.h"

namespace metin2::inline world::inline flying {

    dynamic_pool<flying_data> flying_data::pool_value;

    flying_data::flying_data() {
        initialize_impl();
    }

    void flying_data::initialize_impl() {
        ang_vel_ = math::vector3(0.0f, 0.0f, 0.0f);
        is_homing_ = false;
        init_vel_ = 200.0f;
        cone_angle_ = (0.0f);
        roll_angle_ = 0.0f;
        range_ = 500.0f;
        accel_ = math::vector3(0, 0, 0.0f); // gravity points along -z
        homing_max_angle_ = 3.0f;
        homing_start_time_ = 0.0f;
        gravity_ = 0;
        spreading_ = false;
        maintain_parallel_ = false;

        hit_on_background_ = false;
        hit_on_another_monster_ = false;
        pierce_count_ = 0;
        collision_sphere_radius_ = 0.0f;

        bomb_range_ = 10.0f;
        set_bomb_effect("");
    }

    void flying_data::destroy() {
        attach_data_vector_.clear();

        initialize_impl();
    }

    flying_data::~flying_data() {
        destroy();
    }

    void flying_data::set_bomb_effect(const char *effect_name_value) {
        bomb_effect_name_ = effect_name_value;
        if (bomb_effect_name_.empty()) {
            bomb_effect_id_ = 0;
            return;
        }
        string_path(bomb_effect_name_);
        if (get_effect_manager().register_effect(bomb_effect_name_.c_str()), true) {
            bomb_effect_id_ = get_case_crc32(bomb_effect_name_.c_str(), bomb_effect_name_.size());
        } else {
            bomb_effect_id_ = 0;
        }
    }

    auto flying_data::attach_data_reference(int index) -> flying_data::flying_attach_data & {
        return attach_data_vector_[index];
    }

    auto flying_data::load_script_file(const char *filename) -> bool {
        destroy();

        filename_ = filename;
        string_path(filename_);
        text_file_loader text_file_loader;
        if (!text_file_loader.load(filename_.c_str()))
            return false;

        text_file_loader.set_top();
        int temp;

        if (!text_file_loader.token_integer("spreadingflag", &temp)) {
            spreading_ = false;
        } else {
            spreading_ = temp ? true : false;
        }

        if (!text_file_loader.token_integer("maintainparallelflag", &temp)) {
            maintain_parallel_ = false;
        } else {
            maintain_parallel_ = temp ? true : false;
        }

        if (!text_file_loader.token_float("initialvelocity", &init_vel_)) {
            return false;
        }

        text_file_loader.token_float("coneangle", &cone_angle_);

        text_file_loader.token_float("rollangle", &roll_angle_);

        text_file_loader.token_vector3("angularvelocity", &ang_vel_);

        text_file_loader.token_float("gravity", &gravity_);

        if (text_file_loader.token_integer("hitonbackground", &temp))
            hit_on_background_ = temp ? true : false;
        else
            hit_on_background_ = false;

        if (text_file_loader.token_integer("hitonanothermonster", &temp))
            hit_on_another_monster_ = temp ? true : false;
        else
            hit_on_another_monster_ = false;

        if (!text_file_loader.token_integer("piercecount", &pierce_count_))
            pierce_count_ = 0;

        text_file_loader.token_float("bombrange", &bomb_range_);

        if (!text_file_loader.token_string("bombeffect", &bomb_effect_name_)) {
            bomb_effect_name_ = "";
        } else if (!bomb_effect_name_.empty()) {
            if (!is_global_file_name(bomb_effect_name_.c_str()))
                bomb_effect_name_ = get_only_path_name(text_file_loader.file_name()) + bomb_effect_name_;
            get_effect_manager().register_effect2(bomb_effect_name_.c_str(), &bomb_effect_id_);
        }

        if (!text_file_loader.token_integer("homingflag", &temp))
            is_homing_ = false;
        else
            is_homing_ = temp ? true : false;

        if (!text_file_loader.token_float("homingstarttime", &homing_start_time_))
            homing_start_time_ = 0.0f;

        if (!text_file_loader.token_float("homingmaxangle", &homing_max_angle_))
            homing_max_angle_ = 0.0f;

        if (!text_file_loader.token_float("range", &range_))
            return false;

        if (!text_file_loader.token_vector3("acceleration", &accel_))
            accel_ = math::vector3(0.0f, 0.0f, 0.0f);

        uint32_t i;
        for (i = 0; i < text_file_loader.child_node_count(); i++) {
            text_file_loader::goto_child goto_child(&text_file_loader, i);

            std::string node_name;

            text_file_loader.current_node_name(&node_name);

            if (node_name == "attachdata") {
                flying_attach_data fad;

                if (!text_file_loader.token_integer("type", &fad.type_value))
                    continue;
                text_file_loader.token_integer("flytype", &fad.fly_type);
                if (!text_file_loader.token_string("attachfile", &fad.filename)) {
                    fad.filename = "";
                } else if (!fad.filename.empty()) {
                    if (!is_global_file_name(fad.filename.c_str()))
                        fad.filename = get_only_path_name(text_file_loader.file_name()) + fad.filename;
                    get_effect_manager().register_effect(fad.filename.c_str());
                }

                text_file_loader.token_integer("tailflag", &temp);
                if (temp) {
                    fad.has_tail = true;
                    text_file_loader.token_double_word("tailcolor", &fad.tail_color);
                    text_file_loader.token_float("taillength", &fad.tail_length);
                    text_file_loader.token_float("tailsize", &fad.tail_size);
                    text_file_loader.token_integer("tailshaperect", &temp);
                    fad.rect_shape = temp ? true : false;
                } else {
                    fad.has_tail = false;
                }

                text_file_loader.token_float("roll", &fad.roll);
                text_file_loader.token_float("distance", &fad.distance);
                text_file_loader.token_float("period", &fad.period);
                text_file_loader.token_float("amplitude", &fad.amplitude);

                attach_data_vector_.push_back(fad);
            } else {
                metin2::log::error("Flying data {}: invalid data", filename_.c_str());
            }
        }

        return true;
    }
}
