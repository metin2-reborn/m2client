#include <metin2/effects/effect_manager.h>
#include <metin2/log/log.h>

#include <metin2/core/crc32.h>
#include <metin2/core/mapped_file.h>
#include <metin2/core/utils.h>
#include <metin2/files/asset_toml.h>
#include <metin2/files/eter_file_manager.h>
#include "flying_data.h"

namespace metin2::inline world::inline flying {

    namespace {
        constexpr std::string_view attach_type_names[] = {"None", "Effect", "Object"};
        constexpr std::string_view fly_type_names[] = {"None", "Line", "MultiLine", "Sine", "Exp"};
    }

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

    auto flying_data::load(std::string_view text, std::string_view name) -> bool {
        const auto table = parse_asset_toml(text, name, "Fly");
        if (!table) {
            return false;
        }

        asset_table root(*table, name);
        root.table("asset");
        bool ok = true;

        if (const toml::table *fly = root.table("fly")) {
            asset_table values(*fly, name);
            ok &= values.optional("spreading", spreading_);
            ok &= values.optional("maintain_parallel", maintain_parallel_);
            ok &= values.required("initial_velocity", init_vel_);
            ok &= values.optional("cone_angle", cone_angle_);
            ok &= values.optional("roll_angle", roll_angle_);
            ok &= values.optional("angular_velocity", ang_vel_);
            ok &= values.optional("gravity", gravity_);
            ok &= values.optional("hit_on_background", hit_on_background_);
            ok &= values.optional("hit_on_another_monster", hit_on_another_monster_);
            ok &= values.optional("pierce_count", pierce_count_);
            ok &= values.optional("bomb_range", bomb_range_);
            ok &= values.optional("bomb_effect", bomb_effect_name_);
            ok &= values.optional("homing", is_homing_);
            ok &= values.optional("homing_start_time", homing_start_time_);
            ok &= values.optional("homing_max_angle", homing_max_angle_);
            ok &= values.required("range", range_);
            ok &= values.optional("acceleration", accel_);
            ok &= values.finish();
        } else {
            root.error("fly", "is missing");
            ok = false;
        }

        if (const toml::array *attachments = root.array("attach")) {
            for (const toml::node &node : *attachments) {
                const toml::table *attach_table = node.as_table();
                if (!attach_table) {
                    root.error("attach", "must be tables ([[attach]])");
                    ok = false;
                    continue;
                }
                asset_table values(*attach_table, name);
                flying_attach_data attach{};
                ok &= values.required_enum("type", attach_type_names, attach.type_value);
                ok &= values.required_enum("fly_type", fly_type_names, attach.fly_type);
                ok &= values.optional("file", attach.filename);
                ok &= values.optional("roll", attach.roll);
                ok &= values.optional("distance", attach.distance);
                ok &= values.optional("period", attach.period);
                ok &= values.optional("amplitude", attach.amplitude);
                if (const toml::table *tail = values.table("tail")) {
                    asset_table tail_values(*tail, name);
                    attach.has_tail = true;
                    ok &= tail_values.required("color", attach.tail_color);
                    ok &= tail_values.required("length", attach.tail_length);
                    ok &= tail_values.required("size", attach.tail_size);
                    ok &= tail_values.optional("rect", attach.rect_shape);
                    ok &= tail_values.finish();
                }
                attach_data_vector_.push_back(std::move(attach));
                ok &= values.finish();
            }
        }

        return root.finish() && ok;
    }

    auto flying_data::load_script_file(const char *filename) -> bool {
        destroy();

        filename_ = eter_file_manager::canonical_name(filename);
        string_path(filename_);

        mapped_file file;
        const void *data = nullptr;
        if (!get_file_manager().get(file, filename_.c_str(), &data) ||
            !load(std::string_view(static_cast<const char *>(data), file.size()), filename_)) {
            return false;
        }

        if (!bomb_effect_name_.empty()) {
            get_effect_manager().register_effect2(bomb_effect_name_.c_str(), &bomb_effect_id_);
        }
        for (const flying_attach_data &attach : attach_data_vector_) {
            if (!attach.filename.empty()) {
                get_effect_manager().register_effect(attach.filename.c_str());
            }
        }
        return true;
    }
}
