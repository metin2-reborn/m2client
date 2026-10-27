#include "grp_text_instance.h"
#include <algorithm>
#include <cmath>
#include <metin2/core/utf8.h>
#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include <metin2/log/log.h>
#include <vector>
#include "grp_base.h"
#include "state_manager.h"
#include "text_tag.h"

namespace metin2::inline graphics {

    dynamic_pool<graphic_text_instance> graphic_text_instance::pool_value;

    static int mx = 0;
    static int my = 0;

    static std::wstring hyperlink_text;

    void graphic_text_instance::hyperlink_update_mouse_pos(int x, int y) {
        mx = x;
        my = y;
        hyperlink_text = L"";
    }

    // The hyperlink under the mouse as UTF-8 (not NUL-terminated); returns its length in bytes.
    auto graphic_text_instance::hyperlink_get_text(char *buf, int len) -> int {
        if (hyperlink_text.empty() || len <= 0)
            return 0;

        std::string text = utf8::from_wide(hyperlink_text);
        std::size_t n = utf8::truncated_size(text, static_cast<std::size_t>(len));
        memcpy(buf, text.data(), n);
        return static_cast<int>(n);
    }

    auto graphic_text_instance::pixel_scale() -> float {
        if (ro_text_.is_null())
            return graphic_base::interface_pixel_scale();
        graphic_font_texture *font_texture = ro_text_->font_texture_pointer();
        return font_texture ? font_texture->pixel_scale() : graphic_base::interface_pixel_scale();
    }

    auto graphic_text_instance::draw_character(graphic_font_texture *font_texture, wchar_t text, uint32_t color) -> float {
        graphic_font_texture::character_infomation *ins_char_info = font_texture->get_character_infomation(text);

        if (ins_char_info) {
            color_info_vector_.push_back(color);
            char_info_vector_.push_back(ins_char_info);

            text_width_ += ins_char_info->advance;
            return ins_char_info->advance;
        }

        return 0.0f;
    }

    // Whether glyph `index` starts a new row when the pen (texels from the row start) is in front
    // of it. Only multi-line text wraps; a glyph that does not fit on an empty row is drawn anyway.
    auto graphic_text_instance::wraps(std::size_t index, float pen) -> bool {
        if (!is_multi_line_ || pen <= 0.0f)
            return false;
        return pen + char_info_vector_[index]->advance > limit_width_ * pixel_scale();
    }

    // The pen in front of glyph `index` in texels relative to the aligned origin: x along the row
    // (fractional, rounded per glyph when drawn), y the row's top.
    void graphic_text_instance::pen_at(std::size_t index, float *x, int *y) {
        index = std::min(index, char_info_vector_.size());

        float pen = 0.0f;
        int row = 0;
        for (std::size_t i = 0; i < index; ++i) {
            if (wraps(i, pen)) {
                pen = 0.0f;
                row += text_height_;
            }
            pen += char_info_vector_[i]->advance;
        }
        if (index < char_info_vector_.size() && wraps(index, pen)) {
            pen = 0.0f;
            row += text_height_;
        }

        *x = pen;
        *y = row;
    }

    void graphic_text_instance::character_position(std::size_t index, float *x, float *y) {
        float pen = 0.0f;
        int row = 0;
        pen_at(index, &pen, &row);
        const float px = 1.0f / pixel_scale();
        *x = static_cast<float>(lroundf(pen)) * px;
        *y = static_cast<float>(row) * px;
    }

    auto graphic_text_instance::character_x(std::size_t index) -> float {
        float x = 0.0f, y = 0.0f;
        character_position(index, &x, &y);
        return x;
    }

    auto graphic_text_instance::character_index_at(float x, float y) -> std::size_t {
        const float scale = pixel_scale();
        x *= scale;
        y *= scale;

        float pen = 0.0f;
        int row = 0;
        for (std::size_t i = 0; i < char_info_vector_.size(); ++i) {
            const float advance = char_info_vector_[i]->advance;
            if (wraps(i, pen)) {
                if (y < static_cast<float>(row + text_height_))
                    return i; // the point is right of this row
                pen = 0.0f;
                row += text_height_;
            }
            if (y < static_cast<float>(row + text_height_) && x < pen + advance / 2.0f)
                return i;
            pen += advance;
        }
        return char_info_vector_.size();
    }

    // The aligned origin in texels: the interface position, aligned by the text size, rounded to a
    // whole texel once so every glyph lands on framebuffer pixels. Vertically the rounding happens
    // at the baseline, which is what the eye centres; rounding the line top and the ascent apart
    // can push the glyphs a pixel down.
    void graphic_text_instance::aligned_origin_texels(int *x, int *y) {
        const float scale = pixel_scale();
        float fx = position_value_.x * scale;
        float fy = position_value_.y * scale;

        switch (h_align_) {
        case HORIZONTAL_ALIGN_RIGHT:
            fx -= text_width_;
            break;
        case HORIZONTAL_ALIGN_CENTER:
            fx -= text_width_ / 2.0f;
            break;
        }

        switch (v_align_) {
        case VERTICAL_ALIGN_BOTTOM:
            fy -= static_cast<float>(text_height_);
            break;
        case VERTICAL_ALIGN_CENTER:
            fy -= static_cast<float>(text_height_) / 2.0f;
            break;
        }

        *x = static_cast<int>(lroundf(fx));
        graphic_font_texture *font_texture = ro_text_.is_null() ? nullptr : ro_text_->font_texture_pointer();
        if (font_texture && !font_texture->is_empty()) {
            *y = static_cast<int>(lroundf(fy + font_texture->ascent())) - font_texture->baseline();
        } else {
            *y = static_cast<int>(lroundf(fy));
        }
    }

    void graphic_text_instance::aligned_origin(float *x, float *y) {
        int ox = 0, oy = 0;
        aligned_origin_texels(&ox, &oy);
        const float px = 1.0f / pixel_scale();
        *x = static_cast<float>(ox) * px;
        *y = static_cast<float>(oy) * px;
    }

    void graphic_text_instance::update() {
        if (ro_text_.is_null()) {
            if (!is_update_)
                metin2::log::debug("Text has no font");
            is_update_ = true;
            return;
        }

        if (ro_text_->is_empty())
            return;

        graphic_font_texture *font_texture = ro_text_->font_texture_pointer();
        if (!font_texture)
            return;

        // The glyph pointers belong to one rasterization of the atlas; another one (the font
        // changed, or the interface pixel scale did) means laying the text out again.
        if (is_update_ && font_generation_ == font_texture->generation())
            return;
        font_generation_ = font_texture->generation();

        char_info_vector_.clear();
        color_info_vector_.clear();
        hyperlink_vector_.clear();

        text_width_ = 0.0f;
        text_height_ = font_texture->line_height();

        std::wstring text = utf8::to_wide(text_);
        int text_len = static_cast<int>(text.size());

        uint32_t color = text_color_;

        if (is_secret_) {
            for (int i = 0; i < text_len; ++i)
                draw_character(font_texture, '*', color);
        } else {
            float x = 0.0f;
            int len;
            int hyperlink_step = 0;
            hyperlink hyperlink_value;
            std::wstring hyperlink_buffer;

            for (int i = 0; i < text_len;) {
                int ret = get_text_tag(&text[i], text_len - i, len, hyperlink_buffer);

                if (ret == TEXT_TAG_PLAIN || ret == TEXT_TAG_TAG) {
                    if (hyperlink_step == 1)
                        hyperlink_buffer.append(1, text[i]);
                    else {
                        x += draw_character(font_texture, text[i], color);
                        hyperlink_value.ex = static_cast<short>(lroundf(x));
                    }
                } else {
                    if (ret == TEXT_TAG_COLOR)
                        color = htoi(hyperlink_buffer.c_str(), 8);
                    else if (ret == TEXT_TAG_RESTORE_COLOR)
                        color = text_color_;
                    else if (ret == TEXT_TAG_HYPERLINK_START) {
                        hyperlink_step = 1;
                        hyperlink_buffer = L"";
                    } else if (ret == TEXT_TAG_HYPERLINK_END) {
                        if (hyperlink_step == 1) {
                            ++hyperlink_step;
                            hyperlink_value.ex = hyperlink_value.sx = static_cast<short>(lroundf(x)); // where the actual text starts
                        } else {
                            hyperlink_value.text = hyperlink_buffer;
                            hyperlink_vector_.push_back(hyperlink_value);

                            hyperlink_step = 0;
                            hyperlink_buffer = L"";
                        }
                    }
                }
                i += len;
            }
        }

        font_texture->update_texture();

        is_update_ = true;
    }

    void graphic_text_instance::render(rectangle *clip_rect) {
        if (!is_update_)
            return;

        graphic_text *text = ro_text_.pointer();
        if (!text)
            return;

        graphic_font_texture *font_texture = text->font_texture_pointer();
        if (!font_texture)
            return;
        if (font_generation_ != font_texture->generation()) {
            update();
            if (font_generation_ != font_texture->generation())
                return;
        }

        const float scale = font_texture->pixel_scale();
        const float px = 1.0f / scale;
        const float half = 0.5f * px;

        int origin_x = 0;
        int origin_y = 0;
        aligned_origin_texels(&origin_x, &origin_y);
        const int scroll = static_cast<int>(lroundf(scroll_x_ * scale));
        const float limit = (position_value_.x - static_cast<float>(origin_x) * px + limit_width_) * scale; // texels from the origin
        const int clip_top = clip_rect ? static_cast<int>(lroundf(clip_rect->top * scale)) : 0;

        STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);
        uint32_t fog_enable = STATEMANAGER.render_state(gpu::state::FOG_ENABLE);
        uint32_t lighting = STATEMANAGER.render_state(gpu::state::LIGHTING);
        STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, false);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);

        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);

        {
            // All glyph quads of the text go up in one stream and one draw per atlas page: a draw
            // per glyph (five with the outline) was the most expensive part of a frame.
            static std::vector<vertex> batch;
            batch.clear();
            uint32_t batch_page = 0;
            bool batch_open = false;

            auto flush = [&] {
                if (batch.empty())
                    return;
                font_texture->select_texture(batch_page);
                STATEMANAGER.set_texture(0, font_texture->gpu_texture());
                // Longer runs than the user stream holds go in slices (each a whole number of quads).
                constexpr size_t MAX_VERTICES = 4096 * 6;
                for (size_t at = 0; at < batch.size(); at += MAX_VERTICES) {
                    const size_t count = std::min(MAX_VERTICES, batch.size() - at);
                    STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_LIST, static_cast<uint32_t>(count / 3), batch.data() + at, sizeof(vertex));
                }
                batch.clear();
            };

            // The cell of glyph `info` with its top left corner at texel (x, y) relative to the
            // origin, as a quad in interface pixels on framebuffer pixel boundaries.
            auto push_cell = [&](const graphic_font_texture::character_infomation &info, int x, int y, uint32_t color) {
                if (batch_open && info.index_value != static_cast<short>(batch_page))
                    flush();
                batch_page = static_cast<uint32_t>(info.index_value);
                batch_open = true;

                const float sx = static_cast<float>(origin_x + x) * px - half;
                const float sy = static_cast<float>(origin_y + y) * px - half;
                const float ex = sx + static_cast<float>(info.width) * px;
                const float ey = sy + static_cast<float>(info.height) * px;

                vertex quad[4];
                for (vertex &v : quad) {
                    v.z = position_value_.z;
                    v.color_value = color;
                }
                quad[0].x = sx;
                quad[0].y = sy;
                quad[0].u = info.left;
                quad[0].v = info.top;
                quad[1].x = sx;
                quad[1].y = ey;
                quad[1].u = info.left;
                quad[1].v = info.bottom;
                quad[2].x = ex;
                quad[2].y = sy;
                quad[2].u = info.right;
                quad[2].v = info.top;
                quad[3].x = ex;
                quad[3].y = ey;
                quad[3].u = info.right;
                quad[3].v = info.bottom;

                batch.push_back(quad[0]);
                batch.push_back(quad[1]);
                batch.push_back(quad[2]);
                batch.push_back(quad[2]);
                batch.push_back(quad[1]);
                batch.push_back(quad[3]);
            };

            // Walks the glyphs as laid out, with the cell origin of each one that is drawn.
            auto each_glyph = [&](auto &&draw) {
                float pen = 0.0f;
                int row = 0;
                for (std::size_t i = 0; i < char_info_vector_.size(); ++i) {
                    const graphic_font_texture::character_infomation &info = *char_info_vector_[i];
                    if (wraps(i, pen)) {
                        pen = 0.0f;
                        row += text_height_;
                    }
                    const int x = static_cast<int>(lroundf(pen)) - scroll;
                    pen += info.advance;
                    if (x < 0)
                        continue; // scrolled out to the left
                    if (!is_multi_line_ && static_cast<float>(x) + info.advance > limit)
                        break; // single-line text is clipped at limit_width_
                    if (clip_rect && origin_y + row <= clip_top)
                        continue;
                    if (info.width == 0)
                        continue;
                    draw(i, info, x + info.bearing_x, row + info.bearing_y);
                }
            };

            if (is_outline_) {
                // One framebuffer pixel of outline per interface pixel, on whole texels.
                const int weight = std::max(1, static_cast<int>(lroundf(scale)));
                each_glyph([&](std::size_t, const graphic_font_texture::character_infomation &info, int x, int y) {
                    push_cell(info, x - weight, y, out_line_color_);
                    push_cell(info, x + weight, y, out_line_color_);
                    push_cell(info, x, y - weight, out_line_color_);
                    push_cell(info, x, y + weight, out_line_color_);
                });
                flush();
            }

            each_glyph([&](std::size_t i, const graphic_font_texture::character_infomation &info, int x, int y) {
                push_cell(info, x, y, color_info_vector_[i]);
            });
            flush();
        }

        STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);

        STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, fog_enable);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, lighting);

        if (hyperlink_vector_.size() != 0) {
            const int lx = static_cast<int>(lroundf((static_cast<float>(mx) - position_value_.x) * scale));
            const int ly = static_cast<int>(lroundf((static_cast<float>(my) - position_value_.y) * scale));

            if (lx >= 0 && ly >= 0 && static_cast<float>(lx) < text_width_ && ly < text_height_) {
                auto it = hyperlink_vector_.begin();

                while (it != hyperlink_vector_.end()) {
                    hyperlink &link = *it++;
                    if (lx >= link.sx && lx < link.ex) {
                        hyperlink_text = link.text;
                        break;
                    }
                }
            }
        }
    }

    void graphic_text_instance::create_system(uint32_t capacity) {
        pool_value.create(capacity);
    }

    void graphic_text_instance::destroy_system() {
        pool_value.destroy();
    }

    auto graphic_text_instance::new_value() -> graphic_text_instance * {
        return pool_value.alloc();
    }

    void graphic_text_instance::delete_value(graphic_text_instance *inst) {
        inst->destroy();
        pool_value.free(inst);
    }

    void graphic_text_instance::set_color(uint32_t color) {
        if (text_color_ != color) {
            for (size_t i = 0; i < char_info_vector_.size(); ++i)
                if (color_info_vector_[i] == text_color_)
                    color_info_vector_[i] = color;

            text_color_ = color;
        }
    }

    void graphic_text_instance::set_color(float r, float g, float b, float a) {
        set_color(math::color(r, g, b, a));
    }

    void graphic_text_instance::set_secret(bool value) {
        is_secret_ = value;
    }

    void graphic_text_instance::set_outline(bool value) {
        is_outline_ = value;
    }

    void graphic_text_instance::set_multi_line(bool value) {
        is_multi_line_ = value;
    }

    void graphic_text_instance::set_horizonal_align(int h_align) {
        h_align_ = h_align;
    }

    void graphic_text_instance::set_vertical_align(int v_align) {
        v_align_ = v_align;
    }

    void graphic_text_instance::set_limit_width(float width_value) {
        limit_width_ = width_value;
    }

    void graphic_text_instance::set_value_string(const std::string &value) {
        if (0 == text_.compare(value))
            return;

        text_ = value;
        is_update_ = false;
    }

    void graphic_text_instance::set_value(const char *text, size_t len) {
        if (0 == text_.compare(text))
            return;

        text_ = text;
        is_update_ = false;
    }

    void graphic_text_instance::set_position(float fx, float fy, float fz) {
        position_value_.x = fx;
        position_value_.y = fy;
        position_value_.z = fz;
    }

    void graphic_text_instance::set_text_pointer(graphic_text *text) {
        ro_text_ = text;
    }

    auto graphic_text_instance::value_string_reference() -> const std::string & {
        return text_;
    }

    auto graphic_text_instance::text_line_count() -> uint16_t {
        float pen = 0.0f;
        uint16_t line_count = 1;
        for (std::size_t i = 0; i < char_info_vector_.size(); ++i) {
            if (wraps(i, pen)) {
                pen = 0.0f;
                ++line_count;
            }
            pen += char_info_vector_[i]->advance;
        }

        return line_count;
    }

    void graphic_text_instance::text_size(int *ret_width, int *ret_height) {
        const float scale = pixel_scale();
        *ret_width = static_cast<int>(ceilf(text_width_ / scale - 0.001f));
        *ret_height = static_cast<int>(ceilf(static_cast<float>(text_height_) / scale - 0.001f));
    }

    auto graphic_text_instance::get_horizontal_align() -> int {
        return h_align_;
    }

    void graphic_text_instance::initialize_impl() {
        ro_text_ = nullptr;

        h_align_ = HORIZONTAL_ALIGN_LEFT;
        v_align_ = VERTICAL_ALIGN_TOP;

        limit_width_ = 1600.0f; // The widest supported resolution.
        scroll_x_ = 0.0f;

        is_secret_ = false;
        is_multi_line_ = false;

        is_outline_ = false;

        is_update_ = false;
        font_generation_ = 0;

        text_width_ = 0.0f;
        text_height_ = 0;

        position_value_.x = position_value_.y = position_value_.z = 0.0f;

        out_line_color_ = 0xff000000;
    }

    void graphic_text_instance::destroy() {
        text_ = "";
        char_info_vector_.clear();
        color_info_vector_.clear();
        hyperlink_vector_.clear();

        initialize_impl();
    }

    graphic_text_instance::graphic_text_instance() {
        initialize_impl();
    }

    graphic_text_instance::~graphic_text_instance() {
        destroy();
    }
}
