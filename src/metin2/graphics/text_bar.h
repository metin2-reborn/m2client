#pragma once

#include "dib_bar.h"
#include "font_face.h"

namespace metin2::inline graphics {

    class text_bar : public dib_bar {
      public:
        text_bar(int font_size, bool is_bold);
        ~text_bar() override;

        void text_out(int ix, int iy, const char *text);
        void set_text_color(int r, int g, int b);
        void text_extent(const char *text, int *width, int *height);

      protected:
        void on_create() override;

      protected:
        font_face face_;
        uint32_t text_color_ = 0x00ffffff;

        int font_size_;
        bool is_bold_;
    };
}
