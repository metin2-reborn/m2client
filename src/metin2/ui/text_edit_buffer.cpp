#include "text_edit_buffer.h"

#include <metin2/core/utf8.h>
#include <metin2/graphics/text_tag.h>

#include <algorithm>

namespace metin2::inline ui {

    namespace {
        auto utf8_size(std::u32string_view text) -> std::size_t {
            std::size_t size = 0;
            for (char32_t cp : text)
                size += cp < 0x80 ? 1 : cp < 0x800 ? 2
                                    : cp < 0x10000 ? 3
                                                   : 4;
            return size;
        }

        auto is_space(char32_t cp) -> bool {
            return cp == U' ' || cp == U'\t';
        }

        // Steps over "|H...|h<text>|h" starting at the "|H"; the link text's glyphs go to `visible`.
        auto consume_link(std::u32string_view text, std::size_t i, std::u32string &visible) -> std::size_t {
            int tag_len = 0;
            std::u32string extra;
            i += 2;
            bool in_address = true;
            while (i < text.size()) {
                const int tag = get_text_tag(text.substr(i), tag_len, extra);
                if (tag == TEXT_TAG_HYPERLINK_END) {
                    i += static_cast<std::size_t>(tag_len);
                    if (!in_address)
                        break;
                    in_address = false;
                    continue;
                }
                if (!in_address && (tag == TEXT_TAG_PLAIN || tag == TEXT_TAG_TAG))
                    visible += tag == TEXT_TAG_TAG ? U'|' : text[i];
                i += static_cast<std::size_t>(tag_len);
            }
            return i;
        }
    }

    auto text_edit_buffer::tokenize(std::u32string_view text, bool secret, std::u32string &visible) -> std::vector<atom> {
        std::vector<atom> atoms;
        visible.clear();
        int tag_len = 0;
        std::u32string extra;
        std::size_t i = 0;
        while (i < text.size()) {
            atom a{i, i, visible.size(), visible.size()};
            if (secret) {
                visible += U'*';
                ++i;
            } else {
                const int tag = get_text_tag(text.substr(i), tag_len, extra);
                switch (tag) {
                case TEXT_TAG_PLAIN:
                    visible += text[i];
                    ++i;
                    break;
                case TEXT_TAG_TAG:
                    visible += U'|';
                    i += 2;
                    break;
                case TEXT_TAG_HYPERLINK_START:
                    i = consume_link(text, i, visible);
                    break;
                case TEXT_TAG_COLOR:
                    {
                        // Up to and including the matching "|r"; what is inside renders as plain glyphs.
                        i += static_cast<std::size_t>(tag_len);
                        while (i < text.size()) {
                            const int inner = get_text_tag(text.substr(i), tag_len, extra);
                            if (inner == TEXT_TAG_RESTORE_COLOR) {
                                i += static_cast<std::size_t>(tag_len);
                                break;
                            }
                            if (inner == TEXT_TAG_HYPERLINK_START) {
                                i = consume_link(text, i, visible);
                                continue;
                            }
                            if (inner == TEXT_TAG_PLAIN || inner == TEXT_TAG_TAG)
                                visible += inner == TEXT_TAG_TAG ? U'|' : text[i];
                            i += static_cast<std::size_t>(tag_len);
                        }
                        break;
                    }
                default: // a stray "|r" or "|h": no glyph
                    i += static_cast<std::size_t>(tag_len);
                    break;
                }
            }
            a.raw_end = i;
            a.visible_end = visible.size();
            atoms.push_back(a);
        }

        // An atom without glyphs would give two boundaries the same visible index: it joins a neighbour.
        for (std::size_t k = 0; k < atoms.size();) {
            if (atoms[k].visible_begin != atoms[k].visible_end || atoms.size() == 1) {
                ++k;
                continue;
            }
            if (k + 1 < atoms.size()) {
                atoms[k + 1].raw_begin = atoms[k].raw_begin;
                atoms.erase(atoms.begin() + static_cast<std::ptrdiff_t>(k));
            } else {
                atoms[k - 1].raw_end = atoms[k].raw_end;
                atoms.pop_back();
            }
        }
        return atoms;
    }

    void text_edit_buffer::retokenize() {
        atoms_ = tokenize(text_, secret_, visible_);
    }

    void text_edit_buffer::set_text(std::u32string_view text) {
        text_.assign(text);
        retokenize();
        caret_ = anchor_ = text_.size();
    }

    auto text_edit_buffer::utf8() const -> std::string {
        return utf8::from_u32(text_);
    }

    void text_edit_buffer::set_secret(bool on) {
        secret_ = on;
        retokenize();
        caret_ = anchor_ = text_.size();
    }

    auto text_edit_buffer::raw_to_visible(std::size_t raw) const -> std::size_t {
        for (const atom &a : atoms_)
            if (a.raw_begin >= raw)
                return a.visible_begin;
        return visible_.size();
    }

    auto text_edit_buffer::visible_to_raw(std::size_t visible) const -> std::size_t {
        for (const atom &a : atoms_)
            if (a.visible_begin >= visible)
                return a.raw_begin;
        return text_.size();
    }

    auto text_edit_buffer::atom_at(std::size_t visible) const -> const atom * {
        for (const atom &a : atoms_)
            if (a.visible_begin <= visible && visible < a.visible_end)
                return &a;
        return nullptr;
    }

    auto text_edit_buffer::snap(std::size_t visible) const -> std::size_t {
        visible = std::min(visible, visible_.size());
        const atom *a = atom_at(visible);
        if (!a || a->visible_begin == visible)
            return visible;
        return visible - a->visible_begin <= a->visible_end - visible ? a->visible_begin : a->visible_end;
    }

    auto text_edit_buffer::caret() const -> std::size_t {
        return raw_to_visible(caret_);
    }

    auto text_edit_buffer::selection() const -> std::pair<std::size_t, std::size_t> {
        const std::size_t anchor = raw_to_visible(anchor_);
        const std::size_t caret = raw_to_visible(caret_);
        return {std::min(anchor, caret), std::max(anchor, caret)};
    }

    auto text_edit_buffer::selection_raw() const -> std::pair<std::size_t, std::size_t> {
        return {std::min(anchor_, caret_), std::max(anchor_, caret_)};
    }

    auto text_edit_buffer::accepts(char32_t cp) const -> bool {
        if (cp < 0x20)
            return false;
        return !number_mode_ || (cp >= U'0' && cp <= U'9');
    }

    auto text_edit_buffer::fits(std::size_t raw_begin, std::size_t raw_end, std::u32string_view with) const -> bool {
        if (max_bytes_ == 0 && max_visible_bytes_ == 0)
            return true;
        std::u32string candidate = text_;
        candidate.replace(raw_begin, raw_end - raw_begin, with);
        if (max_bytes_ != 0 && utf8_size(candidate) > max_bytes_)
            return false;
        if (max_visible_bytes_ != 0) {
            std::u32string visible;
            tokenize(candidate, secret_, visible);
            if (utf8_size(visible) > max_visible_bytes_)
                return false;
        }
        return true;
    }

    auto text_edit_buffer::replace(std::size_t raw_begin, std::size_t raw_end, std::u32string_view with) -> bool {
        if (raw_begin == raw_end && with.empty())
            return false;
        if (!fits(raw_begin, raw_end, with))
            return false;
        text_.replace(raw_begin, raw_end - raw_begin, with);
        retokenize();
        caret_ = anchor_ = raw_begin + with.size();
        return true;
    }

    auto text_edit_buffer::insert_char(char32_t cp) -> bool {
        if (!accepts(cp))
            return false;
        const auto [begin, end] = selection_raw();
        if (cp == U'|' && !secret_)
            return replace(begin, end, U"||");
        return replace(begin, end, std::u32string_view(&cp, 1));
    }

    auto text_edit_buffer::insert_text(std::u32string_view text) -> bool {
        std::u32string accepted;
        for (char32_t cp : text)
            if (accepts(cp) || (cp == U'|' && !secret_))
                accepted += cp;

        std::u32string visible;
        const std::vector<atom> atoms = tokenize(accepted, secret_, visible);
        const auto [begin, end] = selection_raw();
        for (std::size_t count = atoms.size(); count > 0; --count) {
            const std::u32string_view prefix(accepted.data(), atoms[count - 1].raw_end);
            if (fits(begin, end, prefix))
                return replace(begin, end, prefix);
        }
        return false;
    }

    auto text_edit_buffer::erase_backward() -> bool {
        if (has_selection()) {
            const auto [begin, end] = selection_raw();
            return replace(begin, end, {});
        }
        for (const atom &a : atoms_)
            if (a.raw_end == caret_)
                return replace(a.raw_begin, a.raw_end, {});
        return false;
    }

    auto text_edit_buffer::erase_forward() -> bool {
        if (has_selection()) {
            const auto [begin, end] = selection_raw();
            return replace(begin, end, {});
        }
        for (const atom &a : atoms_)
            if (a.raw_begin == caret_)
                return replace(a.raw_begin, a.raw_end, {});
        return false;
    }

    auto text_edit_buffer::copy() const -> std::u32string {
        if (secret_)
            return {};
        const auto [begin, end] = selection_raw();
        return text_.substr(begin, end - begin);
    }

    auto text_edit_buffer::cut() -> std::u32string {
        if (secret_)
            return {};
        std::u32string copied = copy();
        if (!copied.empty())
            erase_forward();
        return copied;
    }

    void text_edit_buffer::move_to(std::size_t raw, bool extend) {
        caret_ = raw;
        if (!extend)
            anchor_ = caret_;
    }

    void text_edit_buffer::move_left(bool extend, bool word) {
        if (!extend && has_selection() && !word) {
            move_to(selection_raw().first, false);
            return;
        }
        std::size_t visible = caret();
        if (word) {
            while (visible > 0 && is_space(visible_[visible - 1]))
                --visible;
            while (visible > 0 && !is_space(visible_[visible - 1]))
                --visible;
            if (const atom *a = atom_at(visible); a && a->visible_begin != visible)
                visible = a->visible_begin;
        } else if (visible > 0) {
            visible = atom_at(visible - 1)->visible_begin;
        }
        move_to(visible_to_raw(visible), extend);
    }

    void text_edit_buffer::move_right(bool extend, bool word) {
        if (!extend && has_selection() && !word) {
            move_to(selection_raw().second, false);
            return;
        }
        std::size_t visible = caret();
        if (word) {
            while (visible < visible_.size() && !is_space(visible_[visible]))
                ++visible;
            while (visible < visible_.size() && is_space(visible_[visible]))
                ++visible;
            if (const atom *a = atom_at(visible); a && a->visible_begin != visible)
                visible = a->visible_end;
        } else if (visible < visible_.size()) {
            visible = atom_at(visible)->visible_end;
        }
        move_to(visible_to_raw(visible), extend);
    }

    void text_edit_buffer::move_home(bool extend) {
        move_to(0, extend);
    }

    void text_edit_buffer::move_end(bool extend) {
        move_to(text_.size(), extend);
    }

    void text_edit_buffer::set_caret(std::size_t visible_index, bool extend) {
        move_to(visible_to_raw(snap(visible_index)), extend);
    }

    void text_edit_buffer::select_all() {
        anchor_ = 0;
        caret_ = text_.size();
    }

    void text_edit_buffer::select_word_at(std::size_t visible_index) {
        if (visible_.empty())
            return;
        std::size_t begin = std::min(visible_index, visible_.size() - 1);
        const bool space = is_space(visible_[begin]);
        std::size_t end = begin + 1;
        while (begin > 0 && is_space(visible_[begin - 1]) == space)
            --begin;
        while (end < visible_.size() && is_space(visible_[end]) == space)
            ++end;
        if (const atom *a = atom_at(begin); a)
            begin = a->visible_begin;
        if (const atom *a = atom_at(end); a && a->visible_begin != end)
            end = a->visible_end;
        anchor_ = visible_to_raw(begin);
        caret_ = visible_to_raw(end);
    }

    void text_edit_buffer::clear_selection() {
        anchor_ = caret_;
    }
}
