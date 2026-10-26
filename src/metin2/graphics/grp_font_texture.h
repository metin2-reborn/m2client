#pragma once

#include "font_face.h"
#include "grp_dib.h"
#include "grp_image_texture.h"
#include "grp_texture.h"

#include <map>
#include <vector>

namespace metin2::inline graphics {

    // The glyph atlas of one font at one size. Everything in it is in texels, which are framebuffer
    // pixels: the face is rasterized at the font size times the interface pixel scale, so a glyph
    // drawn with one texel per framebuffer pixel is exactly the size the interface asked for.
    class graphic_font_texture : public graphic_texture {
      public:
        using character_key = wchar_t; // a Unicode code point

        struct character_infomation {
            short index_value; // atlas page
            short width;       // cell size: the ink box with a texel of padding around it (0 without ink)
            short height;
            short bearing_x; // cell origin relative to the pen on the top of the line
            short bearing_y;
            float left; // cell as texture coordinates
            float top;
            float right;
            float bottom;
            float advance; // pen advance, fractional
        };

        using p_character_infomation_vector = std::vector<character_infomation *>;

      public:
        graphic_font_texture();
        ~graphic_font_texture() override;

        void destroy();
        auto create(const char *font_name, int font_size, bool italic) -> bool;

        auto create_device_objects() -> bool;
        void destroy_device_objects();

        auto check_texture_index(uint32_t texture) -> bool;
        void select_texture(uint32_t texture);

        auto update_texture() -> bool;

        auto get_character_infomation(wchar_t key_value) -> character_infomation *;
        auto update_character_infomation(character_key code) -> character_infomation *;

        auto is_empty() const -> bool override;

        // The scale the glyphs were rasterized at, and a number unique to this rasterization: a text
        // instance holding glyph pointers from another one must lay itself out again.
        auto pixel_scale() const -> float { return face_.pixel_scale(); }
        auto generation() const -> uint32_t { return generation_; }
        auto line_height() const -> int { return face_.cell_height(); }

      protected:
        void initialize();

        auto append_texture() -> bool;

      protected:
        using graphic_image_texture_pointer_vector = std::vector<graphic_image_texture *>;
        using character_infomation_map = std::map<character_key, character_infomation>;

      protected:
        graphic_dib dib_;
        font_face face_;

        graphic_image_texture_pointer_vector font_texture_vector_;

        character_infomation_map char_info_map_;

        int x_;
        int y_;
        int step_;
        bool is_dirty_;
        uint32_t generation_ = 0;

        int32_t font_size_;
        bool italic_;
    };
}
