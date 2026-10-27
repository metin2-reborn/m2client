#include "game_type.h"
#include <metin2/core/utils.h>
#include <metin2/graphics/grp_text.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/log/log.h>

namespace metin2::inline client::inline app {
    std::string image_path = "ui:";

    std::string guild_symbol_path_name = "mark/10/";

    static font_spec default_font_value;
    static bool is_default_font_set = false;
    static resource *default_font = nullptr;
    static resource *default_italic_font = nullptr;

    static bool is_reload_default_font = false;

    void default_font_startup() {
        default_font = nullptr;
    }

    void default_font_cleanup() {
        if (default_font) {
            default_font->release();
        }
    }

    void default_font_set(const font_spec &font) {
        default_font_value = font;
        is_default_font_set = true;
        is_reload_default_font = true;
    }

    auto default_font_spec() -> const font_spec & {
        return default_font_value;
    }

    auto reload_default_fonts() -> bool {
        if (!is_default_font_set) {
            metin2::log::error("No default font set");
            return false;
        }
        resource_manager &res_mgr = get_resource_manager();

        is_reload_default_font = false;

        resource *new_font = res_mgr.resource_pointer(graphic_text::resource_name(default_font_value).c_str());
        new_font->add_reference();
        if (default_font) {
            default_font->release();
        }

        default_font = new_font;

        font_spec italic = default_font_value;
        italic.italic = true;
        resource *new_italic_font = res_mgr.resource_pointer(graphic_text::resource_name(italic).c_str());
        new_italic_font->add_reference();
        if (default_italic_font) {
            default_italic_font->release();
        }

        default_italic_font = new_italic_font;

        return true;
    }

    auto default_font_get_resource() -> resource * {
        if (!default_font || is_reload_default_font) {
            reload_default_fonts();
        }

        return default_font;
    }

    auto default_italic_font_get_resource() -> resource * {
        if (!default_italic_font || is_reload_default_font) {
            reload_default_fonts();
        }

        return default_italic_font;
    }

    void set_guild_symbol_path(const char *path_name) {
        guild_symbol_path_name = "mark/";
        guild_symbol_path_name += path_name;
        guild_symbol_path_name += "/";
    }

    auto get_guild_symbol_file_name(uint32_t guild_id) -> const char * {
        return format_string("%s%03d.jpg", guild_symbol_path_name.c_str(), guild_id);
    }

    const uint8_t inven_type_by_slot_type[SLOT_TYPE_MAX] =
        {
            RESERVED_WINDOW,       // SLOT_TYPE_NONE
            INVENTORY,             // SLOT_TYPE_INVENTORY
            RESERVED_WINDOW,       // SLOT_TYPE_SKILL
            RESERVED_WINDOW,       // SLOT_TYPE_EMOTION
            RESERVED_WINDOW,       // SLOT_TYPE_SHOP
            RESERVED_WINDOW,       // SLOT_TYPE_EXCHANGE_OWNER
            RESERVED_WINDOW,       // SLOT_TYPE_EXCHANGE_TARGET
            RESERVED_WINDOW,       // SLOT_TYPE_QUICK_SLOT
            RESERVED_WINDOW,       // SLOT_TYPE_SAFEBOX (the safebox and the mall have their own code)
            RESERVED_WINDOW,       // SLOT_TYPE_PRIVATE_SHOP
            RESERVED_WINDOW,       // SLOT_TYPE_MALL
            DRAGON_SOUL_INVENTORY, // SLOT_TYPE_DRAGON_SOUL_INVENTORY
    };

    auto slot_type_to_inven_type(uint8_t slot_type_value) -> uint8_t {
        if (slot_type_value >= SLOT_TYPE_MAX) {
            return RESERVED_WINDOW;
        }

        return inven_type_by_slot_type[slot_type_value];
    }
}
