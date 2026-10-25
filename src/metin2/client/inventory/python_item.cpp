#include <metin2/client/background/python_background.h>
#include <metin2/core/random.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/geometry/grp_math.h>
#include <metin2/log/log.h>
#include <metin2/world/items/item_manager.h>

#include <metin2/client/social/python_text_tail.h>
#include <metin2/core/timer.h>
#include <metin2/ui/python_graphic.h>
#include "python_item.h"

namespace metin2::inline client::inline inventory {

    std::string python_item::ground_item_instance::drop_sound_file_name[DROPSOUND_NUM];

    void python_item::get_info(std::string *pst_info) {
        char sz_info[256];
        snprintf(sz_info, sizeof(sz_info), "Item: Inst %zu, Pool %d", ground_item_instance_map_value_.size(), ground_item_instance_pool_.capacity());

        pst_info->append(sz_info);
    }

    void python_item::ground_item_instance::clear() {
        ownership = "";
        thing_instance.clear();
        get_effect_manager().destroy_effect_instance(effect_instance_index);
    }

    void python_item::ground_item_instance::play_drop_sound_impl(uint32_t item_type, const math::vector3 &pos) {
        if (item_type >= DROPSOUND_NUM)
            return;

        get_sound_manager().play_sound3d(pos.x, pos.y, pos.z, drop_sound_file_name[item_type].c_str());
    }

    auto python_item::ground_item_instance::update() -> bool {
        if (anim_ended)
            return false;
        if (end_time < get_timer().current_millisecond()) {
            thing_instance.set_rotation_quaternion(end);

            math::quaternion adjust(-center.x, -center.y, -center.z, 0.0f);
            math::quaternion qc;
            qc = math::conjugate(end);
            adjust = adjust * end;
            adjust = qc * adjust;

            thing_instance.set_position(end_position.x + adjust.x,
                end_position.y + adjust.y,
                end_position.z + adjust.z);
            anim_ended = true;

            play_drop_sound_impl(drop_sound_type, end_position);
        } else {
            uint32_t time = get_timer().current_millisecond() - start_time;
            uint32_t etime = end_time - get_timer().current_millisecond();
            float rate = time * 1.0f / (end_time - start_time);

            math::vector3 new_position = end_position;
            new_position.z += 100 - 100 * rate * (3 * rate - 2);

            math::quaternion q;
            q = math::quaternion_rotation_axis(rotation_axis, etime * 0.03f * (-1 + rate * (3 * rate - 2)));
            q = end * q;

            thing_instance.set_rotation_quaternion(q);
            math::quaternion adjust(-center.x, -center.y, -center.z, 0.0f);
            math::quaternion qc;
            qc = math::conjugate(q);
            adjust = adjust * q;
            adjust = qc * adjust;

            thing_instance.set_position(new_position.x + adjust.x,
                new_position.y + adjust.y,
                new_position.z + adjust.z);
        }
        thing_instance.transform();
        thing_instance.deform();
        return !anim_ended;
    }

    void python_item::update(int mouse_x, int mouse_y) {
        auto itor = ground_item_instance_map_value_.begin();
        for (; itor != ground_item_instance_map_value_.end(); ++itor) {
            itor->second->update();
        }

        picked_item_id_ = pick_impl(mouse_x, mouse_y);
    }

    void python_item::render() {
        get_python_graphic().set_diffuse_operation();
        auto itor = ground_item_instance_map_value_.begin();
        for (; itor != ground_item_instance_map_value_.end(); ++itor) {
            graphic_thing_instance &instance = itor->second->thing_instance;
            instance.render();
            instance.blend_render();
        }
    }

    void python_item::set_use_sound_file_name(uint32_t item_type, const std::string &file_name) {
        if (item_type >= USESOUND_NUM)
            return;

        use_sound_file_name_[item_type] = file_name;
    }

    void python_item::set_drop_sound_file_name(uint32_t item_type, const std::string &file_name) {
        if (item_type >= DROPSOUND_NUM)
            return;

        metin2::log::debug("Item type {}: drop sound {}", item_type, file_name.c_str());

        ground_item_instance::drop_sound_file_name[item_type] = file_name;
    }

    void python_item::play_use_sound(uint32_t item_id) {

        item_data *item_data;
        if (!get_item_manager().item_data_pointer(item_id, &item_data))
            return;

        uint32_t item_type = use_sound_type(*item_data);
        if (item_type == USESOUND_NONE)
            return;
        if (item_type >= USESOUND_NUM)
            return;

        get_sound_manager().play_sound2d(use_sound_file_name_[item_type].c_str());
    }

    void python_item::play_drop_sound(uint32_t item_id) {

        item_data *item_data;
        if (!get_item_manager().item_data_pointer(item_id, &item_data))
            return;

        uint32_t item_type = drop_sound_type(*item_data);
        if (item_type >= DROPSOUND_NUM)
            return;

        get_sound_manager().play_sound2d(ground_item_instance::drop_sound_file_name[item_type].c_str());
    }

    void python_item::play_use_potion_sound() {
        get_sound_manager().play_sound2d(use_sound_file_name_[USESOUND_POTION].c_str());
    }

    auto python_item::drop_sound_type(const item_data &item_data) -> uint32_t {
        switch (item_data.get_type()) {
        case item_data::ITEM_TYPE_WEAPON:
            switch (item_data.weapon_type()) {
            case item_data::WEAPON_BOW:
                return DROPSOUND_BOW;
                break;
            case item_data::WEAPON_ARROW:
                return DROPSOUND_DEFAULT;
                break;
            default:
                return DROPSOUND_WEAPON;
                break;
            }
            break;
        case item_data::ITEM_TYPE_ARMOR:
            switch (item_data.sub_type()) {
            case item_data::ARMOR_NECK:
            case item_data::ARMOR_EAR:
                return DROPSOUND_ACCESSORY;
                break;
            case item_data::ARMOR_BODY:
                return DROPSOUND_ARMOR;
            default:
                return DROPSOUND_DEFAULT;
                break;
            }
            break;
        default:
            return DROPSOUND_DEFAULT;
            break;
        }

        return DROPSOUND_DEFAULT;
    }

    auto python_item::use_sound_type(const item_data &item_data) -> uint32_t {
        switch (item_data.get_type()) {
        case item_data::ITEM_TYPE_WEAPON:
            switch (item_data.weapon_type()) {
            case item_data::WEAPON_BOW:
                return USESOUND_BOW;
                break;
            case item_data::WEAPON_ARROW:
                return USESOUND_DEFAULT;
                break;
            default:
                return USESOUND_WEAPON;
                break;
            }
            break;
        case item_data::ITEM_TYPE_ARMOR:
            switch (item_data.sub_type()) {
            case item_data::ARMOR_NECK:
            case item_data::ARMOR_EAR:
                return USESOUND_ACCESSORY;
                break;
            case item_data::ARMOR_BODY:
                return USESOUND_ARMOR;
            default:
                return USESOUND_DEFAULT;
                break;
            }
            break;
        case item_data::ITEM_TYPE_USE:
            switch (item_data.sub_type()) {
            case item_data::USE_ABILITY_UP:
                return USESOUND_POTION;
                break;
            case item_data::USE_POTION:
                return USESOUND_NONE;
                break;
            case item_data::USE_TALISMAN:
                return USESOUND_PORTAL;
                break;
            default:
                return USESOUND_DEFAULT;
                break;
            }
            break;
        default:
            return USESOUND_DEFAULT;
            break;
        }

        return USESOUND_DEFAULT;
    }

    void python_item::create_item(uint32_t virtual_id, uint32_t virtual_number, float x, float y, float z, bool drop) {

        item_data *item_data;
        if (!get_item_manager().item_data_pointer(virtual_number, &item_data))
            return;

        graphic_thing *item_model = item_data->drop_model_thing();

        ground_item_instance *ground_item_instance_value = ground_item_instance_pool_.alloc();
        ground_item_instance_value->virtual_number = virtual_number;

        bool stab_ground = false;

        if (drop) {
            z = get_python_background().height(x, y) + 10.0f;

            if (item_data->get_type() == item_data::ITEM_TYPE_WEAPON &&
                (item_data->weapon_type() == item_data::WEAPON_SWORD ||
                    item_data->weapon_type() == item_data::WEAPON_ARROW))
                stab_ground = true;

            stab_ground = false;
            ground_item_instance_value->anim_ended = false;
        } else {
            ground_item_instance_value->anim_ended = true;
        }

        {
            effect_manager &rem = get_effect_manager();
            ground_item_instance_value->effect_instance_index =
                rem.create_effect(drop_item_effect_id_, math::vector3(x, -y, z), math::vector3(0, 0, 0));

            ground_item_instance_value->drop_sound_type = drop_sound_type(*item_data);
        }

        math::vector3 normal;
        if (!get_python_background().get_normal(int(x), int(y), &normal))
            normal = math::vector3(0.0f, 0.0f, 1.0f);

        ground_item_instance_value->thing_instance.clear();
        ground_item_instance_value->thing_instance.reserve_model_thing(1);
        ground_item_instance_value->thing_instance.reserve_model_instance(1);
        ground_item_instance_value->thing_instance.register_model_thing(0, item_model);
        ground_item_instance_value->thing_instance.set_model_instance(0, 0, 0);
        if (drop) {
            ground_item_instance_value->end_position = math::vector3(x, -y, z);
            ground_item_instance_value->thing_instance.set_position(0, 0, 0);
        } else
            ground_item_instance_value->thing_instance.set_position(x, -y, z);

        ground_item_instance_value->thing_instance.update();
        ground_item_instance_value->thing_instance.transform();
        ground_item_instance_value->thing_instance.deform();

        if (drop) {
            math::vector3 v_min, v_max;
            ground_item_instance_value->thing_instance.get_bound_box(&v_min, &v_max);
            ground_item_instance_value->center = (v_min + v_max) * 0.5f;

            std::pair<float, int> f[3] =
                {
                    std::make_pair(v_max.x - v_min.x, 0),
                    std::make_pair(v_max.y - v_min.y, 1),
                    std::make_pair(v_max.z - v_min.z, 2)};

            std::sort(f, f + 3);

            math::vector3 end;

            if (stab_ground) {
                // Stuck into the ground along its longest axis.
                if (f[2].second == 0) // axis x
                {
                    end.y = 90.0f + metin2::random::fnumber(-15.0f, 15.0f);
                    end.x = metin2::random::fnumber(0.0f, 360.0f);
                    end.z = metin2::random::fnumber(-15.0f, 15.0f);
                } else if (f[2].second == 1) // axis y
                {
                    end.y = metin2::random::fnumber(0.0f, 360.0f);
                    end.x = metin2::random::fnumber(-15.0f, 15.0f);
                    end.z = 180.0f + metin2::random::fnumber(-15.0f, 15.0f);
                } else // axis z
                {
                    end.y = 180.0f + metin2::random::fnumber(-15.0f, 15.0f);
                    end.x = 0.0f + metin2::random::fnumber(-15.0f, 15.0f);
                    end.z = metin2::random::fnumber(0.0f, 360.0f);
                }
            } else {
                // Lying flat, following the ground normal.
                if (f[0].second == 0) {
                    ground_item_instance_value->end =
                        rotation_arc(
                            math::vector3(
                                static_cast<float>(metin2::random::number(0, 1)) * 2 - 1 + metin2::random::fnumber(-0.1f, 0.1f),
                                0 + metin2::random::fnumber(-0.1f, 0.1f),
                                0 + metin2::random::fnumber(-0.1f, 0.1f)),
                            math::vector3(0, 0, 1));
                } else if (f[0].second == 1) {
                    ground_item_instance_value->end =
                        rotation_arc(
                            math::vector3(
                                0 + metin2::random::fnumber(-0.1f, 0.1f),
                                static_cast<float>(metin2::random::number(0, 1)) * 2 - 1 + metin2::random::fnumber(-0.1f, 0.1f),
                                0 + metin2::random::fnumber(-0.1f, 0.1f)),
                            math::vector3(0, 0, 1));
                } else {
                    ground_item_instance_value->end =
                        rotation_arc(
                            math::vector3(
                                0 + metin2::random::fnumber(-0.1f, 0.1f),
                                0 + metin2::random::fnumber(-0.1f, 0.1f),
                                static_cast<float>(metin2::random::number(0, 1)) * 2 - 1 + metin2::random::fnumber(-0.1f, 0.1f)),
                            math::vector3(0, 0, 1));
                }
            }
            float rot = metin2::random::fnumber(0, 2 * 3.1415926535f);
            math::quaternion q(0, 0, cosf(rot), sinf(rot));
            ground_item_instance_value->end = ground_item_instance_value->end * q;
            q = rotation_arc(math::vector3(0, 0, 1), normal);
            ground_item_instance_value->end = ground_item_instance_value->end * q;

            ground_item_instance_value->start_time = get_timer().current_millisecond();
            ground_item_instance_value->end_time = ground_item_instance_value->start_time + 300;
            ground_item_instance_value->rotation_axis.x = sinf(rot + 0);
            ground_item_instance_value->rotation_axis.y = cosf(rot + 0);
            ground_item_instance_value->rotation_axis.z = 0;

            math::vector3 adjust = -ground_item_instance_value->center;
            math::matrix mat;
            mat = math::matrix_rotation_quaternion(ground_item_instance_value->end);

            adjust = math::transform_coord(adjust, mat);
        }

        ground_item_instance_value->thing_instance.show();

        ground_item_instance_map_value_.insert(ground_item_instance_map::value_type(virtual_id, ground_item_instance_value));

        python_text_tail &text_tail = get_python_text_tail();
        text_tail.register_item_text_tail(
            virtual_id,
            item_data->get_name(),
            &ground_item_instance_value->thing_instance);
    }

    void python_item::set_ownership(uint32_t vid, const char *name) {
        auto itor = ground_item_instance_map_value_.find(vid);

        if (ground_item_instance_map_value_.end() == itor)
            return;

        ground_item_instance *ground_item_instance_value = itor->second;
        ground_item_instance_value->ownership.assign(name);

        python_text_tail &text_tail = get_python_text_tail();
        text_tail.set_item_text_tail_owner(vid, name);
    }

    auto python_item::ownership(uint32_t vid, const char **name) -> bool {
        auto itor = ground_item_instance_map_value_.find(vid);

        if (ground_item_instance_map_value_.end() == itor)
            return false;

        ground_item_instance *ground_item_instance_value = itor->second;
        *name = ground_item_instance_value->ownership.c_str();

        return true;
    }

    void python_item::delete_all_items() {
        python_text_tail &text_tail = get_python_text_tail();

        ground_item_instance_map::iterator i;
        for (i = ground_item_instance_map_value_.begin(); i != ground_item_instance_map_value_.end(); ++i) {
            ground_item_instance *ground_item_inst = i->second;
            text_tail.delete_item_text_tail(i->first);
            ground_item_inst->clear();
            ground_item_instance_pool_.free(ground_item_inst);
        }
        ground_item_instance_map_value_.clear();
    }

    void python_item::delete_item(uint32_t virtual_id) {
        auto itor = ground_item_instance_map_value_.find(virtual_id);
        if (ground_item_instance_map_value_.end() == itor)
            return;

        ground_item_instance *ground_item_instance_value = itor->second;
        ground_item_instance_value->clear();
        ground_item_instance_pool_.free(ground_item_instance_value);
        ground_item_instance_map_value_.erase(itor);

        get_python_text_tail().delete_item_text_tail(virtual_id);
    }

    // Approximate distance used for the pickup range; fixes the pickup distance bug, see
    // https://metin2.dev/board/topic/24691-fix-pickup-distances-bug/
    auto distance_approx(int dx, int dy) -> int {
        int min_value, max_value;

        if (dx < 0)
            dx = -dx;

        if (dy < 0)
            dy = -dy;

        if (dx < dy) {
            min_value = dx;
            max_value = dy;
        } else {
            min_value = dy;
            max_value = dx;
        }

        // coefficients equivalent to ( 123/128 * max ) and ( 51/128 * min )
        return (((max_value << 8) + (max_value << 3) - (max_value << 4) - (max_value << 1) +
                    (min_value << 7) - (min_value << 5) + (min_value << 3) - (min_value << 1)) >>
                8);
    }

    auto python_item::close_item(const pixel_position &pixel_position, uint32_t *item_id, uint32_t distance) -> bool {
        uint32_t close_item_id = 0;
        uint32_t close_item_distance = 0;
        int apro_min = 0;

        ground_item_instance_map::iterator i;
        for (i = ground_item_instance_map_value_.begin(); i != ground_item_instance_map_value_.end(); ++i) {
            ground_item_instance *instance = i->second;

            int dist = distance_approx((int)pixel_position.x - (int)instance->end_position.x, (int)pixel_position.y - (-(int)instance->end_position.y));
            if (apro_min == 0)
                apro_min = dist;
            if (apro_min >= dist) {
                apro_min = dist;
                close_item_id = i->first;
                close_item_distance = dist;
            }
        }

        if (close_item_distance > 300)
            return false;

        *item_id = close_item_id;

        return true;
    }

    auto python_item::ground_item_position(uint32_t virtual_id, pixel_position *position) -> bool {
        auto itor = ground_item_instance_map_value_.find(virtual_id);
        if (ground_item_instance_map_value_.end() == itor)
            return false;

        ground_item_instance *instance = itor->second;

        const math::vector3 &item_position = instance->thing_instance.get_position();

        position->x = +item_position.x;
        position->y = -item_position.y;
        position->z = +item_position.z;

        return true;
    }

    auto python_item::pick_impl(int mouse_x, int mouse_y) -> uint32_t {
        float fu, fv, ft;

        auto itor = ground_item_instance_map_value_.begin();
        for (; itor != ground_item_instance_map_value_.end(); ++itor) {
            ground_item_instance *instance = itor->second;

            if (instance->thing_instance.intersect(&fu, &fv, &ft)) {
                return itor->first;
            }
        }

        python_text_tail &text_tail_mgr = get_python_text_tail();
        return text_tail_mgr.pick(mouse_x, mouse_y);
    }

    auto python_item::picked_item_id(uint32_t *picked_item_id) -> bool {
        if (INVALID_ID == picked_item_id_)
            return false;

        *picked_item_id = picked_item_id_;
        return true;
    }

    auto python_item::virtual_number_of_ground_item(uint32_t vid) -> uint32_t {
        auto itor = ground_item_instance_map_value_.find(vid);

        if (itor == ground_item_instance_map_value_.end())
            return 0;
        else
            return itor->second->virtual_number;
    }

    void python_item::destroy() {
        delete_all_items();
        ground_item_instance_pool_.clear();
    }

    void python_item::create() {
        get_effect_manager().register_effect2("effect:etc/dropitem/dropitem.toml", &drop_item_effect_id_);
    }

    python_item::python_item() {
        ground_item_instance_pool_.set_name("CDynamicPool<TGroundItemInstance>");
        picked_item_id_ = INVALID_ID;
    }

    python_item::~python_item() {
        assert(ground_item_instance_map_value_.empty());
    }
}
