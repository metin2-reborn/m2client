#include "python_text_tail.h"
#include <metin2/client/app/locale.h>
#include <metin2/client/app/resource_ids.h>
#include <metin2/client/characters/instance_base.h>
#include <metin2/client/characters/python_character_manager.h>
#include <metin2/client/guilds/mark_manager.h>
#include <metin2/client/guilds/python_guild.h>
#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/log/log.h>
#include <metin2/ui/python_graphic.h>

namespace metin2::inline client::inline social {

    const math::color c_TextTail_Item_Color = math::color(1.0f, 1.0f, 1.0f, 1.0f);
    const math::color c_TextTail_Chat_Color = math::color(1.0f, 1.0f, 1.0f, 1.0f);
    const math::color c_TextTail_Info_Color = math::color(1.0f, 0.785f, 0.785f, 1.0f);
    const math::color c_TextTail_Guild_Name_Color = 0xFFEFD3FF;
    const float c_TextTail_Name_Position = -10.0f;
    const float c_fxMarkPosition = 1.5f;
    const float c_fyGuildNamePosition = 15.0f;
    const float c_fyMarkPosition = 15.0f + 11.0f;
    bool pk_title_enable = true;

    int32_t text_tail_living_time = 5000;

    auto text_tail_get_living_time() -> int32_t {
        assert(text_tail_living_time > 1000);
        return text_tail_living_time;
    }

    void text_tail_set_living_time(int32_t living_time) {
        text_tail_living_time = living_time;
    }

    graphic_text *font = nullptr;

    void python_text_tail::get_info(std::string *pst_info) {
        char sz_info[256];
        snprintf(sz_info, sizeof(sz_info), "TextTail: ChatTail %zu, ChrTail (Map %zu, List %zu), ItemTail (Map %zu, List %zu), Pool %d",
            chat_tail_map_value_.size(),
            character_text_tail_map_.size(), character_text_tail_list_.size(),
            item_text_tail_map_.size(), item_text_tail_list_.size(),
            text_tail_pool_.capacity());

        pst_info->append(sz_info);
    }

    void python_text_tail::update_all_text_tail() {
        instance_base *instance = get_python_character_manager().main_instance_ptr();
        if (instance) {
            pixel_position pixel_pos;
            instance->new_get_pixel_position(&pixel_pos);

            text_tail_map::iterator itor_map;

            for (itor_map = character_text_tail_map_.begin(); itor_map != character_text_tail_map_.end(); ++itor_map) {
                update_distance(pixel_pos, itor_map->second);
            }

            for (itor_map = item_text_tail_map_.begin(); itor_map != item_text_tail_map_.end(); ++itor_map) {
                update_distance(pixel_pos, itor_map->second);
            }

            for (auto itor_chat = chat_tail_map_value_.begin(); itor_chat != chat_tail_map_value_.end(); ++itor_chat) {
                update_distance(pixel_pos, itor_chat->second);

                // With a chat text tail the character name is shown too.
                if (itor_chat->second->name_flag) {
                    uint32_t vid = itor_chat->first;
                    show_character_text_tail(vid);
                }
            }
        }
    }

    void python_text_tail::update_showing_text_tail() {
        text_tail_list::iterator itor;

        for (itor = item_text_tail_list_.begin(); itor != item_text_tail_list_.end(); ++itor) {
            update_text_tail(*itor);
        }

        for (auto itor_chat = chat_tail_map_value_.begin(); itor_chat != chat_tail_map_value_.end(); ++itor_chat) {
            update_text_tail(itor_chat->second);
        }

        for (itor = character_text_tail_list_.begin(); itor != character_text_tail_list_.end(); ++itor) {
            text_tail *text_tail_value = *itor;
            update_text_tail(text_tail_value);

            // A chat text tail moves along with it.
            auto itor = chat_tail_map_value_.find(text_tail_value->virtual_id);
            if (chat_tail_map_value_.end() != itor) {
                text_tail *chat_tail = itor->second;
                if (chat_tail->name_flag) {
                    text_tail_value->y = chat_tail->y - 17.0f;
                }
            }
        }
    }

    void python_text_tail::update_text_tail(text_tail *text_tail_value) {
        if (!text_tail_value->owner)
            return;

        python_graphic &rpy_graphic = get_python_graphic();
        rpy_graphic.identity();

        const math::vector3 &position = text_tail_value->owner->get_position();
        rpy_graphic.project_position(position.x,
            position.y,
            position.z + text_tail_value->height,
            &text_tail_value->x,
            &text_tail_value->y,
            &text_tail_value->z);

        text_tail_value->x = floorf(text_tail_value->x);
        text_tail_value->y = floorf(text_tail_value->y);

        // No depth within 13 m.
        if (text_tail_value->distance_from_player < 1300.0f) {
            text_tail_value->z = 0.0f;
        } else {
            text_tail_value->z = text_tail_value->z * get_python_graphic().ortho_depth() * -1.0f;
            text_tail_value->z += 10.0f;
        }
    }

    void python_text_tail::arrange_text_tail() {
        text_tail_list::iterator itor;
        text_tail_list::iterator itor_compare;

        uint32_t time = get_timer().current_millisecond();

        for (itor = item_text_tail_list_.begin(); itor != item_text_tail_list_.end(); ++itor) {
            text_tail *insert_text_tail = *itor;

            int y_temp = 5;
            int limit_count = 0;

            for (itor_compare = item_text_tail_list_.begin(); itor_compare != item_text_tail_list_.end();) {
                text_tail *compare_text_tail = *itor_compare;

                if (*itor_compare == *itor) {
                    ++itor_compare;
                    continue;
                }

                if (limit_count >= 20)
                    break;

                if (is_in(insert_text_tail, compare_text_tail)) {
                    insert_text_tail->y = (compare_text_tail->y + compare_text_tail->y_end + y_temp);

                    itor_compare = item_text_tail_list_.begin();
                    ++limit_count;
                    continue;
                }

                ++itor_compare;
            }

            if (insert_text_tail->owner_text_instance) {
                insert_text_tail->owner_text_instance->set_position(insert_text_tail->x, insert_text_tail->y, insert_text_tail->z);
                insert_text_tail->owner_text_instance->update();

                insert_text_tail->text_instance->set_color(insert_text_tail->color_value.r, insert_text_tail->color_value.g, insert_text_tail->color_value.b);
                insert_text_tail->text_instance->set_position(insert_text_tail->x, insert_text_tail->y + 15.0f, insert_text_tail->z);
                insert_text_tail->text_instance->update();

            } else {
                insert_text_tail->text_instance->set_color(insert_text_tail->color_value.r, insert_text_tail->color_value.g, insert_text_tail->color_value.b);
                insert_text_tail->text_instance->set_position(insert_text_tail->x, insert_text_tail->y, insert_text_tail->z);
                insert_text_tail->text_instance->update();
            }
        }

        for (itor = character_text_tail_list_.begin(); itor != character_text_tail_list_.end(); ++itor) {
            text_tail *text_tail_value = *itor;

            float fx_add = 0.0f;

            graphic_mark_instance *mark_instance = text_tail_value->mark_instance;
            graphic_text_instance *guild_name_instance = text_tail_value->guild_name_text_instance;
            if (mark_instance && guild_name_instance) {
                int width_value, height;
                int image_half_size = mark_instance->width() / 2 + c_fxMarkPosition;
                guild_name_instance->text_size(&width_value, &height);

                mark_instance->set_position(text_tail_value->x - width_value / 2 - image_half_size, text_tail_value->y - c_fyMarkPosition);
                guild_name_instance->set_position(text_tail_value->x + image_half_size, text_tail_value->y - c_fyGuildNamePosition, text_tail_value->z);
                guild_name_instance->update();
            }

            int name_width, name_height;
            text_tail_value->text_instance->text_size(&name_width, &name_height);

            graphic_text_instance *title = text_tail_value->title_text_instance;
            if (title) {
                int title_width, title_height;
                title->text_size(&title_width, &title_height);

                fx_add = 8.0f;

                title->set_position(text_tail_value->x - ((name_width / 2) - 4.0f), text_tail_value->y, text_tail_value->z); // space between level and alignment

                title->update();

                graphic_text_instance *level = text_tail_value->level_text_instance;
                if (level) {
                    int level_width, level_height;
                    level->text_size(&level_width, &level_height);

                    level->set_position(text_tail_value->x - (name_width / 2) - title_width, text_tail_value->y, text_tail_value->z);

                    level->update();
                }
            } else {
                fx_add = 4.0f;

                graphic_text_instance *level = text_tail_value->level_text_instance;
                if (level) {
                    int level_width, level_height;
                    level->text_size(&level_width, &level_height);

                    level->set_position(text_tail_value->x - (name_width / 2), text_tail_value->y, text_tail_value->z);

                    level->update();
                }
            }

            text_tail_value->text_instance->set_color(text_tail_value->color_value.r, text_tail_value->color_value.g, text_tail_value->color_value.b);
            text_tail_value->text_instance->set_position(text_tail_value->x + fx_add, text_tail_value->y, text_tail_value->z);
            text_tail_value->text_instance->update();
        }

        for (auto itor_chat = chat_tail_map_value_.begin(); itor_chat != chat_tail_map_value_.end();) {
            text_tail *text_tail_value = itor_chat->second;

            if (text_tail_value->living_time < time) {
                delete_text_tail(text_tail_value);
                itor_chat = chat_tail_map_value_.erase(itor_chat);
                continue;
            } else
                ++itor_chat;

            text_tail_value->text_instance->set_color(text_tail_value->color_value);
            text_tail_value->text_instance->set_position(text_tail_value->x, text_tail_value->y, text_tail_value->z);
            text_tail_value->text_instance->update();
        }
    }

    void python_text_tail::render() {
        text_tail_list::iterator itor;

        for (itor = character_text_tail_list_.begin(); itor != character_text_tail_list_.end(); ++itor) {
            text_tail *text_tail_value = *itor;
            text_tail_value->text_instance->render();
            if (text_tail_value->mark_instance && text_tail_value->guild_name_text_instance) {
                text_tail_value->mark_instance->render();
                text_tail_value->guild_name_text_instance->render();
            }
            if (text_tail_value->title_text_instance) {
                text_tail_value->title_text_instance->render();
            }
            if (text_tail_value->level_text_instance) {
                text_tail_value->level_text_instance->render();
            }
        }

        for (itor = item_text_tail_list_.begin(); itor != item_text_tail_list_.end(); ++itor) {
            text_tail *text_tail_value = *itor;

            render_text_tail_box(text_tail_value);
            text_tail_value->text_instance->render();
            if (text_tail_value->owner_text_instance)
                text_tail_value->owner_text_instance->render();
        }

        for (auto itor_chat = chat_tail_map_value_.begin(); itor_chat != chat_tail_map_value_.end(); ++itor_chat) {
            text_tail *text_tail_value = itor_chat->second;
            if (text_tail_value->owner->is_show())
                render_text_tail_name(text_tail_value);
        }
    }

    void python_text_tail::render_text_tail_box(text_tail *text_tail_value) {
        get_python_graphic().set_diffuse_color(0.0f, 0.0f, 0.0f, 1.0f);
        get_python_graphic().render_box2d(text_tail_value->x + text_tail_value->x_start,
            text_tail_value->y + text_tail_value->y_start,
            text_tail_value->x + text_tail_value->x_end,
            text_tail_value->y + text_tail_value->y_end,
            text_tail_value->z);

        get_python_graphic().set_diffuse_color(0.0f, 0.0f, 0.0f, 0.3f);
        get_python_graphic().render_bar2d(text_tail_value->x + text_tail_value->x_start,
            text_tail_value->y + text_tail_value->y_start,
            text_tail_value->x + text_tail_value->x_end,
            text_tail_value->y + text_tail_value->y_end,
            text_tail_value->z);
    }

    void python_text_tail::render_text_tail_name(text_tail *text_tail_value) {
        text_tail_value->text_instance->render();
    }

    void python_text_tail::hide_all_text_tail() {
        // Calling show all without a later hide all may cause trouble.
        character_text_tail_list_.clear();
        item_text_tail_list_.clear();
    }

    void python_text_tail::update_distance(const pixel_position &center_position, text_tail *text_tail_value) {
        const math::vector3 &position = text_tail_value->owner->get_position();
        math::vector2 distance(position.x - center_position.x, -position.y - center_position.y);
        text_tail_value->distance_from_player = math::length(distance);
    }

    void python_text_tail::show_all_text_tail() {
        text_tail_map::iterator itor;
        for (itor = character_text_tail_map_.begin(); itor != character_text_tail_map_.end(); ++itor) {
            text_tail *text_tail_value = itor->second;
            if (text_tail_value->distance_from_player < 3500.0f)
                show_character_text_tail(itor->first);
        }
        for (itor = item_text_tail_map_.begin(); itor != item_text_tail_map_.end(); ++itor) {
            text_tail *text_tail_value = itor->second;
            if (text_tail_value->distance_from_player < 3500.0f)
                show_item_text_tail(itor->first);
        }
    }

    void python_text_tail::show_character_text_tail(uint32_t virtual_id) {
        auto itor = character_text_tail_map_.find(virtual_id);

        if (character_text_tail_map_.end() == itor)
            return;

        text_tail *text_tail_value = itor->second;

        if (character_text_tail_list_.end() != std::find(character_text_tail_list_.begin(), character_text_tail_list_.end(), text_tail_value)) {
            return;
        }

        // Looking up every instance here may be costly; ideally the instance would own its text tail.
        if (!text_tail_value->owner->is_show())
            return;

        instance_base *instance = get_python_character_manager().instance_ptr(text_tail_value->virtual_id);
        if (!instance)
            return;

        if (instance->is_guild_wall())
            return;

        if (instance->can_pick_instance())
            character_text_tail_list_.push_back(text_tail_value);
    }

    void python_text_tail::show_item_text_tail(uint32_t virtual_id) {
        auto itor = item_text_tail_map_.find(virtual_id);

        if (item_text_tail_map_.end() == itor)
            return;

        text_tail *text_tail_value = itor->second;

        if (item_text_tail_list_.end() != std::find(item_text_tail_list_.begin(), item_text_tail_list_.end(), text_tail_value)) {
            return;
        }

        item_text_tail_list_.push_back(text_tail_value);
    }

    auto python_text_tail::is_in(python_text_tail::text_tail *source, python_text_tail::text_tail *target) -> bool {
        float x1_source = source->x + source->x_start;
        float y1_source = source->y + source->y_start;
        float x2_source = source->x + source->x_end;
        float y2_source = source->y + source->y_end;
        float x1_target = target->x + target->x_start;
        float y1_target = target->y + target->y_start;
        float x2_target = target->x + target->x_end;
        float y2_target = target->y + target->y_end;

        if (x1_source <= x2_target && x2_source >= x1_target &&
            y1_source <= y2_target && y2_source >= y1_target) {
            return true;
        }

        return false;
    }

    void python_text_tail::register_character_text_tail(uint32_t guild_id, uint32_t virtual_id, const math::color &color, float add_height) {
        instance_base *character_instance = get_python_character_manager().instance_ptr(virtual_id);

        if (!character_instance)
            return;

        text_tail *text_tail_value = register_text_tail(virtual_id,
            character_instance->name_string(),
            character_instance->graphic_thing_instance_ptr(),
            character_instance->graphic_thing_instance_ref().height() + add_height,
            color);

        graphic_text_instance *text_instance = text_tail_value->text_instance;
        text_instance->set_outline(true);
        text_instance->set_vertical_align(graphic_text_instance::VERTICAL_ALIGN_BOTTOM);

        text_tail_value->mark_instance = nullptr;
        text_tail_value->guild_name_text_instance = nullptr;
        text_tail_value->title_text_instance = nullptr;
        text_tail_value->level_text_instance = nullptr;

        if (0 != guild_id) {
            text_tail_value->mark_instance = graphic_mark_instance::new_value();

            uint32_t mark_id = get_guild_mark_manager().mark_id(guild_id);

            if (mark_id != guild_mark_manager::INVALID_MARK_ID) {
                std::string mark_image_path;

                if (get_guild_mark_manager().mark_image_filename(mark_id / guild_mark_image::MARK_TOTAL_COUNT, mark_image_path)) {
                    text_tail_value->mark_instance->set_image_file_name(mark_image_path.c_str());
                    text_tail_value->mark_instance->load();
                    text_tail_value->mark_instance->set_index(mark_id % guild_mark_image::MARK_TOTAL_COUNT);
                }
            }

            std::string guild_name;
            if (!get_python_guild().guild_name(guild_id, &guild_name))
                guild_name = "Noname";

            graphic_text_instance *&guild_name_instance = text_tail_value->guild_name_text_instance;
            guild_name_instance = graphic_text_instance::new_value();
            guild_name_instance->set_text_pointer(font);
            guild_name_instance->set_outline(true);
            guild_name_instance->set_horizonal_align(graphic_text_instance::HORIZONTAL_ALIGN_CENTER);
            guild_name_instance->set_vertical_align(graphic_text_instance::VERTICAL_ALIGN_BOTTOM);
            guild_name_instance->set_value(guild_name.c_str());
            guild_name_instance->set_color(c_TextTail_Guild_Name_Color.r, c_TextTail_Guild_Name_Color.g, c_TextTail_Guild_Name_Color.b);
            guild_name_instance->update();
        }

        character_text_tail_map_.insert(text_tail_map::value_type(virtual_id, text_tail_value));
    }

    void python_text_tail::register_item_text_tail(uint32_t virtual_id, const char *text, graphic_object_instance *owner) {
        text_tail *text_tail_value = register_text_tail(virtual_id, text, owner, c_TextTail_Name_Position, c_TextTail_Item_Color);
        item_text_tail_map_.insert(text_tail_map::value_type(virtual_id, text_tail_value));
    }

    void python_text_tail::register_chat_tail(uint32_t virtual_id, const char *chat) {
        instance_base *character_instance = get_python_character_manager().instance_ptr(virtual_id);

        if (!character_instance)
            return;

        auto itor = chat_tail_map_value_.find(virtual_id);

        if (chat_tail_map_value_.end() != itor) {
            text_tail *text_tail_value = itor->second;

            text_tail_value->text_instance->set_value(chat);
            text_tail_value->text_instance->update();
            text_tail_value->color_value = c_TextTail_Chat_Color;
            text_tail_value->text_instance->set_color(c_TextTail_Chat_Color);

            text_tail_value->living_time = get_timer().current_millisecond() + text_tail_get_living_time();

            text_tail_value->name_flag = true;

            return;
        }

        text_tail *text_tail_value = register_text_tail(virtual_id,
            chat,
            character_instance->graphic_thing_instance_ptr(),
            character_instance->graphic_thing_instance_ref().height() + 10.0f,
            c_TextTail_Chat_Color);

        text_tail_value->living_time = get_timer().current_millisecond() + text_tail_get_living_time();

        text_tail_value->name_flag = true;
        text_tail_value->text_instance->set_outline(true);
        text_tail_value->text_instance->set_vertical_align(graphic_text_instance::VERTICAL_ALIGN_BOTTOM);
        chat_tail_map_value_.insert(text_tail_map::value_type(virtual_id, text_tail_value));
    }

    void python_text_tail::register_info_tail(uint32_t virtual_id, const char *chat) {
        instance_base *character_instance = get_python_character_manager().instance_ptr(virtual_id);

        if (!character_instance)
            return;

        auto itor = chat_tail_map_value_.find(virtual_id);

        if (chat_tail_map_value_.end() != itor) {
            text_tail *text_tail_value = itor->second;

            text_tail_value->text_instance->set_value(chat);
            text_tail_value->text_instance->update();
            text_tail_value->color_value = c_TextTail_Info_Color;
            text_tail_value->text_instance->set_color(c_TextTail_Info_Color);

            text_tail_value->living_time = get_timer().current_millisecond() + text_tail_get_living_time();

            text_tail_value->name_flag = false;

            return;
        }

        text_tail *text_tail_value = register_text_tail(virtual_id,
            chat,
            character_instance->graphic_thing_instance_ptr(),
            character_instance->graphic_thing_instance_ref().height() + 10.0f,
            c_TextTail_Info_Color);

        text_tail_value->living_time = get_timer().current_millisecond() + text_tail_get_living_time();

        text_tail_value->name_flag = false;
        text_tail_value->text_instance->set_outline(true);
        text_tail_value->text_instance->set_vertical_align(graphic_text_instance::VERTICAL_ALIGN_BOTTOM);
        chat_tail_map_value_.insert(text_tail_map::value_type(virtual_id, text_tail_value));
    }

    auto python_text_tail::text_tail_position(uint32_t vid, float *px, float *py, float *pz) -> bool {
        auto itor_character = character_text_tail_map_.find(vid);

        if (character_text_tail_map_.end() == itor_character) {
            return false;
        }

        text_tail *text_tail_value = itor_character->second;
        *px = text_tail_value->x;
        *py = text_tail_value->y;
        *pz = text_tail_value->z;

        return true;
    }

    auto python_text_tail::is_chat_text_tail(uint32_t vid) -> bool {
        auto itor_chat = chat_tail_map_value_.find(vid);

        if (chat_tail_map_value_.end() == itor_chat)
            return false;

        return true;
    }

    void python_text_tail::set_character_text_tail_color(uint32_t virtual_id, const math::color &color) {
        auto itor_character = character_text_tail_map_.find(virtual_id);

        if (character_text_tail_map_.end() == itor_character)
            return;

        text_tail *text_tail_value = itor_character->second;
        text_tail_value->text_instance->set_color(color);
        text_tail_value->color_value = color;
    }

    void python_text_tail::set_item_text_tail_owner(uint32_t vid, const char *c_sz_name) {
        auto itor = item_text_tail_map_.find(vid);
        if (item_text_tail_map_.end() == itor)
            return;

        text_tail *text_tail_value = itor->second;

        if (strlen(c_sz_name) > 0) {
            if (!text_tail_value->owner_text_instance) {
                text_tail_value->owner_text_instance = graphic_text_instance::new_value();
            }

            std::string str_name = c_sz_name;
            static const std::string ownership = application_string_table_get_stringz(IDS_POSSESSIVE_MORPHENE);
            str_name += ownership;

            text_tail_value->owner_text_instance->set_text_pointer(font);
            text_tail_value->owner_text_instance->set_horizonal_align(graphic_text_instance::HORIZONTAL_ALIGN_CENTER);
            text_tail_value->owner_text_instance->set_value(str_name.c_str());
            text_tail_value->owner_text_instance->set_color(1.0f, 1.0f, 0.0f);
            text_tail_value->owner_text_instance->update();

            int x_owner_size, y_owner_size;
            text_tail_value->owner_text_instance->text_size(&x_owner_size, &y_owner_size);
            text_tail_value->y_start = -2.0f;
            text_tail_value->y_end += float(y_owner_size + 4);
            text_tail_value->x_start = f_min(text_tail_value->x_start, float(-x_owner_size / 2 - 1));
            text_tail_value->x_end = f_max(text_tail_value->x_end, float(x_owner_size / 2 + 1));
        } else {
            if (text_tail_value->owner_text_instance) {
                graphic_text_instance::delete_value(text_tail_value->owner_text_instance);
                text_tail_value->owner_text_instance = nullptr;
            }

            int x_size, y_size;
            text_tail_value->text_instance->text_size(&x_size, &y_size);
            text_tail_value->x_start = (float)(-x_size / 2 - 2);
            text_tail_value->y_start = -2.0f;
            text_tail_value->x_end = (float)(x_size / 2 + 2);
            text_tail_value->y_end = (float)y_size;
        }
    }

    void python_text_tail::delete_character_text_tail(uint32_t virtual_id) {
        auto itor_character = character_text_tail_map_.find(virtual_id);
        auto itor_chat = chat_tail_map_value_.find(virtual_id);

        if (character_text_tail_map_.end() != itor_character) {
            delete_text_tail(itor_character->second);
            character_text_tail_map_.erase(itor_character);
        } else {
            metin2::log::debug("Character text tail for vid {} not found", virtual_id);
        }

        if (chat_tail_map_value_.end() != itor_chat) {
            delete_text_tail(itor_chat->second);
            chat_tail_map_value_.erase(itor_chat);
        }
    }

    void python_text_tail::delete_item_text_tail(uint32_t virtual_id) {
        auto itor = item_text_tail_map_.find(virtual_id);

        if (item_text_tail_map_.end() == itor) {
            metin2::log::debug("Item text tail not found");
            return;
        }

        delete_text_tail(itor->second);
        item_text_tail_map_.erase(itor);
    }

    auto python_text_tail::register_text_tail(uint32_t virtual_id, const char *text, graphic_object_instance *owner, float height, const math::color &color) -> python_text_tail::text_tail * {
        text_tail *text_tail_value = text_tail_pool_.alloc();

        text_tail_value->virtual_id = virtual_id;
        text_tail_value->owner = owner;
        text_tail_value->text_instance = graphic_text_instance::new_value();
        text_tail_value->owner_text_instance = nullptr;
        text_tail_value->height = height;

        text_tail_value->text_instance->set_text_pointer(font);
        text_tail_value->text_instance->set_horizonal_align(graphic_text_instance::HORIZONTAL_ALIGN_CENTER);
        text_tail_value->text_instance->set_value(text);
        text_tail_value->text_instance->set_color(color.r, color.g, color.b);
        text_tail_value->text_instance->update();

        int x_size, y_size;
        text_tail_value->text_instance->text_size(&x_size, &y_size);
        text_tail_value->x_start = (float)(-x_size / 2 - 2);
        text_tail_value->y_start = -2.0f;
        text_tail_value->x_end = (float)(x_size / 2 + 2);
        text_tail_value->y_end = (float)y_size;
        text_tail_value->color_value = color;
        text_tail_value->distance_from_player = 0.0f;
        text_tail_value->x = -100.0f;
        text_tail_value->y = -100.0f;
        text_tail_value->z = 0.0f;
        text_tail_value->mark_instance = nullptr;
        text_tail_value->guild_name_text_instance = nullptr;
        text_tail_value->title_text_instance = nullptr;
        text_tail_value->level_text_instance = nullptr;
        return text_tail_value;
    }

    void python_text_tail::delete_text_tail(text_tail *text_tail_value) {
        if (text_tail_value->text_instance) {
            graphic_text_instance::delete_value(text_tail_value->text_instance);
            text_tail_value->text_instance = nullptr;
        }
        if (text_tail_value->owner_text_instance) {
            graphic_text_instance::delete_value(text_tail_value->owner_text_instance);
            text_tail_value->owner_text_instance = nullptr;
        }
        if (text_tail_value->mark_instance) {
            graphic_mark_instance::delete_value(text_tail_value->mark_instance);
            text_tail_value->mark_instance = nullptr;
        }
        if (text_tail_value->guild_name_text_instance) {
            graphic_text_instance::delete_value(text_tail_value->guild_name_text_instance);
            text_tail_value->guild_name_text_instance = nullptr;
        }
        if (text_tail_value->title_text_instance) {
            graphic_text_instance::delete_value(text_tail_value->title_text_instance);
            text_tail_value->title_text_instance = nullptr;
        }
        if (text_tail_value->level_text_instance) {
            graphic_text_instance::delete_value(text_tail_value->level_text_instance);
            text_tail_value->level_text_instance = nullptr;
        }

        text_tail_pool_.free(text_tail_value);
    }

    auto python_text_tail::pick(int ix_mouse, int iy_mouse) -> int {
        for (auto itor = item_text_tail_map_.begin(); itor != item_text_tail_map_.end(); ++itor) {
            text_tail *text_tail_value = itor->second;

            if (ix_mouse >= text_tail_value->x + text_tail_value->x_start && ix_mouse <= text_tail_value->x + text_tail_value->x_end &&
                iy_mouse >= text_tail_value->y + text_tail_value->y_start && iy_mouse <= text_tail_value->y + text_tail_value->y_end) {
                select_item_name(itor->first);
                return (itor->first);
            }
        }

        return -1;
    }

    void python_text_tail::select_item_name(uint32_t virtual_id) {
        auto itor = item_text_tail_map_.find(virtual_id);

        if (item_text_tail_map_.end() == itor)
            return;

        text_tail *text_tail_value = itor->second;
        text_tail_value->text_instance->set_color(0.1f, 0.9f, 0.1f);
    }

    void python_text_tail::attach_title(uint32_t vid, const char *name, const math::color &color) {
        if (!pk_title_enable)
            return;

        auto itor = character_text_tail_map_.find(vid);
        if (character_text_tail_map_.end() == itor)
            return;

        text_tail *text_tail_value = itor->second;

        graphic_text_instance *&title = text_tail_value->title_text_instance;
        if (!title) {
            title = graphic_text_instance::new_value();
            title->set_text_pointer(font);
            title->set_outline(true);

            title->set_horizonal_align(graphic_text_instance::HORIZONTAL_ALIGN_RIGHT);

            title->set_vertical_align(graphic_text_instance::VERTICAL_ALIGN_BOTTOM);
        }

        title->set_value(name);
        title->set_color(color.r, color.g, color.b);
        title->update();
    }

    void python_text_tail::detach_title(uint32_t vid) {
        if (!pk_title_enable)
            return;

        auto itor = character_text_tail_map_.find(vid);
        if (character_text_tail_map_.end() == itor)
            return;

        text_tail *text_tail_value = itor->second;

        if (text_tail_value->title_text_instance) {
            graphic_text_instance::delete_value(text_tail_value->title_text_instance);
            text_tail_value->title_text_instance = nullptr;
        }
    }

    void python_text_tail::enable_pk_title(bool flag) {
        pk_title_enable = flag;
    }

    void python_text_tail::attach_level(uint32_t vid, const char *text, const math::color &color) {
        if (!pk_title_enable)
            return;

        auto itor = character_text_tail_map_.find(vid);
        if (character_text_tail_map_.end() == itor)
            return;

        text_tail *text_tail_value = itor->second;

        graphic_text_instance *&level = text_tail_value->level_text_instance;
        if (!level) {
            level = graphic_text_instance::new_value();
            level->set_text_pointer(font);
            level->set_outline(true);

            level->set_horizonal_align(graphic_text_instance::HORIZONTAL_ALIGN_RIGHT);
            level->set_vertical_align(graphic_text_instance::VERTICAL_ALIGN_BOTTOM);
        }

        level->set_value(text);
        level->set_color(color.r, color.g, color.b);
        level->update();
    }

    void python_text_tail::initialize() {

        auto *default_font_value = static_cast<graphic_text *>(default_font_get_resource());
        if (!default_font_value) {
            metin2::log::error("Cannot find the default font");
            return;
        }

        font = default_font_value;
    }

    void python_text_tail::destroy() {
        text_tail_pool_.clear();
    }

    void python_text_tail::clear() {
        character_text_tail_map_.clear();
        character_text_tail_list_.clear();
        item_text_tail_map_.clear();
        item_text_tail_list_.clear();
        chat_tail_map_value_.clear();

        text_tail_pool_.clear();
    }

    python_text_tail::python_text_tail() {
        clear();
    }

    python_text_tail::~python_text_tail() {
        destroy();
    }
}
