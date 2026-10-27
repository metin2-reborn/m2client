#include "text_bar.h"
#include <metin2/core/utf8.h>

namespace metin2::inline graphics {

    void text_bar::set_text_color(int r, int g, int b) {
        text_color_ = static_cast<uint32_t>(r & 0xff) | (static_cast<uint32_t>(g & 0xff) << 8) | (static_cast<uint32_t>(b & 0xff) << 16);
    }

    void text_bar::text_extent(const char *text, int *width, int *height) {
        *width = face_.text_width(utf8::to_u32(text));
        *height = face_.cell_height();
    }

    void text_bar::text_out(int ix, int iy, const char *text) {
        face_.draw(dib_.pixels(), dib_.width(), dib_.height(), ix, iy, utf8::to_u32(text), text_color_);
        invalidate();
    }

    void text_bar::on_create() {
        face_.open(file_, size_);
    }

    text_bar::text_bar(std::string file, int size) : file_(std::move(file)), size_(size) {
    }

    text_bar::~text_bar() = default;
}
