#include "instance_base.h"

#include "python_character_manager.h"
#include "python_non_player.h"

#include <metin2/client/app/abstract_application.h>
#include <metin2/client/app/abstract_player.h>
#include <metin2/client/background/python_background.h>
#include <metin2/client/player/python_player.h>
#include <metin2/client/protocol/packet.h>
#include <metin2/core/timer.h>
#include <metin2/files/eter_mark_manager.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>
#include <metin2/world/actors/game_util.h>
#include <metin2/world/items/item_manager.h>

#include <algorithm>
#include <cmath>

namespace metin2::inline client::inline characters {

    bool hair_color_enable = false;
    bool use_armor_specular = false;
    bool ride_horse_enable = true;
    const float c_fDefaultRotationSpeed = 1200.0f;
    const float c_fDefaultHorseRotationSpeed = 300.0f;

    auto is_wall(unsigned race) -> bool {
        switch (race) {
        case 14201:
        case 14202:
        case 14203:
        case 14204:
            return true;
            break;
        }
        return false;
    }

    instance_base::shorse::shorse() {
        initialize_impl();
    }

    instance_base::shorse::~shorse() {
        assert(actor == nullptr);
    }

    void instance_base::shorse::initialize_impl() {
        mounting = false;
        actor.reset();
    }

    void instance_base::shorse::set_attack_speed(uint32_t atk_spd) {
        if (!is_mounting())
            return;

        actor_instance &actor_value = actor_ref();
        actor_value.set_attack_speed(atk_spd / 100.0f);
    }

    void instance_base::shorse::set_move_speed(uint32_t mov_spd) {
        if (!is_mounting())
            return;

        actor_instance &actor_value = actor_ref();
        actor_value.set_move_speed(mov_spd / 100.0f);
    }

    void instance_base::shorse::create(const pixel_position &p_pos, uint32_t race, uint32_t hit_effect) {
        assert(nullptr == actor && "CInstanceBase::SHORSE::Create - ALREADY MOUNT");

        actor = std::make_unique<actor_instance>();

        actor_instance &actor_value = actor_ref();
        actor_value.set_event_handler(actor_instance::event_handler::empty_ptr());
        if (!actor_value.set_race(race)) {
            actor.reset();
            actor.reset();
            return;
        }

        actor_value.set_shape(0);
        actor_value.set_battle_hit_effect(hit_effect);
        actor_value.set_alpha_value(0.0f);
        actor_value.blend_alpha_value(1.0f, 0.5f);
        actor_value.set_move_speed(1.0f);
        actor_value.set_attack_speed(1.0f);
        actor_value.set_motion_mode(race_motion_data::MODE_GENERAL);
        actor_value.stop();
        actor_value.refresh_actor_instance();

        actor_value.set_cur_pixel_position(p_pos);

        mounting = true;
    }

    void instance_base::shorse::destroy() {
        if (actor) {
            actor->destroy();
            actor.reset();
        }

        initialize_impl();
    }

    auto instance_base::shorse::actor_ref() -> actor_instance & {
        assert(nullptr != actor && "CInstanceBase::SHORSE::GetActorRef");
        return *actor;
    }

    auto instance_base::shorse::actor_ptr() -> actor_instance * {
        return actor.get();
    }

    auto instance_base::shorse::get_level() -> uint32_t {
        if (actor) {
            uint32_t mount = actor->race();
            switch (mount) {
            case 20101:
            case 20102:
            case 20103:
                return 1;
            case 20104:
            case 20105:
            case 20106:
                return 2;
            case 20107:
            case 20108:
            case 20109:
            case 20110:
            case 20111:
            case 20112:
            case 20113:
            case 20114:
            case 20115:
            case 20116:
            case 20117:
            case 20118:
            case 20120:
            case 20121:
            case 20122:
            case 20123:
            case 20124:
            case 20125:
                return 3;
            case 20119: // Ramadan event black horse: level 2 (attack, no skills).
            case 20219: // Halloween event black horse, a clone of the Ramadan one.
            case 20220:
            case 20221:
            case 20222:
                return 2;
            }

            // Extended mount system (20201 to 20212, in groups of four: beginner, intermediate, advanced).
            {
                // intermediate mounts are level 2 (attack, no skills)
                if ((20205 <= mount && 20208 >= mount) ||
                    (20214 == mount) || (20217 == mount) || // wild armoured reindeer (male, female)
                    (20224 == mount) || (20229 == mount))
                    return 2;

                // advanced mounts are level 3 (attack and skills)
                if ((20209 <= mount && 20212 >= mount) ||
                    (20215 == mount) || (20218 == mount) || // brave armoured reindeer (male, female)
                    (20220 == mount) || (20225 == mount) || (20230 == mount))
                    return 3;
            }
        }
        return 0;
    }

    auto instance_base::shorse::is_new_mount() -> bool {
        if (!actor)
            return false;
        uint32_t mount = actor->race();

        if ((20205 <= mount && 20208 >= mount) ||
            (20214 == mount) || (20217 == mount) // wild armoured reindeer (male, female)
        )
            return true;

        // advanced mounts
        if ((20209 <= mount && 20212 >= mount) ||
            (20215 == mount) || (20218 == mount) || // brave armoured reindeer (male, female)
            (20220 == mount))
            return true;

        return false;
    }
    auto instance_base::shorse::can_use_skill() -> bool {
        // horse skills need a horse of level 3 or more.
        if (is_mounting())
            return 2 < get_level();

        return true;
    }

    auto instance_base::shorse::can_attack() -> bool {
        if (is_mounting())
            if (get_level() <= 1)
                return false;

        return true;
    }

    auto instance_base::shorse::is_mounting() -> bool {
        return mounting;
    }

    void instance_base::shorse::deform() {
        if (!is_mounting())
            return;

        actor_instance &actor_value = actor_ref();
        actor_value.instancebase_deform();
    }

    void instance_base::shorse::render() {
        if (!is_mounting())
            return;

        actor_instance &actor_value = actor_ref();
        actor_value.render();
    }

    void instance_base::attach_horse_saddle() {
        if (!is_mounting_horse())
            return;
        horse_.actor->attach_model_instance(race_data::PART_MAIN, "saddle", graphic_thing_instance_value_, race_data::PART_MAIN);
    }

    void instance_base::detach_horse_saddle() {
        if (!is_mounting_horse())
            return;
        horse_.actor->detach_model_instance(race_data::PART_MAIN, graphic_thing_instance_value_, race_data::PART_MAIN);
    }

    void instance_base::block_movement() {
        graphic_thing_instance_value_.block_movement();
    }

    auto instance_base::is_block_object(const graphic_object_instance &bg_obj) -> bool {
        return graphic_thing_instance_value_.is_block_object(bg_obj);
    }

    auto instance_base::avoid_object(const graphic_object_instance &bg_obj) -> bool {
        return graphic_thing_instance_value_.avoid_object(bg_obj);
    }

    auto armor_vnum_to_shape(int vnum, uint32_t *shape) -> bool {
        *shape = vnum;

        if (0 == vnum || 1 == vnum)
            return false;

        if (!use_armor_specular)
            return false;

        item_data *item_data;
        if (!get_item_manager().item_data_pointer(vnum, &item_data))
            return false;

        enum {
            SHAPE_VALUE_SLOT_INDEX = 3,
        };

        *shape = item_data->value(SHAPE_VALUE_SLOT_INDEX);

        return true;
    }

    class actor_instance_background : public background_interface {
      public:
        actor_instance_background() = default;
        ~actor_instance_background() = default;
        auto is_block(int x, int y) -> bool override {
            python_background &bg = get_python_background();
            return bg.is_attr_on(x, y, terrain_impl::ATTRIBUTE_BLOCK);
        }
    };

    auto instance_base::less_render_order(instance_base *inst) -> bool {
        int main_alpha = (alpha_value_impl() < 1.0f) ? 1 : 0;
        int test_alpha = (inst->alpha_value_impl() < 1.0f) ? 1 : 0;
        if (main_alpha < test_alpha)
            return true;
        if (main_alpha > test_alpha)
            return false;

        if (race() < inst->race())
            return true;
        if (race() > inst->race())
            return false;

        if (get_shape() < inst->get_shape())
            return true;

        if (get_shape() > inst->get_shape())
            return false;

        uint32_t left_lod_level = less_render_order_get_lod_level();
        uint32_t right_lod_level = inst->less_render_order_get_lod_level();
        if (left_lod_level < right_lod_level)
            return true;
        if (left_lod_level > right_lod_level)
            return false;

        if (part_value_[race_data::PART_WEAPON] < inst->part_value_[race_data::PART_WEAPON])
            return true;

        return false;
    }

    auto instance_base::less_render_order_get_lod_level() -> uint32_t {
        lod_controller *lod_ctrl = graphic_thing_instance_value_.lod_controller_pointer(0);
        if (!lod_ctrl)
            return 0;

        return lod_ctrl->lod_level();
    }

    auto instance_base::background_get_water_height(const pixel_position &p_pos, float *pf_height) -> bool {
        int32_t l_height;
        if (!get_python_background().water_height(int(p_pos.x), int(p_pos.y), &l_height))
            return false;

        *pf_height = float(l_height);

        return true;
    }

    auto instance_base::background_is_water_pixel_position(const pixel_position &p_pos) -> bool {
        return get_python_background().is_attr_on(p_pos.x, p_pos.y, terrain_impl::ATTRIBUTE_WATER);
    }

    const float PC_DUST_RANGE = 2000.0f;
    const float NPC_DUST_RANGE = 1000.0f;

    uint32_t instance_base::update_counter_ = 0;
    uint32_t instance_base::render_counter_ = 0;
    uint32_t instance_base::deform_counter_ = 0;

    dynamic_pool<instance_base> instance_base::pool_value;

    auto instance_base::is_in_dust_range() -> bool {
        if (!is_exist_main_instance())
            return false;

        instance_base *inst_main = main_instance_ptr_impl();

        float distance = new_get_distance_from_dest_instance(*inst_main);

        if (is_pc()) {
            if (distance <= PC_DUST_RANGE)
                return true;
        }

        if (distance <= NPC_DUST_RANGE)
            return true;

        return false;
    }

    void instance_base::enable_skip_collision_impl() {
        if (is_main_instance()) {
            metin2::log::error("The main actor must never skip collision checks");
            return;
        }
        graphic_thing_instance_value_.enable_skip_collision();
    }

    void instance_base::disable_skip_collision_impl() {
        graphic_thing_instance_value_.disable_skip_collision();
    }

    auto instance_base::shadow_map_color_impl(float x, float y) -> uint32_t {
        python_background &bg = get_python_background();
        return bg.shadow_map_color(x, y);
    }

    auto instance_base::background_height(float x, float y) -> float {
        python_background &bg = get_python_background();
        return bg.height(x, y);
    }

    auto instance_base::is_invisibility() -> bool {
        if (is_affect(AFFECT_INVISIBILITY))
            return true;

        return false;
    }

    auto instance_base::is_paralysis() -> bool {
        return graphic_thing_instance_value_.is_paralysis();
    }

    auto instance_base::is_game_master() -> bool {
        if (affect_flag_container_value_.is_set(AFFECT_YMIR))
            return true;
        return false;
    }

    auto instance_base::is_same_empire(instance_base &inst_dst) -> bool {
        if (0 == inst_dst.empire_id_)
            return true;

        if (is_game_master())
            return true;

        if (inst_dst.is_game_master())
            return true;

        if (inst_dst.empire_id_ == empire_id_)
            return true;

        return false;
    }

    auto instance_base::empire_id() -> uint32_t {
        return empire_id_;
    }

    auto instance_base::guild_id() -> uint32_t {
        return guild_id_;
    }

    auto instance_base::alignment() -> int {
        return alignment_;
    }

    auto instance_base::alignment_grade() -> uint32_t {
        if (alignment_ >= 12000)
            return 0;
        else if (alignment_ >= 8000)
            return 1;
        else if (alignment_ >= 4000)
            return 2;
        else if (alignment_ >= 1000)
            return 3;
        else if (alignment_ >= 0)
            return 4;
        else if (alignment_ > -4000)
            return 5;
        else if (alignment_ > -8000)
            return 6;
        else if (alignment_ > -12000)
            return 7;

        return 8;
    }

    auto instance_base::alignment_type() -> int {
        switch (alignment_grade()) {
        case 0:
        case 1:
        case 2:
        case 3:
            {
                return ALIGNMENT_TYPE_WHITE;
                break;
            }

        case 5:
        case 6:
        case 7:
        case 8:
            {
                return ALIGNMENT_TYPE_DARK;
                break;
            }
        }

        return ALIGNMENT_TYPE_NORMAL;
    }

    auto instance_base::pk_mode() -> uint8_t {
        return pk_mode_;
    }

    auto instance_base::is_killer() -> bool {
        return is_killer_;
    }

    auto instance_base::is_party_member() -> bool {
        return is_party_member_;
    }

    auto instance_base::is_in_safe() -> bool {
        const pixel_position &p_pos_cur = graphic_thing_instance_value_.new_get_cur_pixel_position_ref();
        if (get_python_background().is_attr_on(p_pos_cur.x, p_pos_cur.y, terrain_impl::ATTRIBUTE_BANPK))
            return true;

        return false;
    }

    auto instance_base::calculate_distance_sq3d(const pixel_position &p_pos_dst) -> float {
        const pixel_position &p_pos_src = graphic_thing_instance_value_.new_get_cur_pixel_position_ref();
        return s_pixel_position_calculate_distance_sq3d(p_pos_src, p_pos_dst);
    }

    void instance_base::on_selected() {

        if (is_stone_door())
            return;

        if (is_dead())
            return;

        attach_select_effect();
    }

    void instance_base::on_unselected() {
        detach_select_effect();
    }

    void instance_base::on_targeted() {

        if (is_stone_door())
            return;

        if (is_dead())
            return;

        attach_target_effect();
    }

    void instance_base::on_untargeted() {
        detach_target_effect();
    }

    void instance_base::destroy_system() {
        pool_value.clear();
    }

    void instance_base::create_system(uint32_t capacity) {
        pool_value.create(capacity);

        memset(affect_effect_crcs_, 0, sizeof(affect_effect_crcs_));

        dust_gap_ = 250.0f;
        horse_dust_gap_ = 500.0f;
    }

    auto instance_base::new_value() -> instance_base * {
        return pool_value.alloc();
    }

    void instance_base::delete_value(instance_base *inst) {
        inst->destroy();
        pool_value.free(inst);
    }

    void instance_base::set_main_instance() {
        python_character_manager &chr_mgr = get_python_character_manager();

        uint32_t vid = virtual_id();
        chr_mgr.set_main_instance(vid);

        graphic_thing_instance_value_.set_main_instance();
    }

    auto instance_base::main_instance_ptr_impl() -> instance_base * {
        python_character_manager &chr_mgr = get_python_character_manager();
        return chr_mgr.main_instance_ptr();
    }

    void instance_base::clear_main_instance_impl() {
        python_character_manager &chr_mgr = get_python_character_manager();
        chr_mgr.clear_main_instance();
    }

    // The character controlled by this client.
    auto instance_base::is_main_instance() -> bool {
        if (this == main_instance_ptr_impl())
            return true;

        return false;
    }

    auto instance_base::is_exist_main_instance() -> bool {
        if (main_instance_ptr_impl())
            return true;
        else
            return false;
    }

    auto instance_base::main_can_see_hidden_thing() -> bool {
        return false;
    }

    auto instance_base::bow_range() -> float {
        float range = 2500.0f - 100.0f;

        if (is_main_instance()) {
            abstract_player &player = get_abstract_player();
            range += float(player.status(POINT_BOW_DISTANCE));
        }

        return range;
    }

    auto instance_base::find_race_type(uint32_t race, uint8_t *type_value) -> bool {
        python_non_player &non_player = get_python_non_player();
        return non_player.instance_type(race, type_value);
    }

    auto instance_base::create(const create_data &create_data_value) -> bool {
        get_abstract_application().skip_render_buffering(300);

        set_instance_type(create_data_value.type_value);

        if (!set_race(create_data_value.race))
            return false;

        set_virtual_id(create_data_value.vid);

        if (create_data_value.is_main)
            set_main_instance();

        if (is_guild_wall()) {
            unsigned center_x;
            unsigned center_y;

            create_data_value.affect_flags.convert_to_position(&center_x, &center_y);

            float center_z = background_height(center_x, center_y);
            new_set_pixel_position(pixel_position(float(create_data_value.pos_x), float(create_data_value.pos_y), center_z));
        } else {
            script_set_pixel_position(float(create_data_value.pos_x), float(create_data_value.pos_y));
        }

        if (0 != create_data_value.mount_vnum)
            mount_horse(create_data_value.mount_vnum);

        set_armor(create_data_value.armor);

        if (is_pc()) {
            set_hair(create_data_value.hair_value);
            set_weapon(create_data_value.weapon);
        }

        create_set_name(create_data_value);

        level_value_ = create_data_value.level_value;
        guild_id_ = create_data_value.guild_id;
        empire_id_ = create_data_value.empire_id;

        set_virtual_number(create_data_value.race);
        set_rotation(create_data_value.rot);

        set_alignment(create_data_value.alignment);
        set_pk_mode(create_data_value.pk_mode);

        set_move_speed(create_data_value.mov_spd);
        set_attack_speed(create_data_value.atk_spd);

        // No alpha while wearing a dress.
        if (!is_wearing_dress()) {
            // Must come before the affects are set.
            graphic_thing_instance_value_.set_alpha_value(0.0f);
            graphic_thing_instance_value_.blend_alpha_value(1.0f, 0.5f);
        }

        if (!is_guild_wall()) {
            set_affect_flag_container(create_data_value.affect_flags);
        }

        // Must come after the affects are set.
        attach_text_tail();
        refresh_text_tail();

        if (create_data_value.state_flags & ADD_CHARACTER_STATE_SPAWN) {
            if (is_affect(AFFECT_SPAWN))
                attach_effect(EFFECT_SPAWN_APPEAR);

            if (is_pc()) {
                refresh(race_motion_data::NAME_WAIT, true);
            } else {
                refresh(race_motion_data::NAME_SPAWN, false);
            }
        } else {
            refresh(race_motion_data::NAME_WAIT, true);
        }

        attach_empire_effect(create_data_value.empire_id);

        register_bounding_sphere();

        if (create_data_value.state_flags & ADD_CHARACTER_STATE_DEAD)
            graphic_thing_instance_value_.die_end();

        set_state_flags(create_data_value.state_flags);

        graphic_thing_instance_value_.set_battle_hit_effect(affect_effect_crcs_[EFFECT_HIT]);

        if (!is_pc()) {
            uint32_t body_color = get_python_non_player().monster_color(create_data_value.race);
            if (0 != body_color) {
                set_modulate_render_mode();
                set_add_color(body_color);
            }
        }

        attach_horse_saddle();

        // TODO: temporary guild symbol code; the right position is still unknown.
        const int guild_symbol_race = 14200;
        if (guild_symbol_race == race()) {
            std::string file_name = get_guild_symbol_file_name(guild_id_);
            if (get_mark_manager().is_exist(file_name.c_str()))
                graphic_thing_instance_value_.change_material(file_name.c_str());
        }

        return true;
    }

    void instance_base::create_set_name(const create_data &create_data_value) {
        if (is_goto()) {
            set_name_string("", 0);
            return;
        }
        if (is_warp()) {
            create_set_warp_name(create_data_value);
            return;
        }

        set_name_string(create_data_value.name_value.c_str(), create_data_value.name_value.length());
    }

    void instance_base::create_set_warp_name(const create_data &create_data_value) {
        const char *c_sz_name;
        if (get_python_non_player().get_name(create_data_value.race, &c_sz_name)) {
            std::string str_name = c_sz_name;
            int finding_pos = str_name.find_first_of(" ", 0);
            if (finding_pos > 0) {
                str_name.resize(finding_pos);
            }
            set_name_string(str_name.c_str(), str_name.length());
        } else {
            set_name_string(create_data_value.name_value.c_str(), create_data_value.name_value.length());
        }
    }

    void instance_base::set_name_string(const char *name, int len) {
        name_value_.assign(name, len);
    }

    auto instance_base::set_race(uint32_t race) -> bool {
        race_ = race;

        if (!graphic_thing_instance_value_.set_race(race))
            return false;

        if (!find_race_type(race_, &race_type_))
            race_type_ = actor_instance::TYPE_PC;

        return true;
    }

    auto instance_base::is_changable_weapon(int weapon_id) -> bool {
        // A dress allows only the bouquets, no other weapons.
        if (is_wearing_dress()) {
            const int bouquets[] =
                {
                    50201, // Bouquet for Assassin
                    50202, // Bouquet for Shaman
                    50203,
                    50204,
                    0,
            };

            for (int i = 0; bouquets[i] != 0; ++i)
                if (weapon_id == bouquets[i])
                    return true;

            return false;
        } else
            return true;
    }

    auto instance_base::is_wearing_dress() -> bool {
        const int wedding_dress_shape = 201;
        return wedding_dress_shape == shape_value_;
    }

    auto instance_base::is_holding_pick_axe() -> bool {
        const int pick_axe_start = 29101;
        const int pick_axe_end = 29110;
        return part_value_[race_data::PART_WEAPON] >= pick_axe_start && part_value_[race_data::PART_WEAPON] <= pick_axe_end;
    }

    auto instance_base::is_new_mount() -> bool {
        return horse_.is_new_mount();
    }

    auto instance_base::is_mounting_horse() -> bool {
        return horse_.is_mounting();
    }

    void instance_base::mount_horse(uint32_t race) {
        horse_.destroy();
        horse_.create(graphic_thing_instance_value_.new_get_cur_pixel_position_ref(), race, affect_effect_crcs_[EFFECT_HIT]);

        set_motion_mode(race_motion_data::MODE_HORSE);
        set_rotation_speed(c_fDefaultHorseRotationSpeed);

        graphic_thing_instance_value_.mount_horse(horse_.actor_ptr());
        graphic_thing_instance_value_.stop();
        graphic_thing_instance_value_.refresh_actor_instance();
    }

    void instance_base::dismount_horse() {
        horse_.destroy();
    }

    void instance_base::get_info(std::string *pst_info) {
        char sz_info[256];
        snprintf(sz_info, sizeof(sz_info), "Inst - UC %d, RC %d Pool - %d ",
            update_counter_,
            render_counter_,
            pool_value.capacity());

        pst_info->append(sz_info);
    }

    void instance_base::reset_performance_counter() {
        update_counter_ = 0;
        render_counter_ = 0;
        deform_counter_ = 0;
    }

    auto instance_base::new_get_last_pixel_position_ref() -> const pixel_position & {
        return graphic_thing_instance_value_.new_get_last_pixel_position_ref();
    }

    void instance_base::new_set_dst_pixel_position_z(float z) {
        graphic_thing_instance_value_.new_set_dst_pixel_position_z(z);
    }

    void instance_base::new_set_dst_pixel_position(const pixel_position &p_pos_dst) {
        graphic_thing_instance_value_.new_set_dst_pixel_position(p_pos_dst);
    }

    void instance_base::new_set_src_pixel_position(const pixel_position &p_pos_src) {
        graphic_thing_instance_value_.new_set_src_pixel_position(p_pos_src);
    }

    auto instance_base::new_get_cur_pixel_position_ref() -> const pixel_position & {
        return graphic_thing_instance_value_.new_get_cur_pixel_position_ref();
    }

    auto instance_base::new_get_dst_pixel_position_ref() -> const pixel_position & {
        return graphic_thing_instance_value_.new_get_dst_pixel_position_ref();
    }

    auto instance_base::new_get_src_pixel_position_ref() -> const pixel_position & {
        return graphic_thing_instance_value_.new_get_src_pixel_position_ref();
    }

    void instance_base::on_syncing() {
        graphic_thing_instance_value_.on_syncing_impl();
    }

    void instance_base::on_waiting() {
        graphic_thing_instance_value_.on_waiting_impl();
    }

    void instance_base::on_moving() {
        graphic_thing_instance_value_.on_moving_impl();
    }

    void instance_base::change_guild(uint32_t guild_id) {
        guild_id_ = guild_id;

        detach_text_tail();
        attach_text_tail();
        refresh_text_tail();
    }

    auto instance_base::get_part(race_data::parts part) -> uint32_t {
        assert(part >= 0 && part < race_data::PART_MAX_NUM);
        return part_value_[part];
    }

    auto instance_base::get_shape() -> uint32_t {
        return shape_value_;
    }

    auto instance_base::can_act() -> bool {
        return graphic_thing_instance_value_.can_act();
    }

    auto instance_base::can_move() -> bool {
        return graphic_thing_instance_value_.can_move();
    }

    auto instance_base::can_use_skill() -> bool {
        if (is_poly())
            return false;

        if (is_wearing_dress())
            return false;

        if (is_holding_pick_axe())
            return false;

        if (!horse_.can_use_skill())
            return false;

        if (!graphic_thing_instance_value_.can_use_skill())
            return false;

        return true;
    }

    auto instance_base::can_attack() -> bool {
        if (!horse_.can_attack())
            return false;

        if (is_wearing_dress())
            return false;

        if (is_holding_pick_axe())
            return false;

        return graphic_thing_instance_value_.can_attack();
    }

    auto instance_base::can_fishing() -> bool {
        return graphic_thing_instance_value_.can_fishing();
    }

    auto instance_base::is_bow_mode() -> bool {
        return graphic_thing_instance_value_.is_bow_mode();
    }

    auto instance_base::is_hand_mode() -> bool {
        return graphic_thing_instance_value_.is_hand_mode();
    }

    auto instance_base::is_fishing_mode() -> bool {
        if (race_motion_data::MODE_FISHING == graphic_thing_instance_value_.motion_mode())
            return true;

        return false;
    }

    auto instance_base::is_fishing() -> bool {
        return graphic_thing_instance_value_.is_fishing();
    }

    auto instance_base::is_dead() -> bool {
        return graphic_thing_instance_value_.is_dead();
    }

    auto instance_base::is_stun() -> bool {
        return graphic_thing_instance_value_.is_stun();
    }

    auto instance_base::is_sleep() -> bool {
        return graphic_thing_instance_value_.is_sleep();
    }

    auto instance_base::is_syncing() -> bool {
        return graphic_thing_instance_value_.is_syncing();
    }

    void instance_base::new_set_owner(uint32_t vid_owner) {
        graphic_thing_instance_value_.set_owner(vid_owner);
    }

    auto instance_base::local_time() -> float {
        return graphic_thing_instance_value_.local_time();
    }

    void instance_base::push_tcp_state_expanded(uint32_t cmd_time, const pixel_position &p_pos_dst, float dst_rot, uint32_t func, uint32_t arg, uint32_t target_vid) {
        command cmd_new;
        cmd_new.p_pos_dst = p_pos_dst;
        cmd_new.chk_time = cmd_time + 100;
        cmd_new.cmd_time = cmd_time;
        cmd_new.dst_rot = dst_rot;
        cmd_new.func = func;
        cmd_new.arg_value = arg;
        cmd_new.target_vid = target_vid;
        que_cmd_new_.push_back(cmd_new);
    }

    void instance_base::push_tcp_state(uint32_t cmd_time, const pixel_position &p_pos_dst, float dst_rot, uint32_t func, uint32_t arg) {
        if (is_main_instance()) {
            metin2::log::error("The main actor must never receive a move packet");
            return;
        }

        int network_gap = el_timer_get_server_frame_m_sec() - cmd_time;

        average_network_gap_ = (average_network_gap_ * 70 + network_gap * 30) / 100;

        command cmd_new;
        cmd_new.p_pos_dst = p_pos_dst;
        cmd_new.chk_time = cmd_time + average_network_gap_;
        cmd_new.cmd_time = cmd_time;
        cmd_new.dst_rot = dst_rot;
        cmd_new.func = func;
        cmd_new.arg_value = arg;
        que_cmd_new_.push_back(cmd_new);
    }

    auto instance_base::can_process_network_state_packet() -> bool {
        if (graphic_thing_instance_value_.is_dead())
            return false;
        if (graphic_thing_instance_value_.is_knock_down())
            return false;
        if (graphic_thing_instance_value_.is_using_skill())
            if (!graphic_thing_instance_value_.can_cancel_skill())
                return false;

        return true;
    }

    auto instance_base::is_enable_tcp_process(uint32_t cur_func) -> bool {
        if (graphic_thing_instance_value_.is_act_emotion()) {
            return false;
        }

        if (!enable_tcp_state_) {
            if (FUNC_EMOTION != cur_func) {
                return false;
            }
        }

        return true;
    }

    void instance_base::state_process() {
        while (1) {
            if (que_cmd_new_.empty())
                return;

            uint32_t dst_chk_time = que_cmd_new_.front().chk_time;
            uint32_t cur_chk_time = el_timer_get_server_frame_m_sec();

            if (cur_chk_time < dst_chk_time)
                return;

            command cmd_top = que_cmd_new_.front();
            que_cmd_new_.pop_front();

            pixel_position p_pos_dst = cmd_top.p_pos_dst;
            float rot_dst = cmd_top.dst_rot;
            uint32_t func = cmd_top.func;
            uint32_t arg = cmd_top.arg_value;
            uint32_t vid = virtual_id();
            uint32_t target_vid = cmd_top.target_vid;

            pixel_position p_pos_cur;
            new_get_pixel_position(&p_pos_cur);

            pixel_position p_pos_dir = p_pos_dst - p_pos_cur;
            auto dir_len = (float)sqrt(p_pos_dir.x * p_pos_dir.x + p_pos_dir.y * p_pos_dir.y);

            if (!can_process_network_state_packet()) {
                metin2::log::debug("Character (vid {}) cannot move, skipped (dead {}, knocked down {})", vid, graphic_thing_instance_value_.is_dead(), graphic_thing_instance_value_.is_knock_down());
                return;
            }

            if (!is_enable_tcp_process(func)) {
                return;
            }

            switch (func) {
            case FUNC_WAIT:
                {
                    if (dir_len > 1.0f) {
                        new_set_src_pixel_position(p_pos_cur);
                        new_set_dst_pixel_position(p_pos_dst);

                        enable_skip_collision_impl();

                        dst_rot_ = rot_dst;
                        is_going_ = true;

                        mov_after_func_.func = FUNC_WAIT;

                        if (!is_walking())
                            start_walking();

                    } else {

                        is_going_ = false;

                        if (!is_waiting())
                            end_walking();

                        script_set_pixel_position(p_pos_dst.x, p_pos_dst.y);
                        set_advancing_rotation(rot_dst);
                        set_rotation(rot_dst);
                    }
                    break;
                }

            case FUNC_MOVE:
                {
                    new_set_src_pixel_position(p_pos_cur);
                    new_set_dst_pixel_position(p_pos_dst);
                    dst_rot_ = rot_dst;
                    is_going_ = true;
                    enable_skip_collision_impl();

                    mov_after_func_.func = FUNC_MOVE;

                    if (!is_walking()) {
                        start_walking();
                    } else {
                    }
                    break;
                }

            case FUNC_COMBO:
                {
                    if (dir_len >= 50.0f) {
                        new_set_src_pixel_position(p_pos_cur);
                        new_set_dst_pixel_position(p_pos_dst);
                        dst_rot_ = rot_dst;
                        is_going_ = true;
                        enable_skip_collision_impl();

                        mov_after_func_.func = FUNC_COMBO;
                        mov_after_func_.arg_value = arg;

                        if (!is_walking())
                            start_walking();
                    } else {

                        is_going_ = false;

                        if (is_walking())
                            end_walking();

                        script_set_pixel_position(p_pos_dst.x, p_pos_dst.y);
                        run_combo_attack(rot_dst, arg);
                    }
                    break;
                }

            case FUNC_ATTACK:
                {
                    if (dir_len >= 50.0f) {
                        new_set_src_pixel_position(p_pos_cur);
                        new_set_dst_pixel_position(p_pos_dst);
                        dst_rot_ = rot_dst;
                        is_going_ = true;
                        enable_skip_collision_impl();

                        mov_after_func_.func = FUNC_ATTACK;

                        if (!is_walking())
                            start_walking();

                    } else {

                        is_going_ = false;

                        if (is_walking())
                            end_walking();

                        script_set_pixel_position(p_pos_dst.x, p_pos_dst.y);
                        blend_rotation(rot_dst);

                        run_normal_attack(rot_dst);
                    }
                    break;
                }

            case FUNC_MOB_SKILL:
                {
                    if (dir_len >= 50.0f) {
                        new_set_src_pixel_position(p_pos_cur);
                        new_set_dst_pixel_position(p_pos_dst);
                        dst_rot_ = rot_dst;
                        is_going_ = true;
                        enable_skip_collision_impl();

                        mov_after_func_.func = FUNC_MOB_SKILL;
                        mov_after_func_.arg_value = arg;

                        if (!is_walking())
                            start_walking();
                    } else {
                        is_going_ = false;

                        if (is_walking())
                            end_walking();

                        script_set_pixel_position(p_pos_dst.x, p_pos_dst.y);
                        blend_rotation(rot_dst);

                        graphic_thing_instance_value_.intercept_once_motion(race_motion_data::NAME_SPECIAL_1 + arg);
                    }
                    break;
                }

            case FUNC_EMOTION:
                {
                    if (dir_len > 100.0f) {
                        new_set_src_pixel_position(p_pos_cur);
                        new_set_dst_pixel_position(p_pos_dst);
                        dst_rot_ = rot_dst;
                        is_going_ = true;

                        if (is_main_instance())
                            enable_skip_collision_impl();

                        mov_after_func_.func = FUNC_EMOTION;
                        mov_after_func_.arg_value = arg;
                        mov_after_func_.arg_expanded = target_vid;
                        mov_after_func_.pos_dst = p_pos_dst;

                        if (!is_walking())
                            start_walking();
                    } else {
                        process_function_emotion(arg, target_vid, p_pos_dst);
                    }
                    break;
                }

            default:
                {
                    if (func & FUNC_SKILL) {
                        if (dir_len >= 50.0f) {
                            new_set_src_pixel_position(p_pos_cur);
                            new_set_dst_pixel_position(p_pos_dst);
                            dst_rot_ = rot_dst;
                            is_going_ = true;
                            enable_skip_collision_impl();

                            mov_after_func_.func = func;
                            mov_after_func_.arg_value = arg;

                            if (!is_walking())
                                start_walking();

                        } else {

                            is_going_ = false;

                            if (is_walking())
                                end_walking();

                            script_set_pixel_position(p_pos_dst.x, p_pos_dst.y);
                            set_advancing_rotation(rot_dst);
                            set_rotation(rot_dst);

                            new_use_skill(0, func & 0x7f, arg & 0x0f, (arg >> 4) ? true : false);
                        }
                    }
                    break;
                }
            }
        }
    }

    void instance_base::movement_process() {
        pixel_position p_pos_cur;
        new_get_pixel_position(&p_pos_cur);

        // The movement vector is in rendering coordinates, so y is negated before adding.

        pixel_position p_pos_next;
        {
            const math::vector3 &v3_mov = graphic_thing_instance_value_.movement_vector_ref();

            p_pos_next.x = p_pos_cur.x + (+v3_mov.x);
            p_pos_next.y = p_pos_cur.y + (-v3_mov.y);
            p_pos_next.z = p_pos_cur.z + (+v3_mov.z);
        }

        pixel_position p_pos_delta_sc = p_pos_cur - new_get_src_pixel_position_ref();
        pixel_position p_pos_delta_sn = p_pos_next - new_get_src_pixel_position_ref();
        pixel_position p_pos_delta_sd = new_get_dst_pixel_position_ref() - new_get_src_pixel_position_ref();

        float cur_len = sqrtf(p_pos_delta_sc.x * p_pos_delta_sc.x + p_pos_delta_sc.y * p_pos_delta_sc.y);
        float next_len = sqrtf(p_pos_delta_sn.x * p_pos_delta_sn.x + p_pos_delta_sn.y * p_pos_delta_sn.y);
        float total_len = sqrtf(p_pos_delta_sd.x * p_pos_delta_sd.x + p_pos_delta_sd.y * p_pos_delta_sd.y);
        float rest_len = total_len - cur_len;

        if (is_main_instance()) {
            if (is_going_ && is_walking()) {
                float dst_rot = new_get_advancing_rotation_from_pixel_position(new_get_src_pixel_position_ref(), new_get_dst_pixel_position_ref());

                set_advancing_rotation(dst_rot);

                if (rest_len <= 0.0) {
                    if (is_walking())
                        end_walking();

                    is_going_ = false;

                    block_movement();

                    if (FUNC_EMOTION == mov_after_func_.func) {
                        uint32_t motion_number = mov_after_func_.arg_value;
                        uint32_t target_vid = mov_after_func_.arg_expanded;
                        process_function_emotion(motion_number, target_vid, mov_after_func_.pos_dst);
                        mov_after_func_.func = FUNC_WAIT;
                        return;
                    }
                }
            }
        } else {
            if (is_going_ && is_walking()) {
                float dst_rot = new_get_advancing_rotation_from_pixel_position(new_get_src_pixel_position_ref(), new_get_dst_pixel_position_ref());

                set_advancing_rotation(dst_rot);

                // High latency carried it past the destination.
                if (rest_len < -100.0f) {
                    new_set_src_pixel_position(p_pos_cur);

                    float dst_rot = new_get_advancing_rotation_from_pixel_position(p_pos_cur, new_get_dst_pixel_position_ref());
                    set_advancing_rotation(dst_rot);

                    if (FUNC_MOVE == mov_after_func_.func) {
                        mov_after_func_.func = FUNC_WAIT;
                    }
                } else if (cur_len <= total_len && total_len <= next_len) {
                    if (graphic_thing_instance_value_.is_dead() || graphic_thing_instance_value_.is_knock_down()) {
                        disable_skip_collision_impl();

                        is_going_ = false;

                    } else {
                        switch (mov_after_func_.func) {
                        case FUNC_ATTACK:
                            {
                                if (is_walking())
                                    end_walking();

                                disable_skip_collision_impl();
                                is_going_ = false;

                                block_movement();
                                script_set_pixel_position(new_get_dst_pixel_position_ref().x, new_get_dst_pixel_position_ref().y);
                                set_advancing_rotation(dst_rot_);
                                set_rotation(dst_rot_);

                                run_normal_attack(dst_rot_);
                                break;
                            }

                        case FUNC_COMBO:
                            {
                                if (is_walking())
                                    end_walking();

                                disable_skip_collision_impl();
                                is_going_ = false;

                                block_movement();
                                script_set_pixel_position(new_get_dst_pixel_position_ref().x, new_get_dst_pixel_position_ref().y);
                                run_combo_attack(dst_rot_, mov_after_func_.arg_value);
                                break;
                            }

                        case FUNC_EMOTION:
                            {
                                is_going_ = false;
                                mov_after_func_.func = FUNC_WAIT;
                                disable_skip_collision_impl();
                                block_movement();

                                uint32_t motion_number = mov_after_func_.arg_value;
                                uint32_t target_vid = mov_after_func_.arg_expanded;
                                process_function_emotion(motion_number, target_vid, mov_after_func_.pos_dst);
                                break;
                            }

                        case FUNC_MOVE:
                            {
                                break;
                            }

                        case FUNC_MOB_SKILL:
                            {
                                if (is_walking())
                                    end_walking();

                                disable_skip_collision_impl();
                                is_going_ = false;

                                block_movement();
                                script_set_pixel_position(new_get_dst_pixel_position_ref().x, new_get_dst_pixel_position_ref().y);
                                set_advancing_rotation(dst_rot_);
                                set_rotation(dst_rot_);

                                graphic_thing_instance_value_.intercept_once_motion(race_motion_data::NAME_SPECIAL_1 + mov_after_func_.arg_value);
                                break;
                            }

                        default:
                            {
                                if (mov_after_func_.func & FUNC_SKILL) {
                                    set_advancing_rotation(dst_rot_);
                                    blend_rotation(dst_rot_);
                                    new_use_skill(0, mov_after_func_.func & 0x7f, mov_after_func_.arg_value & 0x0f, (mov_after_func_.arg_value >> 4) ? true : false);
                                } else {

                                    disable_skip_collision_impl();
                                    is_going_ = false;

                                    block_movement();
                                    script_set_pixel_position(new_get_dst_pixel_position_ref().x, new_get_dst_pixel_position_ref().y);
                                    set_advancing_rotation(dst_rot_);
                                    blend_rotation(dst_rot_);
                                    if (!is_waiting()) {
                                        end_walking();
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (is_walking() || graphic_thing_instance_value_.is_using_moving_skill()) {
            float rotation = graphic_thing_instance_value_.rotation();
            float advancing_rotation = graphic_thing_instance_value_.advancing_rotation();
            int direction_value = get_rotating_direction(rotation, advancing_rotation);

            if (DEGREE_DIRECTION_SAME != rotating_direction_) {
                if (DEGREE_DIRECTION_LEFT == direction_value) {
                    rotation = fmodf(rotation + rot_spd_ * graphic_thing_instance_value_.second_elapsed(), 360.0f);
                } else if (DEGREE_DIRECTION_RIGHT == direction_value) {
                    rotation = fmodf(rotation - rot_spd_ * graphic_thing_instance_value_.second_elapsed() + 360.0f, 360.0f);
                }

                if (rotating_direction_ != get_rotating_direction(rotation, advancing_rotation)) {
                    rotating_direction_ = DEGREE_DIRECTION_SAME;
                    rotation = advancing_rotation;
                }

                graphic_thing_instance_value_.set_rotation(rotation);
            }

            if (is_in_dust_range()) {
                float dust_distance = new_get_distance_from_dest_pixel_position(p_pos_dust_);
                if (is_mounting_horse()) {
                    if (dust_distance > horse_dust_gap_) {
                        new_get_pixel_position(&p_pos_dust_);
                        attach_effect(EFFECT_HORSE_DUST);
                    }
                } else {
                    if (dust_distance > dust_gap_) {
                        new_get_pixel_position(&p_pos_dust_);
                        attach_effect(EFFECT_DUST);
                    }
                }
            }
        }
    }

    void instance_base::process_function_emotion(uint32_t motion_number, uint32_t target_vid, const pixel_position &pos_dst) {
        if (is_walking())
            end_walking_without_blending();

        enable_changing_tcp_state();
        script_set_pixel_position(pos_dst.x, pos_dst.y);

        instance_base *target_instance = get_python_character_manager().instance_ptr(target_vid);
        if (target_instance) {
            target_instance->enable_changing_tcp_state();

            if (target_instance->is_walking())
                target_instance->end_walking_without_blending();

            uint16_t motion_number1 = static_cast<uint16_t>(motion_number >> 16);
            uint16_t motion_number2 = static_cast<uint16_t>(motion_number);

            int src_job = race_to_job(race());
            int dst_job = race_to_job(target_instance->race());

            new_look_at_dest_instance(*target_instance);
            graphic_thing_instance_value_.intercept_once_motion(motion_number1 + dst_job);
            graphic_thing_instance_value_.set_rotation(graphic_thing_instance_value_.target_rotation());
            graphic_thing_instance_value_.set_advancing_rotation(graphic_thing_instance_value_.target_rotation());

            target_instance->new_look_at_dest_instance(*this);
            target_instance->graphic_thing_instance_value_.intercept_once_motion(motion_number2 + src_job);
            target_instance->graphic_thing_instance_value_.set_rotation(target_instance->graphic_thing_instance_value_.target_rotation());
            target_instance->graphic_thing_instance_value_.set_advancing_rotation(target_instance->graphic_thing_instance_value_.target_rotation());

            if (target_instance->is_main_instance()) {
                abstract_player &player = get_abstract_player();
                player.end_emotion_process();
            }
        }

        if (is_main_instance()) {
            abstract_player &player = get_abstract_player();
            player.end_emotion_process();
        }
    }

    int accumulation_time = 0;

    namespace {
        constexpr float HEIGHT_SNAP_GAP = 100.0f; // taller than any stair step
        constexpr float HEIGHT_EASE_RATE = 15.0f; // per second
    }

    void instance_base::update() {
        ++update_counter_;

        state_process();
        graphic_thing_instance_value_.physics_process();
        graphic_thing_instance_value_.rotation_process();
        graphic_thing_instance_value_.combo_process();
        graphic_thing_instance_value_.accumulation_movement();

        if (graphic_thing_instance_value_.is_movement()) {
            pixel_position p_pos_cur;
            new_get_pixel_position(&p_pos_cur);

            uint32_t cur_time = el_timer_get_frame_m_sec();
            {
                next_update_height_time_ = cur_time;
                // Steps and slopes are eased over about a tenth of a second instead of snapping the
                // feet; anything taller than a step (a ledge, a warp) is still taken at once.
                const float ground = background_height(p_pos_cur.x, p_pos_cur.y);
                const float gap = ground - p_pos_cur.z;
                if (std::fabs(gap) > HEIGHT_SNAP_GAP)
                    p_pos_cur.z = ground;
                else
                    p_pos_cur.z += gap * std::min(1.0f, get_timer().elapsed_second() * HEIGHT_EASE_RATE);
                new_set_pixel_position(p_pos_cur);
            }

            {
                uint32_t mtrl_color = shadow_map_color_impl(p_pos_cur.x, p_pos_cur.y);
                graphic_thing_instance_value_.set_material_color(mtrl_color);
            }
        }

        graphic_thing_instance_value_.update_advancing_point_instance();

        attack_process();
        movement_process();

        graphic_thing_instance_value_.motion_process(is_pc());
        if (is_mounting_horse()) {
            horse_.actor->horse_motion_process(false);
        }

        combo_process_impl();

        process_damage();
    }

    void instance_base::transform() {
        if (is_syncing()) {
        } else {
            if (is_walking() || graphic_thing_instance_value_.is_using_moving_skill()) {
                // The movement vector is this frame's step. A threshold per frame would take a run at
                // a high frame rate (under a unit a frame) for standing still, and the wait packet
                // that follows makes the server stop the move and end a sprint.
                constexpr float MIN_MOVING_SPEED = 60.0f; // units per second
                const math::vector3 &movment = graphic_thing_instance_value_.movement_vector_ref();
                const float min_step = MIN_MOVING_SPEED * get_timer().elapsed_second();

                float len = (movment.x * movment.x) + (movment.y * movment.y);
                if (len > min_step * min_step)
                    on_moving();
                else
                    on_waiting();
            }
        }

        graphic_thing_instance_value_.instancebase_transform();
    }

    void instance_base::deform() {
        if (!can_render())
            return;

        ++deform_counter_;

        graphic_thing_instance_value_.instancebase_deform();

        horse_.deform();
    }

    void instance_base::render_trace() {
        if (!can_render())
            return;

        graphic_thing_instance_value_.render_trace();
    }

    void instance_base::render() {
        if (!can_render())
            return;

        ++render_counter_;

        horse_.render();
        graphic_thing_instance_value_.render();

        if (actor_instance::is_dir_line()) {
            if (new_get_dst_pixel_position_ref().x != 0.0f) {
                static screen screen;

                STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::DIFFUSE);
                STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
                STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
                STATEMANAGER.save_render_state(gpu::state::Z_ENABLE, false);
                STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, false);
                STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);

                pixel_position px;
                graphic_thing_instance_value_.get_pixel_position(&px);
                math::vector3 current_position(px.x, px.y, px.z);
                math::vector3 destination_position(new_get_dst_pixel_position_ref().x, -new_get_dst_pixel_position_ref().y, new_get_dst_pixel_position_ref().z);

                screen.set_diffuse_color(0.0f, 0.0f, 1.0f);
                screen.render_line3d(current_position.x, current_position.y, px.z, destination_position.x, destination_position.y, px.z);
                STATEMANAGER.restore_render_state(gpu::state::Z_ENABLE);
                STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, true);
                STATEMANAGER.set_render_state(gpu::state::LIGHTING, true);
            }
        }
    }

    void instance_base::render_to_shadow_map() {
        if (is_door())
            return;

        if (is_building())
            return;

        if (!can_render())
            return;

        if (!is_exist_main_instance())
            return;

        instance_base *inst_main = main_instance_ptr_impl();

        const float shadow_apply_distance = 2500.0f;

        float distance = new_get_distance_from_dest_instance(*inst_main);
        if (distance >= shadow_apply_distance)
            return;

        graphic_thing_instance_value_.render_to_shadow_map();
    }

    void instance_base::render_collision() {
        graphic_thing_instance_value_.render_collision_data();
    }

    void instance_base::set_virtual_id(uint32_t virtual_id) {
        graphic_thing_instance_value_.set_virtual_id(virtual_id);
    }

    void instance_base::set_virtual_number(uint32_t virtual_number) {
        virtual_number_ = virtual_number;
    }

    void instance_base::set_instance_type(int instance_type) {
        graphic_thing_instance_value_.set_actor_type(instance_type);
    }

    void instance_base::set_alignment(short alignment) {
        alignment_ = alignment;
        refresh_text_tail_title();
    }

    void instance_base::set_pk_mode(uint8_t pk_mode) {
        if (pk_mode_ == pk_mode)
            return;

        pk_mode_ = pk_mode;

        if (is_main_instance()) {
            abstract_player &player = get_abstract_player();
            player.notify_change_pk_mode();
        }
    }

    void instance_base::set_killer(bool flag) {
        if (is_killer_ == flag)
            return;

        is_killer_ = flag;
        refresh_text_tail();
    }

    void instance_base::set_party_member_flag(bool flag) {
        is_party_member_ = flag;
    }

    void instance_base::set_state_flags(uint32_t state_flags) {
        if (state_flags & ADD_CHARACTER_STATE_KILLER)
            set_killer(true);
        else
            set_killer(false);

        if (state_flags & ADD_CHARACTER_STATE_PARTY)
            set_party_member_flag(true);
        else
            set_party_member_flag(false);
    }

    void instance_base::set_combo_type(uint32_t combo_type) {
        graphic_thing_instance_value_.set_combo_type(combo_type);
    }

    auto instance_base::name_string() -> const char * {
        return name_value_.c_str();
    }

    auto instance_base::race() -> uint32_t {
        return race_;
    }

    auto instance_base::is_conflict_alignment_instance(instance_base &inst_victim) -> bool {
        if (PK_MODE_PROTECT == inst_victim.pk_mode())
            return false;

        switch (alignment_type()) {
        case ALIGNMENT_TYPE_NORMAL:
        case ALIGNMENT_TYPE_WHITE:
            if (ALIGNMENT_TYPE_DARK == inst_victim.alignment_type())
                return true;
            break;
        case ALIGNMENT_TYPE_DARK:
            if (alignment_type() != inst_victim.alignment_type())
                return true;
            break;
        }

        return false;
    }

    void instance_base::set_duel_mode(uint32_t type_value) {
        duel_mode_ = type_value;
    }

    auto instance_base::duel_mode() -> uint32_t {
        return duel_mode_;
    }

    auto instance_base::is_attackable_instance(instance_base &inst_victim) -> bool {
        if (is_main_instance()) {
            python_player &player = get_python_player();
            if (player.is_observer_mode())
                return false;
        }

        if (virtual_id() == inst_victim.virtual_id())
            return false;

        if (is_stone()) {
            if (inst_victim.is_pc())
                return true;
        } else if (is_pc()) {
            if (inst_victim.is_stone())
                return true;

            if (inst_victim.is_pc()) {
                if (duel_mode()) {
                    switch (duel_mode()) {
                    case DUEL_CANNOTATTACK:
                        return false;
                    case DUEL_START:
                        if (find_duel_key(virtual_id(), inst_victim.virtual_id()))
                            return true;
                        else
                            return false;
                    }
                }
                if (PK_MODE_GUILD == pk_mode())
                    if (guild_id() == inst_victim.guild_id())
                        return false;

                if (inst_victim.is_killer())
                    if (!get_abstract_player().is_same_party_member(virtual_id(), inst_victim.virtual_id()))
                        return true;

                if (PK_MODE_PROTECT != pk_mode()) {
                    if (PK_MODE_FREE == pk_mode()) {
                        if (PK_MODE_PROTECT != inst_victim.pk_mode())
                            if (!get_abstract_player().is_same_party_member(virtual_id(), inst_victim.virtual_id()))
                                return true;
                    }
                    if (PK_MODE_GUILD == pk_mode()) {
                        if (PK_MODE_PROTECT != inst_victim.pk_mode())
                            if (!get_abstract_player().is_same_party_member(virtual_id(), inst_victim.virtual_id()))
                                if (guild_id() != inst_victim.guild_id())
                                    return true;
                    }
                }

                if (is_same_empire(inst_victim)) {
                    if (is_pvp_instance(inst_victim))
                        return true;

                    if (PK_MODE_REVENGE == pk_mode())
                        if (!get_abstract_player().is_same_party_member(virtual_id(), inst_victim.virtual_id()))
                            if (is_conflict_alignment_instance(inst_victim))
                                return true;
                } else {
                    return true;
                }
            }

            if (inst_victim.is_enemy())
                return true;

            if (inst_victim.is_wooden_door())
                return true;
        } else if (is_enemy()) {
            if (inst_victim.is_pc())
                return true;

            if (inst_victim.is_building())
                return true;

        } else if (is_poly()) {
            if (inst_victim.is_pc())
                return true;

            if (inst_victim.is_enemy())
                return true;
        }
        return false;
    }

    auto instance_base::is_targetable_instance(instance_base &inst_victim) -> bool {
        return inst_victim.can_pick_instance();
    }

    auto instance_base::can_change_target() -> bool {
        return graphic_thing_instance_value_.can_change_target();
    }

    auto instance_base::can_pick_instance() -> bool {
        if (!is_in_view_frustum())
            return false;

        if (is_door()) {
            if (is_dead())
                return false;
        }

        if (is_pc()) {
            if (is_affect(AFFECT_EUNHYEONG)) {
                if (!main_can_see_hidden_thing())
                    return false;
            }
            if (is_affect(AFFECT_REVIVE_INVISIBILITY))
                return false;
            if (is_affect(AFFECT_INVISIBILITY))
                return false;
        }

        if (is_dead())
            return false;

        return true;
    }

    auto instance_base::can_view_target_hp(instance_base &inst_victim) -> bool {
        if (inst_victim.is_stone())
            return true;
        if (inst_victim.is_wooden_door())
            return true;
        if (inst_victim.is_enemy())
            return true;

        return false;
    }

    auto instance_base::is_poly() -> bool {
        return graphic_thing_instance_value_.is_poly();
    }

    auto instance_base::is_pc() -> bool {
        return graphic_thing_instance_value_.is_pc();
    }

    auto instance_base::is_npc() -> bool {
        return graphic_thing_instance_value_.is_npc();
    }

    auto instance_base::is_enemy() -> bool {
        return graphic_thing_instance_value_.is_enemy();
    }

    auto instance_base::is_stone() -> bool {
        return graphic_thing_instance_value_.is_stone();
    }

    auto instance_base::is_guild_wall() -> bool // Only the walls, not the gates; is_building covers every guild building.
    {
        return is_wall(race_);
    }

    auto instance_base::is_resource() -> bool {
        switch (virtual_number_) {
        case 20047:
        case 20048:
        case 20049:
        case 20050:
        case 20051:
        case 20052:
        case 20053:
        case 20054:
        case 20055:
        case 20056:
        case 20057:
        case 20058:
        case 20059:
        case 30301:
        case 30302:
        case 30303:
        case 30304:
        case 30305:
            return true;
        }

        return false;
    }

    auto instance_base::is_warp() -> bool {
        return graphic_thing_instance_value_.is_warp();
    }

    auto instance_base::is_goto() -> bool {
        return graphic_thing_instance_value_.is_goto();
    }

    auto instance_base::is_object() -> bool {
        return graphic_thing_instance_value_.is_object();
    }

    auto instance_base::is_building() -> bool {
        return graphic_thing_instance_value_.is_building();
    }

    auto instance_base::is_door() -> bool {
        return graphic_thing_instance_value_.is_door();
    }

    auto instance_base::is_wooden_door() -> bool {
        if (graphic_thing_instance_value_.is_door()) {
            int vnum = virtual_number();
            if (vnum == 13000) // wooden gate
                return true;
            else if (vnum >= 30111 && vnum <= 30119) // Sagwi gate
                return true;
            else
                return false;
        } else {
            return false;
        }
    }

    auto instance_base::is_stone_door() -> bool {
        return graphic_thing_instance_value_.is_door() && 13001 == virtual_number();
    }

    auto instance_base::is_flag() -> bool {
        if (race() == 20035)
            return true;
        if (race() == 20036)
            return true;
        if (race() == 20037)
            return true;

        return false;
    }

    auto instance_base::is_force_visible() -> bool {
        if (is_affect(AFFECT_SHOW_ALWAYS))
            return true;

        if (is_object() || is_building() || is_door())
            return true;

        return false;
    }

    auto instance_base::instance_type() -> int {
        return graphic_thing_instance_value_.actor_type();
    }

    auto instance_base::virtual_id() -> uint32_t {
        return graphic_thing_instance_value_.virtual_id();
    }

    auto instance_base::virtual_number() -> uint32_t {
        return virtual_number_;
    }

    auto instance_base::is_in_view_frustum() -> bool {
        return graphic_thing_instance_value_.is_show();
    }

    auto instance_base::can_render() -> bool {
        if (!is_in_view_frustum())
            return false;
        if (is_affect(AFFECT_INVISIBILITY))
            return false;

        return true;
    }

    auto instance_base::intersect_bounding_box() -> bool {
        float u, v, t;
        return graphic_thing_instance_value_.intersect(&u, &v, &t);
    }

    auto instance_base::intersect_defending_sphere() -> bool {
        return graphic_thing_instance_value_.intersect_defending_sphere();
    }

    auto instance_base::distance(instance_base *target_inst) -> float {
        pixel_position target_pixel_position;
        target_inst->graphic_thing_instance_value_.get_pixel_position(&target_pixel_position);
        return distance(target_pixel_position);
    }

    auto instance_base::distance(const pixel_position &c_r_pixel_position) -> float {
        pixel_position pixel_position;
        graphic_thing_instance_value_.get_pixel_position(&pixel_position);

        float fdx = pixel_position.x - c_r_pixel_position.x;
        float fdy = pixel_position.y - c_r_pixel_position.y;

        return sqrtf((fdx * fdx) + (fdy * fdy));
    }

    auto instance_base::graphic_thing_instance_ref() -> actor_instance & {
        return graphic_thing_instance_value_;
    }

    auto instance_base::graphic_thing_instance_ptr() -> actor_instance * {
        return &graphic_thing_instance_value_;
    }

    void instance_base::refresh_actor_instance() {
        graphic_thing_instance_value_.refresh_actor_instance();
    }

    void instance_base::refresh(uint32_t mot_index, bool is_loop) {
        refresh_state(mot_index, is_loop);
    }

    void instance_base::restore_render_mode() {
        graphic_thing_instance_value_.restore_render_mode();
    }

    void instance_base::set_add_render_mode() {
        graphic_thing_instance_value_.set_add_render_mode();
    }

    void instance_base::set_modulate_render_mode() {
        graphic_thing_instance_value_.set_modulate_render_mode();
    }

    void instance_base::set_render_mode(int render_mode) {
        graphic_thing_instance_value_.set_render_mode(render_mode);
    }

    void instance_base::set_add_color(const math::color &color) {
        graphic_thing_instance_value_.set_add_color(color);
    }

    void instance_base::set_blend_rendering_mode() {
        graphic_thing_instance_value_.set_blend_render_mode();
    }

    void instance_base::set_alpha_value_impl(float alpha) {
        graphic_thing_instance_value_.set_alpha_value(alpha);
    }

    auto instance_base::alpha_value_impl() -> float {
        return graphic_thing_instance_value_.alpha_value();
    }

    void instance_base::set_hair(uint32_t hair) {
        if (!hair_color_enable)
            return;

        if (is_pc() == false)
            return;
        part_value_[race_data::PART_HAIR] = hair;
        graphic_thing_instance_value_.set_hair(hair);
    }

    void instance_base::change_hair(uint32_t hair) {
        if (!hair_color_enable)
            return;

        if (is_pc() == false)
            return;

        if (get_part(race_data::PART_HAIR) == hair)
            return;

        set_hair(hair);

        refresh_state(race_motion_data::NAME_WAIT, true);
    }

    void instance_base::set_armor(uint32_t armor) {
        uint32_t shape;
        if (armor_vnum_to_shape(armor, &shape)) {
            item_data *item_data;
            if (get_item_manager().item_data_pointer(armor, &item_data)) {
                float specular_power = item_data->specular_powerf();
                set_shape(shape, specular_power);
                refined_effect(item_data);
                return;
            } else
                clear_armor_refine_effect();
        }

        set_shape(armor);
    }

    void instance_base::set_shape(uint32_t shape, float specular) {
        if (is_poly()) {
            graphic_thing_instance_value_.set_shape(0);
        } else {
            graphic_thing_instance_value_.set_shape(shape, specular);
        }

        shape_value_ = shape;
    }

    auto instance_base::weapon_type() -> uint32_t {
        uint32_t weapon = get_part(race_data::PART_WEAPON);
        item_data *item_data;
        if (!get_item_manager().item_data_pointer(weapon, &item_data))
            return item_data::WEAPON_NONE;

        return item_data->weapon_type();
    }

    void instance_base::clear_weapon_refine_effect() {
        if (sword_refine_effect_right_) {
            detach_effect(sword_refine_effect_right_);
            sword_refine_effect_right_ = 0;
        }
        if (sword_refine_effect_left_) {
            detach_effect(sword_refine_effect_left_);
            sword_refine_effect_left_ = 0;
        }
    }

    void instance_base::clear_armor_refine_effect() {
        if (armor_refine_effect_) {
            detach_effect(armor_refine_effect_);
            armor_refine_effect_ = 0;
        }
    }

    auto instance_base::refined_effect(item_data *item) -> uint32_t {
        uint32_t refine = std::max<uint32_t>(item->refine() + item->socket_count(), item_data::ITEM_SOCKET_MAX_NUM) - item_data::ITEM_SOCKET_MAX_NUM;
        switch (item->get_type()) {
        case item_data::ITEM_TYPE_WEAPON:
            clear_weapon_refine_effect();
            if (refine < 7) // Only refine level 7 and above has an effect.
                return 0;
            switch (item->sub_type()) {
            case item_data::WEAPON_DAGGER:
                sword_refine_effect_right_ = EFFECT_REFINED + static_cast<uint32_t>(EFFECT_SMALLSWORD_REFINED7) + refine - 7;
                sword_refine_effect_left_ = EFFECT_REFINED + static_cast<uint32_t>(EFFECT_SMALLSWORD_REFINED7_LEFT) + refine - 7;
                break;
            case item_data::WEAPON_FAN:
                sword_refine_effect_right_ = EFFECT_REFINED + static_cast<uint32_t>(EFFECT_FANBELL_REFINED7) + refine - 7;
                break;
            case item_data::WEAPON_ARROW:
            case item_data::WEAPON_BELL:
                sword_refine_effect_right_ = EFFECT_REFINED + static_cast<uint32_t>(EFFECT_SMALLSWORD_REFINED7) + refine - 7;
                break;
            case item_data::WEAPON_BOW:
                sword_refine_effect_right_ = EFFECT_REFINED + static_cast<uint32_t>(EFFECT_BOW_REFINED7) + refine - 7;
                break;
            default:
                sword_refine_effect_right_ = EFFECT_REFINED + static_cast<uint32_t>(EFFECT_SWORD_REFINED7) + refine - 7;
            }
            if (sword_refine_effect_right_)
                sword_refine_effect_right_ = attach_effect(sword_refine_effect_right_);
            if (sword_refine_effect_left_)
                sword_refine_effect_left_ = attach_effect(sword_refine_effect_left_);
            break;
        case item_data::ITEM_TYPE_ARMOR:
            clear_armor_refine_effect();

            if (item->sub_type() == item_data::ARMOR_BODY) {
                uint32_t vnum = item->get_index();

                if (12010 <= vnum && vnum <= 12049) {
                    attach_effect(EFFECT_REFINED + static_cast<uint32_t>(EFFECT_BODYARMOR_SPECIAL));
                    attach_effect(EFFECT_REFINED + static_cast<uint32_t>(EFFECT_BODYARMOR_SPECIAL2));
                }
            }

            if (refine < 7) // Only refine level 7 and above has an effect.
                return 0;

            if (item->sub_type() == item_data::ARMOR_BODY) {
                armor_refine_effect_ = EFFECT_REFINED + static_cast<uint32_t>(EFFECT_BODYARMOR_REFINED7) + refine - 7;
                attach_effect(armor_refine_effect_);
            }
            break;
        }
        return 0;
    }

    auto instance_base::set_weapon(uint32_t weapon) -> bool {
        if (is_poly())
            return false;

        if (is_shape_animal_wear())
            return false;

        if (is_changable_weapon(weapon) == false)
            weapon = 0;

        graphic_thing_instance_value_.attach_weapon(weapon);
        part_value_[race_data::PART_WEAPON] = weapon;

        item_data *item_data;
        if (get_item_manager().item_data_pointer(weapon, &item_data))
            refined_effect(item_data);
        else
            clear_weapon_refine_effect();

        return true;
    }

    void instance_base::change_weapon(uint32_t weapon) {
        if (weapon == graphic_thing_instance_value_.part_item_id(race_data::PART_WEAPON))
            return;

        if (set_weapon(weapon))
            refresh_state(race_motion_data::NAME_WAIT, true);
    }

    auto instance_base::change_armor(uint32_t armor) -> bool {
        uint32_t shape_value;
        armor_vnum_to_shape(armor, &shape_value);

        if (get_shape() == shape_value)
            return false;

        affect_flag_container affect_flag_container_value;
        affect_flag_container_value.copy_instance(affect_flag_container_value_);

        uint32_t vid = virtual_id();
        uint32_t race_value = race();
        uint32_t hair_value = get_part(race_data::PART_HAIR);
        uint32_t weapon = get_part(race_data::PART_WEAPON);
        float rot = rotation();
        float adv_rot = advancing_rotation();

        if (is_walking())
            end_walking();

        clear_affects_impl();

        if (!set_race(race_value)) {
            metin2::log::error("Character (vid {}): cannot change armor, race {} cannot be set", vid, race_value);
            return false;
        }

        set_armor(armor);
        set_hair(hair_value);
        set_weapon(weapon);

        set_rotation(rot);
        set_advancing_rotation(adv_rot);

        attach_horse_saddle();

        refresh_state(race_motion_data::NAME_WAIT, true);

        set_affect_flag_container(affect_flag_container_value);

        actor_instance::event_handler &event_handler_value = event_handler_ref();
        event_handler_value.on_change_shape();

        return true;
    }

    auto instance_base::is_shape_animal_wear() -> bool {
        if (100 == get_shape() ||
            101 == get_shape() ||
            102 == get_shape() ||
            103 == get_shape())
            return true;

        return false;
    }

    void instance_base::refresh_state(uint32_t mot_index, bool is_loop) {
        uint32_t part_item_id = graphic_thing_instance_value_.part_item_id(race_data::PART_WEAPON);

        uint8_t item_type = 0xff;
        uint8_t sub_type = 0xff;

        item_manager &item_mgr = get_item_manager();
        item_data *item_data;

        if (item_mgr.item_data_pointer(part_item_id, &item_data)) {
            item_type = item_data->get_type();
            sub_type = item_data->weapon_type();
        }

        if (is_poly()) {
            set_motion_mode(race_motion_data::MODE_GENERAL);
        } else if (is_wearing_dress()) {
            set_motion_mode(race_motion_data::MODE_WEDDING_DRESS);
        } else if (is_holding_pick_axe()) {
            if (horse_.is_mounting()) {
                set_motion_mode(race_motion_data::MODE_HORSE);
            } else {
                set_motion_mode(race_motion_data::MODE_GENERAL);
            }
        } else if (item_data::ITEM_TYPE_ROD == item_type) {
            if (horse_.is_mounting()) {
                set_motion_mode(race_motion_data::MODE_HORSE);
            } else {
                set_motion_mode(race_motion_data::MODE_FISHING);
            }
        } else if (horse_.is_mounting()) {
            switch (sub_type) {
            case item_data::WEAPON_SWORD:
                set_motion_mode(race_motion_data::MODE_HORSE_ONEHAND_SWORD);
                break;

            case item_data::WEAPON_TWO_HANDED:
                set_motion_mode(race_motion_data::MODE_HORSE_TWOHAND_SWORD);
                break;

            case item_data::WEAPON_DAGGER:
                set_motion_mode(race_motion_data::MODE_HORSE_DUALHAND_SWORD);
                break;

            case item_data::WEAPON_FAN:
                set_motion_mode(race_motion_data::MODE_HORSE_FAN);
                break;

            case item_data::WEAPON_BELL:
                set_motion_mode(race_motion_data::MODE_HORSE_BELL);
                break;

            case item_data::WEAPON_BOW:
                set_motion_mode(race_motion_data::MODE_HORSE_BOW);
                break;

            default:
                set_motion_mode(race_motion_data::MODE_HORSE);
                break;
            }
        } else {
            switch (sub_type) {
            case item_data::WEAPON_SWORD:
                set_motion_mode(race_motion_data::MODE_ONEHAND_SWORD);
                break;

            case item_data::WEAPON_TWO_HANDED:
                set_motion_mode(race_motion_data::MODE_TWOHAND_SWORD);
                break;

            case item_data::WEAPON_DAGGER:
                set_motion_mode(race_motion_data::MODE_DUALHAND_SWORD);
                break;

            case item_data::WEAPON_BOW:
                set_motion_mode(race_motion_data::MODE_BOW);
                break;

            case item_data::WEAPON_FAN:
                set_motion_mode(race_motion_data::MODE_FAN);
                break;

            case item_data::WEAPON_BELL:
                set_motion_mode(race_motion_data::MODE_BELL);
                break;

            case item_data::WEAPON_ARROW:
            default:
                set_motion_mode(race_motion_data::MODE_GENERAL);
                break;
            }
        }

        if (is_loop)
            graphic_thing_instance_value_.intercept_loop_motion(mot_index);
        else
            graphic_thing_instance_value_.intercept_once_motion(mot_index);

        refresh_actor_instance();
    }

    void instance_base::register_bounding_sphere() {
        // Stones skip deform_no_skin: animations such as falling would move the bounding box and break culling.
        if (!is_stone()) {
            graphic_thing_instance_value_.deform_no_skin();
        }

        graphic_thing_instance_value_.register_bounding_sphere();
    }

    auto instance_base::create_device_objects() -> bool {
        return graphic_thing_instance_value_.create_device_objects();
    }

    void instance_base::destroy_device_objects() {
        graphic_thing_instance_value_.destroy_device_objects();
    }

    void instance_base::destroy() {
        detach_text_tail();

        dismount_horse();

        que_cmd_new_.clear();

        effect_container_destroy();
        stone_smoke_destroy();

        if (is_main_instance())
            clear_main_instance_impl();

        graphic_thing_instance_value_.destroy();

        initialize_impl();
    }

    void instance_base::initialize_rotation_speed() {
        set_rotation_speed(c_fDefaultRotationSpeed);
    }

    void instance_base::warrior_initialize() {
        warrior_value_.geomgyeong_effect = 0;
    }

    void instance_base::initialize_impl() {
        warrior_initialize();
        stone_smoke_inialize();
        effect_container_initialize();
        initialize_rotation_speed();

        set_event_handler(actor_instance::event_handler::empty_ptr());

        affect_flag_container_value_.clear();

        level_value_ = 0;
        guild_id_ = 0;
        empire_id_ = 0;

        type_value_ = 0;
        race_type_ = 0;
        shape_value_ = 0;
        race_ = 0;
        virtual_number_ = 0;

        base_cmd_time_ = 0;
        base_chk_time_ = 0;
        skip_time_ = 0;

        graphic_thing_instance_value_.initialize();

        adv_actor_vid_ = 0;
        last_dmg_actor_vid_ = 0;

        average_network_gap_ = 0;
        next_update_height_time_ = 0;

        rotating_direction_ = DEGREE_DIRECTION_SAME;

        is_text_tail_ = false;
        is_going_ = false;
        new_set_src_pixel_position(pixel_position(0, 0, 0));
        new_set_dst_pixel_position(pixel_position(0, 0, 0));

        p_pos_dust_ = pixel_position(0, 0, 0);

        que_cmd_new_.clear();

        last_combo_index_ = 0;

        sword_refine_effect_right_ = 0;
        sword_refine_effect_left_ = 0;
        armor_refine_effect_ = 0;

        alignment_ = 0;
        pk_mode_ = 0;
        is_killer_ = false;
        is_party_member_ = false;

        enable_tcp_state_ = true;

        name_value_ = "";

        memset(part_value_, 0, sizeof(part_value_));
        memset(attached_affect_effects_, 0, sizeof(attached_affect_effects_));
        memset(static_cast<void *>(&mov_after_func_), 0, sizeof(mov_after_func_));

        damage_effect_type_ = false;
        duel_mode_ = DUEL_NONE;
        emoticon_time_ = 0;
    }

    instance_base::instance_base() {
        initialize_impl();
    }

    instance_base::~instance_base() {
        destroy();
    }

    void instance_base::get_bound_box(math::vector3 *vt_min, math::vector3 *vt_max) {
        graphic_thing_instance_value_.get_bound_box(vt_min, vt_max);
    }

    void instance_base::set_level(uint32_t lvl) {
        level_value_ = lvl;
    }
}

namespace metin2::inline world::inline actors {

    auto get_background_interface() -> background_interface & {
        static metin2::actor_instance_background instance;
        return instance;
    }
}
