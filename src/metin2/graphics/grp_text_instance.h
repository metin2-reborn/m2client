#pragma once

#include <metin2/core/pool.h>
#include <metin2/core/rectangle.h>
#include "grp_text.h"

namespace metin2::inline graphics {

    class graphic_text_instance {
      public:
        using pool_type = dynamic_pool<graphic_text_instance>;

      public:
        enum horizontal_align {
            HORIZONTAL_ALIGN_LEFT = 0x01,
            HORIZONTAL_ALIGN_CENTER = 0x02,
            HORIZONTAL_ALIGN_RIGHT = 0x03,
        };
        enum vertical_align {
            VERTICAL_ALIGN_TOP = 0x10,
            VERTICAL_ALIGN_CENTER = 0x20,
            VERTICAL_ALIGN_BOTTOM = 0x30
        };

      public:
        static void hyperlink_update_mouse_pos(int x, int y);
        static auto hyperlink_get_text(char *buf, int len) -> int;

      public:
        graphic_text_instance();
        virtual ~graphic_text_instance();

        void destroy();

        void update();
        void render(rectangle *clip_rect = nullptr);

        void set_color(uint32_t color);
        void set_color(float r, float g, float b, float a = 1.0f);

        void set_horizonal_align(int h_align);
        void set_vertical_align(int v_align);
        void set_text_pointer(graphic_text *text);
        void set_value_string(const std::string &value);
        void set_value(const char *value, size_t len = -1);
        void set_position(float fx, float fy, float fz = 0.0f);
        void set_secret(bool value);
        void set_outline(bool value);
        void set_feather(bool value);
        void set_multi_line(bool value);
        void set_limit_width(float width_value);

        void text_size(int *ret_width, int *ret_height);
        auto value_string_reference() -> const std::string &;
        auto text_line_count() -> uint16_t;

        auto get_horizontal_align() -> int;

        // Glyph geometry for edit lines, in pixels relative to the aligned origin (see aligned_origin):
        // the top left of glyph `index` (glyph_count() gives the end of the text), wrapping like render().
        void character_position(std::size_t index, float *x, float *y);
        auto character_x(std::size_t index) -> float;
        // The glyph boundary nearest to (x, y), 0..glyph_count(); y picks the wrapped row.
        auto character_index_at(float x, float y) -> std::size_t;
        auto glyph_count() -> std::size_t { return char_info_vector_.size(); }
        // Where the first glyph is drawn once the alignment is applied.
        void aligned_origin(float *x, float *y);
        // Rendering starts this many pixels into the text; glyphs left of the origin are skipped.
        void set_scroll_x(float scroll_x) { scroll_x_ = scroll_x; }

      protected:
        void initialize_impl();
        auto draw_character(graphic_font_texture *font_texture, wchar_t text, uint32_t color) -> int;

      protected:
        struct hyperlink {
            short sx;
            short ex;
            std::wstring text;

            hyperlink() : sx(0), ex(0) {}
        };

      protected:
        uint32_t text_color_;
        uint32_t out_line_color_;

        uint16_t text_width_;
        uint16_t text_height_;

        uint8_t h_align_;
        uint8_t v_align_;

        float limit_width_;
        float scroll_x_;

        bool is_secret_;
        bool is_multi_line_;

        bool is_outline_;
        float font_feather_;

        std::string text_;
        math::vector3 position_value_;

      private:
        bool is_update_;

        graphic_text::ref ro_text_;
        graphic_font_texture::p_character_infomation_vector char_info_vector_;
        std::vector<uint32_t> color_info_vector_;
        std::vector<hyperlink> hyperlink_vector_;

      public:
        static void create_system(uint32_t capacity);
        static void destroy_system();

        static auto new_value() -> graphic_text_instance *;
        static void delete_value(graphic_text_instance *inst);

        static dynamic_pool<graphic_text_instance> pool_value;
    };

    extern auto find_token(const char *begin, const char *end) -> const char *;
    extern auto read_token(const char *token) -> int;
}
