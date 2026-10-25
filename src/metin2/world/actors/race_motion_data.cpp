#include "race_motion_data.h"
#include <metin2/core/mapped_file.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/files/asset_toml.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>
#include <metin2/world/flying/flying_object_manager.h>

namespace metin2::inline world::inline actors {

    dynamic_pool<race_motion_data> race_motion_data::pool_value_;

    void race_motion_data::create_system(uint32_t capacity) {
        pool_value_.create(capacity);
    }

    void race_motion_data::destroy_system() {
        pool_value_.clear();
    }

    auto race_motion_data::new_value() -> race_motion_data * {
        return pool_value_.alloc();
    }

    void race_motion_data::delete_value(race_motion_data *data) {
        data->destroy();

        pool_value_.free(data);
    }

    void race_motion_data::set_name(uint32_t name_value) {
        name_value_ = name_value;

        switch (name_value_) {
        case NAME_NONE:
            set_type(TYPE_NONE);
            break;
        case NAME_WAIT:
        case NAME_INTRO_WAIT:
        case NAME_STOP:
            set_type(TYPE_WAIT);
            break;
        case NAME_WALK:
        case NAME_RUN:
            set_type(TYPE_MOVE);
            break;
        case NAME_DAMAGE:
        case NAME_DAMAGE_BACK:
            set_type(TYPE_DAMAGE);
            break;
        case NAME_DAMAGE_FLYING:
        case NAME_DAMAGE_FLYING_BACK:
            set_type(TYPE_KNOCKDOWN);
            break;
        case NAME_STAND_UP:
        case NAME_STAND_UP_BACK:
            set_type(TYPE_STANDUP);
            break;
        case NAME_SPAWN:
        case NAME_CHANGE_WEAPON:
        case NAME_INTRO_SELECTED:
        case NAME_INTRO_NOT_SELECTED:
        case NAME_SPECIAL_1:
        case NAME_SPECIAL_2:
        case NAME_SPECIAL_3:
        case NAME_SPECIAL_4:
        case NAME_SPECIAL_5:
        case NAME_SPECIAL_6:
        case NAME_CLAP:
        case NAME_DANCE_1:
        case NAME_DANCE_2:
        case NAME_DANCE_3:
        case NAME_DANCE_4:
        case NAME_DANCE_5:
        case NAME_DANCE_6:
        case NAME_CONGRATULATION:
        case NAME_FORGIVE:
        case NAME_ANGRY:
        case NAME_ATTRACTIVE:
        case NAME_SAD:
        case NAME_SHY:
        case NAME_CHEERUP:
        case NAME_BANTER:
        case NAME_JOY:
        case NAME_CHEERS_1:
        case NAME_CHEERS_2:
        case NAME_KISS_WITH_WARRIOR:
        case NAME_KISS_WITH_ASSASSIN:
        case NAME_KISS_WITH_SURA:
        case NAME_KISS_WITH_SHAMAN:
        case NAME_FRENCH_KISS_WITH_WARRIOR:
        case NAME_FRENCH_KISS_WITH_ASSASSIN:
        case NAME_FRENCH_KISS_WITH_SURA:
        case NAME_FRENCH_KISS_WITH_SHAMAN:
        case NAME_SLAP_HIT_WITH_WARRIOR:
        case NAME_SLAP_HIT_WITH_ASSASSIN:
        case NAME_SLAP_HIT_WITH_SURA:
        case NAME_SLAP_HIT_WITH_SHAMAN:
        case NAME_SLAP_HURT_WITH_WARRIOR:
        case NAME_SLAP_HURT_WITH_ASSASSIN:
        case NAME_SLAP_HURT_WITH_SURA:
        case NAME_SLAP_HURT_WITH_SHAMAN:
        case NAME_DIG:
            set_type(TYPE_EVENT);
            break;
        case NAME_DEAD:
        case NAME_DEAD_BACK:
            set_type(TYPE_DIE);
            break;
        case NAME_NORMAL_ATTACK:
            set_type(TYPE_ATTACK);
            break;
        case NAME_COMBO_ATTACK_1:
        case NAME_COMBO_ATTACK_2:
        case NAME_COMBO_ATTACK_3:
        case NAME_COMBO_ATTACK_4:
        case NAME_COMBO_ATTACK_5:
        case NAME_COMBO_ATTACK_6:
        case NAME_COMBO_ATTACK_7:
        case NAME_COMBO_ATTACK_8:
            set_type(TYPE_COMBO);
            break;
        case NAME_FISHING_THROW:
        case NAME_FISHING_WAIT:
        case NAME_FISHING_REACT:
        case NAME_FISHING_CATCH:
        case NAME_FISHING_FAIL:
        case NAME_FISHING_STOP:
            set_type(TYPE_FISHING);
            break;
        default:
            if (name_value >= NAME_SKILL && name_value <= NAME_SKILL + static_cast<int>(SKILL_NUM))
                set_type(TYPE_SKILL);
            else
                metin2::log::warn("Unknown motion name {}", name_value);
            break;
        }
    }
    void race_motion_data::set_type(uint32_t type_value) {
        type_value_ = type_value;

        switch (type_value_) {
        case TYPE_ATTACK:
        case TYPE_COMBO:
        case TYPE_SKILL:
            is_lock_ = true;
            break;
        default:
            is_lock_ = false;
            break;
        }
    }

    auto race_motion_data::get_type() const -> uint32_t {
        return type_value_;
    }

    auto race_motion_data::is_lock() const -> bool {
        return is_lock_ ? true : false;
    }

    auto race_motion_data::get_loop_count() const -> int {
        return loop_count_;
    }

    auto race_motion_data::motion_duration() -> float {
        return motion_duration_;
    }

    auto race_motion_data::is_combo_input_time_data() const -> bool {
        return is_combo_motion_;
    }

    auto race_motion_data::combo_input_start_time() const -> float {
        assert(is_combo_motion_);
        return combo_input_data_value_.input_start_time;
    }

    auto race_motion_data::next_combo_time() const -> float {
        assert(is_combo_motion_);
        return combo_input_data_value_.next_combo_time;
    }

    auto race_motion_data::combo_input_end_time() const -> float {
        assert(is_combo_motion_);
        return combo_input_data_value_.input_end_time;
    }

    auto race_motion_data::is_attacking_motion() const -> bool {
        return is_attacking_motion_;
    }

    auto race_motion_data::motion_attack_data_pointer() const -> const race_types::motion_attack_data * {
        return &motion_attack_data_value_;
    }

    auto race_motion_data::has_splash_motion_event() const -> bool {
        return has_splash_event_;
    }

    auto race_motion_data::is_cancel_enable_skill() const -> bool {
        return cancel_enable_skill_;
    }

    auto race_motion_data::is_loop_motion() const -> bool {
        return is_loop_motion_;
    }

    auto race_motion_data::loop_start_time() const -> float {
        return loop_start_time_;
    }

    auto race_motion_data::loop_end_time() const -> float {
        return loop_end_time_;
    }

    auto race_motion_data::motion_event_data_count() const -> uint32_t {
        return motion_event_data_vector_value_.size();
    }

    auto race_motion_data::motion_event_data_pointer(uint8_t index, const race_motion_data::motion_event_data **data) const -> bool {
        if (index >= motion_event_data_vector_value_.size())
            return false;

        *data = motion_event_data_vector_value_[index];

        return true;
    }

    auto race_motion_data::get_event_type(uint32_t index) const -> int {
        if (index >= motion_event_data_vector_value_.size())
            return MOTION_EVENT_TYPE_NONE;

        return motion_event_data_vector_value_[index]->type_value;
    }

    auto race_motion_data::sound_instance_vector_pointer() const -> const sound_types::sound_instance_vector * {
        return &sound_instance_vector_value_;
    }

    void race_motion_data::set_accumulation_position(const pixel_position &pos) {
        accumulation_position_ = pos;
        is_accumulation_motion_ = true;
    }

    namespace {
        constexpr std::string_view event_type_names[] = {
            "", "Effect", "ScreenWaving", "", "SpecialAttack", "Sound", "Fly", "CharacterShow", "CharacterHide", "Warp", "EffectToTarget"};

        auto new_event(int type_value) -> race_motion_data::motion_event_data * {
            switch (type_value) {
            case race_motion_data::MOTION_EVENT_TYPE_EFFECT:
                return new race_motion_data::motion_effect_event_data;
            case race_motion_data::MOTION_EVENT_TYPE_SCREEN_WAVING:
                return new race_motion_data::screen_waving_event_data;
            case race_motion_data::MOTION_EVENT_TYPE_SPECIAL_ATTACKING:
                return new race_motion_data::motion_attacking_event_data;
            case race_motion_data::MOTION_EVENT_TYPE_SOUND:
                return new race_motion_data::motion_sound_event_data;
            case race_motion_data::MOTION_EVENT_TYPE_FLY:
                return new race_motion_data::motion_fly_event_data;
            case race_motion_data::MOTION_EVENT_TYPE_CHARACTER_SHOW:
                return new race_motion_data::motion_character_show_event_data;
            case race_motion_data::MOTION_EVENT_TYPE_CHARACTER_HIDE:
                return new race_motion_data::motion_character_hide_event_data;
            case race_motion_data::MOTION_EVENT_TYPE_WARP:
                return new race_motion_data::motion_warp_event_data;
            case race_motion_data::MOTION_EVENT_TYPE_EFFECT_TO_TARGET:
                return new race_motion_data::motion_effect_to_target_event_data;
            default:
                return nullptr;
            }
        }
    }

    auto race_motion_data::load(std::string_view text, std::string_view name) -> bool {
        const auto parsed = parse_asset_toml(text, name, "Motion");
        if (!parsed) {
            return false;
        }

        asset_table root(*parsed, name);
        root.table("asset");
        const toml::table *motion = root.table("motion");
        if (!motion) {
            root.error("", "[motion] is missing");
            return false;
        }
        asset_table motion_table(*motion, name);
        bool ok = motion_table.required("file", motion_file_name_);
        ok &= motion_table.required("duration", motion_duration_);
        if (motion->contains("accumulation")) {
            math::vector3 accumulation;
            if (motion_table.required("accumulation", accumulation)) {
                set_accumulation_position(accumulation);
            }
        }
        ok &= motion_table.finish();

        if (const toml::table *combo = root.table("combo")) {
            is_combo_motion_ = true;
            asset_table table(*combo, name);
            ok &= table.required("pre_input_time", combo_input_data_value_.input_start_time);
            ok &= table.required("direct_input_time", combo_input_data_value_.next_combo_time);
            ok &= table.required("input_limit_time", combo_input_data_value_.input_end_time);
            ok &= table.finish();
        }

        if (const toml::table *attack = root.table("attack")) {
            is_attacking_motion_ = true;
            asset_table table(*attack, name);
            ok &= race_types::load_motion_attack_data(table, &motion_attack_data_value_);
            ok &= table.finish();
        }

        if (const toml::table *loop = root.table("loop")) {
            is_loop_motion_ = true;
            asset_table table(*loop, name);
            loop_count_ = -1;
            ok &= table.optional("count", loop_count_);
            ok &= table.optional("cancel_enable", cancel_enable_skill_);
            ok &= table.required("start_time", loop_start_time_);
            ok &= table.required("end_time", loop_end_time_);
            ok &= table.finish();
        }

        const float frame_time_value = 1.0f / game_fps;
        stl_wipe(motion_event_data_vector_value_);
        if (const toml::array *events = root.array("event")) {
            for (const toml::node &node : *events) {
                const toml::table *event = node.as_table();
                if (!event) {
                    root.error("event", "must be tables ([[event]])");
                    return false;
                }
                asset_table table(*event, name);
                int type_value = MOTION_EVENT_TYPE_NONE;
                if (!table.required_enum("type", event_type_names, type_value)) {
                    return false;
                }
                motion_event_data *data = new_event(type_value);
                motion_event_data_vector_value_.push_back(data);
                data->type_value = type_value;
                ok &= table.required("time", data->starting_time);
                ok &= data->load(table);
                ok &= table.finish();
                data->frame = static_cast<uint32_t>(data->starting_time / frame_time_value);
                if (type_value == MOTION_EVENT_TYPE_SPECIAL_ATTACKING) {
                    has_splash_event_ = true;
                }
            }
        }

        return root.finish() && ok;
    }

    auto race_motion_data::load_motion_data(const char *file_name) -> bool {
        const std::string name = eter_file_manager::canonical_name(file_name);

        mapped_file file;
        const void *data = nullptr;
        if (!get_file_manager().get(file, name.c_str(), &data)) {
            return false;
        }
        if (!load(std::string_view(static_cast<const char *>(data), file.size()), name)) {
            return false;
        }

        for (motion_event_data *event : motion_event_data_vector_value_) {
            event->register_assets();
        }

        sound_script_data_file_name_ = eter_file_manager::sound_script_name(name);
        if (!sound_script_data_file_name_.empty()) {
            load_sound_script_data(sound_script_data_file_name_.c_str());
        }

        return true;
    }

    auto race_motion_data::load_sound_script_data(const char *file_name) -> bool {
        sound_types::sound_data_vector sound_data_vector;
        if (!sound_types::load_sound_information_piece(file_name, sound_data_vector)) {
            return false;
        }

        sound_types::data_to_instance(sound_data_vector, &sound_instance_vector_value_);
        return true;
    }

    auto race_motion_data::motion_file_name() const -> const char * {
        return motion_file_name_.c_str();
    }

    void race_motion_data::initialize() {
        loop_count_ = 0;
        motion_duration_ = 0.0f;
        accumulation_position_.x = 0.0f;
        accumulation_position_.y = 0.0f;
        accumulation_position_.z = 0.0f;
        loop_start_time_ = 0.0f;
        loop_end_time_ = 0.0f;

        is_accumulation_motion_ = false;
        is_combo_motion_ = false;
        is_loop_motion_ = false;
        is_attacking_motion_ = false;
        cancel_enable_skill_ = false;

        has_splash_event_ = false;

        is_lock_ = false;

        type_value_ = TYPE_NONE;
        name_value_ = NAME_NONE;

        motion_event_data_vector_value_.clear();
        sound_instance_vector_value_.clear();
    }

    void race_motion_data::destroy() {
        stl_wipe(motion_event_data_vector_value_);
        initialize();
    }

    race_motion_data::race_motion_data() {
        initialize();
    }

    race_motion_data::~race_motion_data() {
        destroy();
    }
}
