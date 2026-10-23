#include "grp_text_instance.h"
#include <algorithm>
#include <metin2/core/utf8.h>
#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include <metin2/log/log.h>
#include "state_manager.h"
#include "text_tag.h"

namespace metin2::inline graphics {

    const float c_fFontFeather = 0.5f;

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

    auto graphic_text_instance::draw_character(graphic_font_texture *font_texture, wchar_t text, uint32_t color) -> int {
        graphic_font_texture::character_infomation *ins_char_info = font_texture->get_character_infomation(text);

        if (ins_char_info) {
            color_info_vector_.push_back(color);
            char_info_vector_.push_back(ins_char_info);

            text_width_ += ins_char_info->advance;
            text_height_ = std::max<int>(ins_char_info->height, text_height_);
            return ins_char_info->advance;
        }

        return 0;
    }

    void graphic_text_instance::character_position(std::size_t index, float *x, float *y) {
        index = std::min(index, char_info_vector_.size());

        float sx = 0;
        float sy = 0;
        float font_max_height = 0;

        for (std::size_t i = 0; i < index; ++i) {
            if (is_multi_line_ && sx + float(char_info_vector_[i]->width) > limit_width_) {
                sx = 0;
                sy += font_max_height;
            }

            sx += float(char_info_vector_[i]->advance);
            font_max_height = std::max(float(char_info_vector_[i]->height), font_max_height);
        }

        *x = sx;
        *y = sy;
    }

    auto graphic_text_instance::character_x(std::size_t index) -> float {
        float x = 0.0f, y = 0.0f;
        character_position(index, &x, &y);
        return x;
    }

    auto graphic_text_instance::character_index_at(float x, float y) -> std::size_t {
        float sx = 0;
        float sy = 0;
        float font_max_height = 0;

        for (std::size_t i = 0; i < char_info_vector_.size(); ++i) {
            const auto width = float(char_info_vector_[i]->width);
            const auto advance = float(char_info_vector_[i]->advance);
            if (is_multi_line_ && sx + width > limit_width_) {
                if (y < sy + font_max_height)
                    return i; // the point is right of this row
                sx = 0;
                sy += font_max_height;
            }
            font_max_height = std::max(float(char_info_vector_[i]->height), font_max_height);
            if (y < sy + font_max_height && x < sx + advance / 2.0f)
                return i;
            sx += advance;
        }
        return char_info_vector_.size();
    }

    void graphic_text_instance::aligned_origin(float *x, float *y) {
        *x = position_value_.x;
        *y = position_value_.y + 1.0f;

        switch (h_align_) {
        case HORIZONTAL_ALIGN_RIGHT:
            *x -= text_width_;
            break;
        case HORIZONTAL_ALIGN_CENTER:
            *x -= float(text_width_ / 2);
            break;
        }

        switch (v_align_) {
        case VERTICAL_ALIGN_BOTTOM:
            *y -= text_height_;
            break;
        case VERTICAL_ALIGN_CENTER:
            *y -= float(text_height_) / 2.0f;
            break;
        }
    }

    void graphic_text_instance::update() {
        if (is_update_)
            return;

        if (ro_text_.is_null()) {
            metin2::log::debug("Text has no font");
            return;
        }

        if (ro_text_->is_empty())
            return;

        graphic_font_texture *font_texture = ro_text_->font_texture_pointer();
        if (!font_texture)
            return;

        graphic_font_texture::character_infomation *space_info = font_texture->get_character_infomation(' ');

        int space_height = space_info ? space_info->height : 12;

        char_info_vector_.clear();
        color_info_vector_.clear();
        hyperlink_vector_.clear();

        text_width_ = 0;
        text_height_ = space_height;

        std::wstring text = utf8::to_wide(text_);
        int text_len = static_cast<int>(text.size());

        uint32_t color = text_color_;

        if (is_secret_) {
            for (int i = 0; i < text_len; ++i)
                draw_character(font_texture, '*', color);
        } else {
            int x = 0;
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
                        int char_width = draw_character(font_texture, text[i], color);
                        hyperlink_value.ex += char_width;
                        x += char_width;
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
                            hyperlink_value.ex = hyperlink_value.sx = x; // where the actual text starts
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

        float stan_x;
        float stan_y;
        aligned_origin(&stan_x, &stan_y);
        const float visible_left = stan_x; // glyphs scrolled out to the left of the aligned origin are skipped
        stan_x -= scroll_x_;

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
            const float font_half_weight = 1.0f;

            float cur_x;
            float cur_y;

            float font_sx;
            float font_sy;
            float font_ex;
            float font_ey;
            float font_width;
            float font_height;
            float font_max_height;
            float font_advance;

            vertex vertex[4];
            vertex[0].z = position_value_.z;
            vertex[1].z = position_value_.z;
            vertex[2].z = position_value_.z;
            vertex[3].z = position_value_.z;

            graphic_font_texture::character_infomation *cur_char_info;

            if (is_outline_) {
                cur_x = stan_x;
                cur_y = stan_y;
                font_max_height = 0.0f;

                graphic_font_texture::p_character_infomation_vector::iterator i;
                for (i = char_info_vector_.begin(); i != char_info_vector_.end(); ++i) {
                    cur_char_info = *i;

                    font_width = float(cur_char_info->width);
                    font_height = float(cur_char_info->height);
                    font_advance = float(cur_char_info->advance);

                    if (cur_x < visible_left) {
                        cur_x += font_advance;
                        continue;
                    }

                    // Text is clipped (or wrapped) at limit_width_.
                    if ((cur_x + font_width) - position_value_.x > limit_width_) {
                        if (is_multi_line_) {
                            cur_x = stan_x;
                            cur_y += font_max_height;
                        } else {
                            break;
                        }
                    }

                    if (clip_rect) {
                        if (cur_y <= clip_rect->top) {
                            cur_x += font_advance;
                            continue;
                        }
                    }

                    font_sx = cur_x - 0.5f;
                    font_sy = cur_y - 0.5f;
                    font_ex = font_sx + font_width;
                    font_ey = font_sy + font_height;

                    font_texture->select_texture(cur_char_info->index_value);
                    STATEMANAGER.set_texture(0, font_texture->gpu_texture());

                    vertex[0].u = cur_char_info->left;
                    vertex[0].v = cur_char_info->top;
                    vertex[1].u = cur_char_info->left;
                    vertex[1].v = cur_char_info->bottom;
                    vertex[2].u = cur_char_info->right;
                    vertex[2].v = cur_char_info->top;
                    vertex[3].u = cur_char_info->right;
                    vertex[3].v = cur_char_info->bottom;

                    vertex[3].color_value = vertex[2].color_value = vertex[1].color_value = vertex[0].color_value = out_line_color_;

                    float feather = 0.0f;

                    vertex[0].y = font_sy - feather;
                    vertex[1].y = font_ey + feather;
                    vertex[2].y = font_sy - feather;
                    vertex[3].y = font_ey + feather;

                    vertex[0].x = font_sx - font_half_weight - feather;
                    vertex[1].x = font_sx - font_half_weight - feather;
                    vertex[2].x = font_ex - font_half_weight + feather;
                    vertex[3].x = font_ex - font_half_weight + feather;

                    if (graphic_base::set_pdt_stream((pdt_vertex *)vertex, 4))
                        STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_STRIP, 0, 2);

                    vertex[0].x = font_sx + font_half_weight - feather;
                    vertex[1].x = font_sx + font_half_weight - feather;
                    vertex[2].x = font_ex + font_half_weight + feather;
                    vertex[3].x = font_ex + font_half_weight + feather;

                    if (graphic_base::set_pdt_stream((pdt_vertex *)vertex, 4))
                        STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_STRIP, 0, 2);

                    vertex[0].x = font_sx - feather;
                    vertex[1].x = font_sx - feather;
                    vertex[2].x = font_ex + feather;
                    vertex[3].x = font_ex + feather;

                    vertex[0].y = font_sy - font_half_weight - feather;
                    vertex[1].y = font_ey - font_half_weight + feather;
                    vertex[2].y = font_sy - font_half_weight - feather;
                    vertex[3].y = font_ey - font_half_weight + feather;

                    if (graphic_base::set_pdt_stream((pdt_vertex *)vertex, 4))
                        STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_STRIP, 0, 2);

                    vertex[0].y = font_sy + font_half_weight - feather;
                    vertex[1].y = font_ey + font_half_weight + feather;
                    vertex[2].y = font_sy + font_half_weight - feather;
                    vertex[3].y = font_ey + font_half_weight + feather;

                    if (graphic_base::set_pdt_stream((pdt_vertex *)vertex, 4))
                        STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_STRIP, 0, 2);

                    cur_x += font_advance;
                }
            }

            cur_x = stan_x;
            cur_y = stan_y;
            font_max_height = 0.0f;

            for (size_t i = 0; i < char_info_vector_.size(); ++i) {
                cur_char_info = char_info_vector_[i];

                font_width = float(cur_char_info->width);
                font_height = float(cur_char_info->height);
                font_max_height = std::max<float>(font_height, cur_char_info->height);
                font_advance = float(cur_char_info->advance);

                if (cur_x < visible_left) {
                    cur_x += font_advance;
                    continue;
                }

                // Text is clipped (or wrapped) at limit_width_.
                if ((cur_x + font_width) - position_value_.x > limit_width_) {
                    if (is_multi_line_) {
                        cur_x = stan_x;
                        cur_y += font_max_height;
                    } else {
                        break;
                    }
                }

                if (clip_rect) {
                    if (cur_y <= clip_rect->top) {
                        cur_x += font_advance;
                        continue;
                    }
                }

                font_sx = cur_x - 0.5f;
                font_sy = cur_y - 0.5f;
                font_ex = font_sx + font_width;
                font_ey = font_sy + font_height;

                font_texture->select_texture(cur_char_info->index_value);
                STATEMANAGER.set_texture(0, font_texture->gpu_texture());

                vertex[0].x = font_sx;
                vertex[0].y = font_sy;
                vertex[0].u = cur_char_info->left;
                vertex[0].v = cur_char_info->top;

                vertex[1].x = font_sx;
                vertex[1].y = font_ey;
                vertex[1].u = cur_char_info->left;
                vertex[1].v = cur_char_info->bottom;

                vertex[2].x = font_ex;
                vertex[2].y = font_sy;
                vertex[2].u = cur_char_info->right;
                vertex[2].v = cur_char_info->top;

                vertex[3].x = font_ex;
                vertex[3].y = font_ey;
                vertex[3].u = cur_char_info->right;
                vertex[3].v = cur_char_info->bottom;

                vertex[0].color_value = vertex[1].color_value = vertex[2].color_value = vertex[3].color_value = color_info_vector_[i];

                if (graphic_base::set_pdt_stream((pdt_vertex *)vertex, 4))
                    STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_STRIP, 0, 2);

                cur_x += font_advance;
            }
        }

        STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);

        STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, fog_enable);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, lighting);

        if (hyperlink_vector_.size() != 0) {
            int lx = mx - position_value_.x;
            int ly = my - position_value_.y;

            if (lx >= 0 && ly >= 0 && lx < text_width_ && ly < text_height_) {
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

    void graphic_text_instance::set_feather(bool value) {
        if (value) {
            font_feather_ = c_fFontFeather;
        } else {
            font_feather_ = 0.0f;
        }
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
        graphic_font_texture::character_infomation *cur_char_info;
        graphic_font_texture::p_character_infomation_vector::iterator itor;

        float fx = 0.0f;
        uint16_t line_count = 1;
        for (itor = char_info_vector_.begin(); itor != char_info_vector_.end(); ++itor) {
            cur_char_info = *itor;

            auto font_width = float(cur_char_info->width);
            auto font_advance = float(cur_char_info->advance);

            // Text is clipped (or wrapped) at limit_width_.
            if (fx + font_width > limit_width_) {
                fx = 0.0f;
                ++line_count;
            }

            fx += font_advance;
        }

        return line_count;
    }

    void graphic_text_instance::text_size(int *ret_width, int *ret_height) {
        *ret_width = text_width_;
        *ret_height = text_height_;
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
        font_feather_ = c_fFontFeather;

        is_update_ = false;

        text_width_ = 0;
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
