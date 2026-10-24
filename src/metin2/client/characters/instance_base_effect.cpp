#include <metin2/client/app/abstract_application.h>
#include <metin2/client/app/abstract_player.h>
#include <metin2/client/app/python_system.h>
#include <metin2/client/player/python_player.h>
#include <metin2/client/social/python_text_tail.h>
#include <metin2/log/log.h>
#include "instance_base.h"

#include <format>
#include <metin2/core/timer.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/graphics/camera.h>
#include <metin2/world/actors/game_util.h>

namespace metin2::inline client::inline characters {

    float instance_base::dust_gap_;
    float instance_base::horse_dust_gap_;
    uint32_t instance_base::affect_effect_crcs_[instance_base::EFFECT_NUM];
    std::string instance_base::affect_effect_attach_bone_[EFFECT_NUM];

#define BYTE_COLOR_TO_COLOR(r, g, b) math::color(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, 1.0f)

    math::color title_colors[instance_base::TITLE_NUM];
    math::color name_colors[instance_base::NAMECOLOR_NUM];

    std::map<int, std::string> title_name_map;
    std::set<uint32_t> set_pvp_ready_key;
    std::set<uint32_t> set_pvp_key;
    std::set<uint32_t> set_gvg_key;
    std::set<uint32_t> set_duel_key;

    bool is_empire_name_mode = false;

    void instance_base::set_empire_name_mode(bool is_enable) {
        is_empire_name_mode = is_enable;

        if (is_enable) {
            name_colors[NAMECOLOR_MOB] = name_colors[NAMECOLOR_EMPIRE_MOB];
            name_colors[NAMECOLOR_NPC] = name_colors[NAMECOLOR_EMPIRE_NPC];
            name_colors[NAMECOLOR_PC] = name_colors[NAMECOLOR_NORMAL_PC];

            for (uint32_t empire = 1; empire < EMPIRE_NUM; ++empire)
                name_colors[NAMECOLOR_PC + empire] = name_colors[NAMECOLOR_EMPIRE_PC + empire];

        } else {
            name_colors[NAMECOLOR_MOB] = name_colors[NAMECOLOR_NORMAL_MOB];
            name_colors[NAMECOLOR_NPC] = name_colors[NAMECOLOR_NORMAL_NPC];

            for (uint32_t empire = 0; empire < EMPIRE_NUM; ++empire)
                name_colors[NAMECOLOR_PC + empire] = name_colors[NAMECOLOR_NORMAL_PC];
        }
    }

    auto instance_base::indexed_name_color(uint32_t name_color) -> const math::color & {
        if (name_color >= NAMECOLOR_NUM) {
            static math::color default_name_color(0xffffffff);
            return default_name_color;
        }

        return name_colors[name_color];
    }

    void instance_base::add_damage_effect(uint32_t damage, uint8_t flag, bool self, bool target) {
        if (get_python_system().is_show_damage()) {
            effect_damage s_damage;
            s_damage.self = self;
            s_damage.target = target;
            s_damage.damage = damage;
            s_damage.flag = flag;
            damage_queue_.push_back(s_damage);
        }
    }

    void instance_base::process_damage() {
        if (damage_queue_.empty())
            return;

        effect_damage s_damage = damage_queue_.front();

        damage_queue_.pop_front();

        uint32_t damage = s_damage.damage;
        uint8_t flag = s_damage.flag;
        bool self = s_damage.self;
        bool target = s_damage.target;

        camera *camera = get_camera_manager().current_camera();
        float camera_angle = get_degree_from_position2(camera->target().x, camera->target().y, camera->eye().x, camera->eye().y);

        uint32_t font_width = 30;

        effect_manager &eft_mgr = get_effect_manager();

        math::vector3 pos = graphic_thing_instance_value_.get_position();
        pos.z += float(graphic_thing_instance_value_.height());

        math::vector3 rot = math::vector3(0.0f, 0.0f, camera_angle);

        if ((flag & DAMAGE_DODGE) || (flag & DAMAGE_BLOCK)) {
            if (self)
                eft_mgr.create_effect(affect_effect_crcs_[EFFECT_DAMAGE_MISS], pos, rot);
            else
                eft_mgr.create_effect(affect_effect_crcs_[EFFECT_DAMAGE_TARGETMISS], pos, rot);
            return;
        }
        if ((flag & DAMAGE_CRITICAL) && affect_effect_crcs_[EFFECT_DAMAGE_CRITICAL])
            eft_mgr.create_effect(affect_effect_crcs_[EFFECT_DAMAGE_CRITICAL], pos, rot);

        std::string damage_type;
        uint32_t crc_eft = 0;
        {
            if (self) {
                damage_type = "damage_";
                if (damage_effect_type_ == 0)
                    crc_eft = EFFECT_DAMAGE_SELFDAMAGE;
                else
                    crc_eft = EFFECT_DAMAGE_SELFDAMAGE2;
                damage_effect_type_ = !damage_effect_type_;
            } else if (target == false) {
                damage_type = "nontarget_";
                crc_eft = EFFECT_DAMAGE_NOT_TARGET;
                return; // Non-target damage is not shown for now.
            } else {
                damage_type = "target_";
                crc_eft = EFFECT_DAMAGE_TARGET;
            }
        }

        uint32_t index = 0;
        uint32_t num = 0;
        std::vector<std::string> textures;
        while (damage > 0) {
            if (index > 7) {
                metin2::log::error("Possible endless loop while processing damage");
                break;
            }
            num = damage % 10;
            damage /= 10;
            textures.push_back(std::format("d:/ymir work/effect/affect/damagevalue/{}{}.dds", damage_type, num));

            eft_mgr.set_effect_textures(affect_effect_crcs_[crc_eft], textures);

            math::matrix matrix, trans;
            matrix = math::matrix_identity();
            matrix._41 = pos.x;
            matrix._42 = pos.y;
            matrix._43 = pos.z;
            matrix = math::matrix_translation(pos.x, pos.y, pos.z);
            matrix = camera->inverse_view_matrix() * matrix;
            trans = math::matrix_translation(font_width * index, 0, 0);
            trans._41 = -trans._41;
            matrix = trans * matrix;
            matrix = camera->view_matrix() * matrix;

            eft_mgr.create_effect(affect_effect_crcs_[crc_eft], math::vector3(matrix._41, matrix._42, matrix._43), rot);

            textures.clear();

            index++;
        }
    }

    void instance_base::attach_special_effect(uint32_t effect) {
        attach_effect(effect);
    }

    void instance_base::level_up() {
        attach_effect(EFFECT_LEVELUP);
    }

    void instance_base::skill_up() {
        attach_effect(EFFECT_SKILLUP);
    }

    void instance_base::create_special_effect(uint32_t i_effect_index) {
        const math::matrix &global = graphic_thing_instance_value_.get_transform();

        uint32_t dw_effect_index = get_effect_manager().empty_index();
        uint32_t effect_crc = affect_effect_crcs_[i_effect_index];
        get_effect_manager().create_effect_instance(dw_effect_index, effect_crc);
        get_effect_manager().select_effect_instance(dw_effect_index);
        get_effect_manager().set_effect_instance_global_matrix(global);
    }

    void instance_base::effect_container_destroy() {
        effect_container::dict &dct_eft_id = effect_container_get_dict();

        effect_container::dict::iterator i;
        for (i = dct_eft_id.begin(); i != dct_eft_id.end(); ++i)
            detach_effect(i->second);

        dct_eft_id.clear();
    }

    void instance_base::effect_container_initialize() {
        effect_container::dict &dct_eft_id = effect_container_get_dict();
        dct_eft_id.clear();
    }

    auto instance_base::effect_container_get_dict() -> instance_base::effect_container::dict & {
        return effect_container_value_.dct_eft_id;
    }

    auto instance_base::effect_container_attach_effect(uint32_t eft_key) -> uint32_t {
        effect_container::dict &dct_eft_id = effect_container_get_dict();
        auto f = dct_eft_id.find(eft_key);
        if (dct_eft_id.end() != f)
            return 0;

        uint32_t eft_id = attach_effect(eft_key);
        dct_eft_id.insert(effect_container::dict::value_type(eft_key, eft_id));
        return eft_id;
    }

    void instance_base::effect_container_detach_effect(uint32_t eft_key) {
        effect_container::dict &dct_eft_id = effect_container_get_dict();
        auto f = dct_eft_id.find(eft_key);
        if (dct_eft_id.end() == f)
            return;

        detach_effect(f->second);

        dct_eft_id.erase(f);
    }

    void instance_base::attach_empire_effect(uint32_t empire) {
        if (!is_exist_main_instance())
            return;

        instance_base *inst_main = main_instance_ptr_impl();

        if (is_warp())
            return;
        if (is_object())
            return;
        if (is_flag())
            return;
        if (is_resource())
            return;

        if (inst_main->is_game_master()) {
        } else {
            if (inst_main->is_same_empire(*this))
                return;

            if (is_affect(AFFECT_EUNHYEONG))
                return;
        }

        if (is_game_master())
            return;

        effect_container_attach_effect(EFFECT_EMPIRE + empire);
    }

    void instance_base::attach_select_effect() {
        effect_container_attach_effect(EFFECT_SELECT);
    }

    void instance_base::detach_select_effect() {
        effect_container_detach_effect(EFFECT_SELECT);
    }

    void instance_base::attach_target_effect() {
        effect_container_attach_effect(EFFECT_TARGET);
    }

    void instance_base::detach_target_effect() {
        effect_container_detach_effect(EFFECT_TARGET);
    }

    void instance_base::stone_smoke_inialize() {
        stone_smoke_value_.eft_id = 0;
    }

    void instance_base::stone_smoke_destroy() {
        if (!stone_smoke_value_.eft_id)
            return;

        detach_effect(stone_smoke_value_.eft_id);
        stone_smoke_value_.eft_id = 0;
    }

    void instance_base::stone_smoke_create(uint32_t smoke) {
        stone_smoke_value_.eft_id = graphic_thing_instance_value_.attach_smoke_effect(smoke);
    }

    void instance_base::set_alpha(float alpha) {
        set_blend_rendering_mode();
        set_alpha_value_impl(alpha);
    }

    auto instance_base::update_deleting() -> bool {
        update();
        transform();

        abstract_application &app = get_abstract_application();

        float alpha = alpha_value_impl() - (app.global_elapsed_time() * 1.5f);
        set_alpha_value_impl(alpha);

        if (alpha < 0.0f)
            return false;

        return true;
    }

    void instance_base::delete_blend_out() {
        set_blend_rendering_mode();
        set_alpha_value_impl(1.0f);
        detach_text_tail();

        abstract_player &player = get_abstract_player();
        player.notify_deleting_character_instance(virtual_id());
    }

    void instance_base::clear_pvp_key_system() {
        set_pvp_ready_key.clear();
        set_pvp_key.clear();
        set_gvg_key.clear();
        set_duel_key.clear();
    }

    void instance_base::insert_pvp_key(uint32_t vid_src, uint32_t vid_dst) {
        uint32_t pvp_key_value = pvp_key(vid_src, vid_dst);

        set_pvp_key.insert(pvp_key_value);
    }

    void instance_base::remove_pvp_key(uint32_t vid_src, uint32_t vid_dst) {
        uint32_t pvp_key_value = pvp_key(vid_src, vid_dst);

        set_pvp_key.erase(pvp_key_value);
    }

    void instance_base::insert_gvg_key(uint32_t src_guild_vid, uint32_t dst_guild_vid) {
        uint32_t gvg_key = pvp_key(src_guild_vid, dst_guild_vid);
        set_gvg_key.insert(gvg_key);
    }

    void instance_base::remove_gvg_key(uint32_t src_guild_vid, uint32_t dst_guild_vid) {
        uint32_t gvg_key = pvp_key(src_guild_vid, dst_guild_vid);
        set_gvg_key.erase(gvg_key);
    }

    void instance_base::insert_duel_key(uint32_t vid_src, uint32_t vid_dst) {
        uint32_t pvp_key_value = pvp_key(vid_src, vid_dst);

        set_duel_key.insert(pvp_key_value);
    }

    auto instance_base::pvp_key(uint32_t vid_src, uint32_t vid_dst) -> uint32_t {
        if (vid_src > vid_dst)
            std::swap(vid_src, vid_dst);

        uint32_t src[2];
        src[0] = vid_src;
        src[1] = vid_dst;

        const auto *s = (const uint8_t *)src;
        const uint8_t *end = s + sizeof(src);
        uint32_t h = 0;

        while (s < end) {
            h *= 16777619;
            h ^= (uint8_t)*(uint8_t *)(s++);
        }

        return h;
    }

    auto instance_base::find_pvp_key(uint32_t vid_src, uint32_t vid_dst) -> bool {
        uint32_t pvp_key_value = pvp_key(vid_src, vid_dst);

        if (set_pvp_key.end() == set_pvp_key.find(pvp_key_value))
            return false;

        return true;
    }

    // True when the two guilds are at war.
    auto instance_base::find_gvg_key(uint32_t src_guild_id, uint32_t dst_guild_id) -> bool {
        uint32_t gvg_key = pvp_key(src_guild_id, dst_guild_id);

        if (set_gvg_key.end() == set_gvg_key.find(gvg_key))
            return false;

        return true;
    }
    // True when the two characters are dueling each other.
    auto instance_base::find_duel_key(uint32_t vid_src, uint32_t vid_dst) -> bool {
        uint32_t duel_key = pvp_key(vid_src, vid_dst);

        if (set_duel_key.end() == set_duel_key.find(duel_key))
            return false;

        return true;
    }

    auto instance_base::is_pvp_instance(instance_base &inst_sel) -> bool {
        uint32_t vid_src = virtual_id();
        uint32_t vid_dst = inst_sel.virtual_id();

        uint32_t guild_id_src = guild_id();
        uint32_t guild_id_dst = inst_sel.guild_id();

        if (duel_mode())
            return true;

        return find_pvp_key(vid_src, vid_dst) || find_gvg_key(guild_id_src, guild_id_dst);
    }

    auto instance_base::name_color() -> const math::color & {
        return indexed_name_color(name_color_index());
    }

    auto instance_base::name_color_index() -> uint32_t {
        if (is_pc()) {
            if (is_killer_) {
                return NAMECOLOR_PK;
            }

            if (is_exist_main_instance() && !is_main_instance()) {
                instance_base *inst_main = main_instance_ptr_impl();
                if (!inst_main) {
                    metin2::log::error("Cannot get name color: no main instance");
                    return NAMECOLOR_PC;
                }
                uint32_t vid_main = inst_main->virtual_id();
                uint32_t vid_self = virtual_id();

                if (inst_main->duel_mode()) {
                    switch (inst_main->duel_mode()) {
                    case DUEL_CANNOTATTACK:
                        return NAMECOLOR_PC + empire_id();
                    case DUEL_START:
                        if (find_duel_key(vid_main, vid_self))
                            return NAMECOLOR_PVP;
                        else
                            return NAMECOLOR_PC + empire_id();
                    }
                }

                if (inst_main->is_same_empire(*this)) {
                    if (find_pvp_key(vid_main, vid_self)) {
                        return NAMECOLOR_PVP;
                    }

                    uint32_t guild_id_main = inst_main->guild_id();
                    uint32_t guild_id_self = guild_id();
                    if (find_gvg_key(guild_id_main, guild_id_self)) {
                        return NAMECOLOR_PVP;
                    }
                } else {
                    return NAMECOLOR_PVP;
                }
            }

            abstract_player &player = get_abstract_player();
            if (player.is_party_member_by_vid(virtual_id()))
                return NAMECOLOR_PARTY;

            return NAMECOLOR_PC + empire_id();

        } else if (is_npc()) {
            return NAMECOLOR_NPC;
        } else if (is_enemy()) {
            return NAMECOLOR_MOB;
        } else if (is_poly()) {
            return NAMECOLOR_MOB;
        }

        return math::color(0xffffffff);
    }

    auto instance_base::title_color() -> const math::color & {
        uint32_t grade = alignment_grade();
        if (grade >= TITLE_NUM) {
            static math::color default_title_color(0xffffffff);
            return default_title_color;
        }

        return title_colors[grade];
    }

    void instance_base::attach_text_tail() {
        if (is_text_tail_) {
            metin2::log::error("Character (vid {}): text tail already exists", virtual_id());
            return;
        }

        is_text_tail_ = true;

        uint32_t vid = virtual_id();

        float text_tail_height = is_mounting_horse() ? 110.0f : 10.0f;

        static math::color text_tail_color = math::color(1.0f, 1.0f, 1.0f, 1.0f);
        get_python_text_tail().register_character_text_tail(guild_id_, vid, text_tail_color, text_tail_height);

        if (level_value_) {
            update_text_tail_level(level_value_);
        }
    }

    void instance_base::detach_text_tail() {
        if (!is_text_tail_)
            return;

        is_text_tail_ = false;
        get_python_text_tail().delete_character_text_tail(virtual_id());
    }

    void instance_base::update_text_tail_level(uint32_t level) {
        static math::color level_color = math::color(152.0f / 255.0f, 255.0f / 255.0f, 51.0f / 255.0f, 1.0f);

        char text[256];
        snprintf(text, sizeof(text), "Lv %d", level);
        get_python_text_tail().attach_level(virtual_id(), text, level_color);
    }

    void instance_base::refresh_text_tail() {
        get_python_text_tail().set_character_text_tail_color(virtual_id(), name_color());

        int alignment_grade_value = alignment_grade();
        if (TITLE_NONE == alignment_grade_value) {
            get_python_text_tail().detach_title(virtual_id());
        } else {
            auto itor = title_name_map.find(alignment_grade_value);
            if (title_name_map.end() != itor) {
                const std::string &title_name = itor->second;
                get_python_text_tail().attach_title(virtual_id(), title_name.c_str(), title_color());
            }
        }
    }

    void instance_base::refresh_text_tail_title() {
        refresh_text_tail();
    }

    void instance_base::clear_affect_flag_container() {
        affect_flag_container_value_.clear();
    }

    void instance_base::clear_affects_impl() {
        if (is_stone()) {
            stone_smoke_destroy();
        } else {
            for (int affect = 0; affect < AFFECT_NUM; ++affect) {
                detach_effect(attached_affect_effects_[affect]);
                attached_affect_effects_[affect] = 0;
            }

            clear_affect_flag_container();
        }

        graphic_thing_instance_value_.on_clear_affects_impl();
    }

    void instance_base::set_normal_affect_flag_container(const affect_flag_container &affect_flag_container) {
        for (int i = 0; i < affect_flag_container::BIT_SIZE; ++i) {
            bool is_old_set = affect_flag_container_value_.is_set(i);
            bool is_new_set = affect_flag_container.is_set(i);

            if (is_old_set != is_new_set) {
                set_affect_impl(i, is_new_set);

                if (is_new_set)
                    graphic_thing_instance_value_.on_set_affect_impl(i);
                else
                    graphic_thing_instance_value_.on_reset_affect_impl(i);
            }
        }

        affect_flag_container_value_.copy_instance(affect_flag_container);
    }

    void instance_base::set_stone_smoke_flag_container(const affect_flag_container &affect_flag_container) {
        affect_flag_container_value_.copy_instance(affect_flag_container);

        uint32_t smoke;
        if (affect_flag_container_value_.is_set(STONE_SMOKE8))
            smoke = 3;
        else if (affect_flag_container_value_.is_set(STONE_SMOKE5) || affect_flag_container_value_.is_set(STONE_SMOKE6) || affect_flag_container_value_.is_set(STONE_SMOKE7))
            smoke = 2;
        else if (affect_flag_container_value_.is_set(STONE_SMOKE2) || affect_flag_container_value_.is_set(STONE_SMOKE3) || affect_flag_container_value_.is_set(STONE_SMOKE4))
            smoke = 1;
        else
            smoke = 0;

        stone_smoke_destroy();
        stone_smoke_create(smoke);
    }

    void instance_base::set_affect_flag_container(const affect_flag_container &affect_flag_container) {
        if (is_building()) {
            return;
        } else if (is_stone()) {
            set_stone_smoke_flag_container(affect_flag_container);
        } else {
            set_normal_affect_flag_container(affect_flag_container);
        }
    }

    void instance_base::script_set_affect(uint32_t affect, bool is_visible) {
        set_affect_impl(affect, is_visible);
    }

    void instance_base::assassin_set_eunhyeong_affect(bool is_visible) {
        if (is_visible) {
            // No alpha while wearing a dress.
            if (is_wearing_dress())
                return;

            if (is_main_instance() || main_can_see_hidden_thing()) {
                graphic_thing_instance_value_.blend_alpha_value(0.5f, 1.0f);
            } else {
                // Everyone else cannot see the assassin at all.
                graphic_thing_instance_value_.blend_alpha_value(0.0f, 1.0f);
                graphic_thing_instance_value_.hide_all_attaching_effect();
            }
        } else {
            graphic_thing_instance_value_.blend_alpha_value(1.0f, 1.0f);
            graphic_thing_instance_value_.show_all_attaching_effect();
        }
    }

    void instance_base::warrior_set_geomgyeong_affect(bool is_visible) {
        if (is_visible) {
            if (is_wearing_dress())
                return;

            if (warrior_value_.geomgyeong_effect)
                detach_effect(warrior_value_.geomgyeong_effect);

            graphic_thing_instance_value_.set_reach_scale(1.5f);
            if (graphic_thing_instance_value_.is_two_hand_mode())
                warrior_value_.geomgyeong_effect = attach_effect(EFFECT_WEAPON + static_cast<uint32_t>(WEAPON_TWOHAND));
            else
                warrior_value_.geomgyeong_effect = attach_effect(EFFECT_WEAPON + static_cast<uint32_t>(WEAPON_ONEHAND));
        } else {
            graphic_thing_instance_value_.set_reach_scale(1.0f);

            detach_effect(warrior_value_.geomgyeong_effect);
            warrior_value_.geomgyeong_effect = 0;
        }
    }

    void instance_base::set_affect_impl(uint32_t affect, bool is_visible) {
        switch (affect) {
        case AFFECT_YMIR:
            if (is_affect(AFFECT_INVISIBILITY))
                return;
            break;
        case AFFECT_CHEONGEUN:
            graphic_thing_instance_value_.set_resist_fallen(is_visible);
            break;
        case AFFECT_GEOMGYEONG:
            warrior_set_geomgyeong_affect(is_visible);
            return;
            break;
        case AFFECT_REVIVE_INVISIBILITY:
            assassin_set_eunhyeong_affect(is_visible);
            break;
        case AFFECT_EUNHYEONG:
            assassin_set_eunhyeong_affect(is_visible);
            break;
        case AFFECT_GYEONGGONG:
        case AFFECT_KWAESOK:
            // Gyeonggongsul and Kwaesok are only attached while running.
            if (is_visible)
                if (!is_walking())
                    return;
            break;
        case AFFECT_INVISIBILITY:
            if (is_visible) {
                graphic_thing_instance_value_.clear_attaching_effect();
                effect_container_destroy();
                detach_text_tail();
            } else {
                graphic_thing_instance_value_.blend_alpha_value(1.0f, 1.0f);
                attach_text_tail();
                refresh_text_tail();
            }
            return;
            break;
        case AFFECT_STUN:
            graphic_thing_instance_value_.set_sleep(is_visible);
            break;
        }

        if (affect >= AFFECT_NUM) {
            metin2::log::error("Character (vid {}): affect {} out of range (limit {}, visible {})", virtual_id(), affect, static_cast<int>(AFFECT_NUM), is_visible);
            return;
        }

        if (is_visible) {
            if (!attached_affect_effects_[affect]) {
                attached_affect_effects_[affect] = attach_effect(EFFECT_AFFECT + affect);
            }
        } else {
            if (attached_affect_effects_[affect]) {
                detach_effect(attached_affect_effects_[affect]);
                attached_affect_effects_[affect] = 0;
            }
        }
    }

    auto instance_base::is_possible_emoticon() -> bool {
        effect_manager &eft_mgr = get_effect_manager();
        for (uint32_t emoticon = 0; emoticon < EMOTICON_NUM; emoticon++) {
            uint32_t effect_id = affect_effect_crcs_[EFFECT_EMOTICON + emoticon];
            if (effect_id && eft_mgr.is_alive_effect(effect_id))
                return false;
        }

        if (el_timer_get_m_sec() - emoticon_time_ < 1000) {
            metin2::log::debug("Emoticon still on cooldown");
            return false;
        }

        return true;
    }

    void instance_base::set_fish_emoticon() {
        set_emoticon(EMOTICON_FISH);
    }

    void instance_base::set_emoticon(uint32_t emoticon) {
        if (emoticon >= EMOTICON_NUM) {
            metin2::log::error("Character (vid {}): emoticon {} out of range (limit {})", virtual_id(), emoticon, static_cast<int>(EMOTICON_NUM));
            return;
        }
        if (is_possible_emoticon()) {
            math::vector3 pos = graphic_thing_instance_value_.get_position();
            pos.z += float(graphic_thing_instance_value_.height());

            camera *camera = get_camera_manager().current_camera();

            math::vector3 dir = (camera->eye() - pos) * 9 / 10;
            pos = camera->eye() - dir;

            pos = math::vector3(0, 0, 0);
            pos.z += float(graphic_thing_instance_value_.height());

            graphic_thing_instance_value_.attach_effect_by_id(0, nullptr, affect_effect_crcs_[EFFECT_EMOTICON + emoticon], &pos);
            emoticon_time_ = el_timer_get_m_sec();
        }
    }

    void instance_base::set_dust_gap(float dust_gap) {
        dust_gap_ = dust_gap;
    }

    void instance_base::set_horse_dust_gap(float dust_gap) {
        horse_dust_gap_ = dust_gap;
    }

    void instance_base::detach_effect(uint32_t eid) {
        graphic_thing_instance_value_.dettach_effect(eid);
    }

    auto instance_base::attach_effect(uint32_t eft_type) -> uint32_t {
        if (is_affect(AFFECT_INVISIBILITY))
            return 0;

        if (eft_type >= EFFECT_NUM)
            return 0;

        if (affect_effect_attach_bone_[eft_type].empty()) {
            return graphic_thing_instance_value_.attach_effect_by_id(0, nullptr, affect_effect_crcs_[eft_type]);
        } else {
            std::string &rstr_bone_name = affect_effect_attach_bone_[eft_type];
            const char *c_sz_bone_name;
            // PART_WEAPON is resolved per character because each race names its equipment bones differently.
            if (0 == rstr_bone_name.compare("PART_WEAPON")) {
                if (graphic_thing_instance_value_.attaching_bone_name(race_data::PART_WEAPON, &c_sz_bone_name)) {
                    return graphic_thing_instance_value_.attach_effect_by_id(0, c_sz_bone_name, affect_effect_crcs_[eft_type]);
                }
            } else if (0 == rstr_bone_name.compare("PART_WEAPON_LEFT")) {
                if (graphic_thing_instance_value_.attaching_bone_name(race_data::PART_WEAPON_LEFT, &c_sz_bone_name)) {
                    return graphic_thing_instance_value_.attach_effect_by_id(0, c_sz_bone_name, affect_effect_crcs_[eft_type]);
                }
            } else {
                return graphic_thing_instance_value_.attach_effect_by_id(0, rstr_bone_name.c_str(), affect_effect_crcs_[eft_type]);
            }
        }

        return 0;
    }

    void instance_base::combo_process_impl() {
    }

    auto instance_base::register_effect(uint32_t eft_type, const char *eft_attach_bone, const char *eft_name, bool is_cache) -> bool {
        if (eft_type >= EFFECT_NUM)
            return false;

        affect_effect_attach_bone_[eft_type] = eft_attach_bone;

        uint32_t &crc_eft = affect_effect_crcs_[eft_type];
        if (!get_effect_manager().register_effect2(eft_name, &crc_eft, is_cache)) {
            metin2::log::error("Cannot register effect {} (bone {}, file {}, cache {})", eft_type, eft_attach_bone, eft_name, is_cache);
            crc_eft = 0;
            return false;
        }

        return true;
    }

    void instance_base::register_title_name(int index, const char *title_name) {
        title_name_map.insert(std::make_pair(index, title_name));
    }

    auto rgb_to_color(uint32_t r, uint32_t g, uint32_t b) -> math::color {
        uint32_t color = 0xff;
        color <<= 8;
        color |= r;
        color <<= 8;
        color |= g;
        color <<= 8;
        color |= b;

        return math::color(color);
    }

    auto instance_base::register_name_color(uint32_t index, uint32_t r, uint32_t g, uint32_t b) -> bool {
        if (index >= NAMECOLOR_NUM)
            return false;

        name_colors[index] = rgb_to_color(r, g, b);
        return true;
    }

    auto instance_base::register_title_color(uint32_t index, uint32_t r, uint32_t g, uint32_t b) -> bool {
        if (index >= TITLE_NUM)
            return false;

        title_colors[index] = rgb_to_color(r, g, b);
        return true;
    }
}
