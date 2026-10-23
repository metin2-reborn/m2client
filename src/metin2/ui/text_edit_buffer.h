#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace metin2::inline ui {

    // The text model of an edit line: UTF-32 text, caret, selection, limits and modes, no rendering.
    //
    // Positions are visible glyph indices, the units the renderer draws and the mouse hits. The raw
    // text keeps the colour and hyperlink tags of the chat markup ("|cAARRGGBB...|r", "|Hitem:...|h[name]|h",
    // "||" for a bar); each of them is one atom that the caret steps over and Backspace or Delete
    // removes whole, so a pasted item link can never be split. In secret mode nothing is parsed and
    // every code point is one atom shown as '*'.
    class text_edit_buffer {
      public:
        void set_text(std::u32string_view text);
        auto text() const -> const std::u32string & { return text_; }
        auto utf8() const -> std::string;
        auto visible_text() const -> const std::u32string & { return visible_; }

        // Limits are UTF-8 byte counts, the sizes of the fields the text is sent in; 0 is unlimited.
        void set_max_bytes(std::size_t bytes) { max_bytes_ = bytes; }
        // Limit of the text without its tags (the chat line's user maximum).
        void set_max_visible_bytes(std::size_t bytes) { max_visible_bytes_ = bytes; }
        void set_number_mode(bool on) { number_mode_ = on; }
        void set_secret(bool on);
        auto secret() const -> bool { return secret_; }

        // Editing. Each returns whether the text changed; a change that would exceed a limit is refused whole.
        auto insert_char(char32_t cp) -> bool;              // replaces the selection; '|' is stored as "||"
        auto insert_text(std::u32string_view text) -> bool; // paste or item link: the longest prefix of whole atoms that fits
        auto erase_backward() -> bool;                      // the selection, else the atom before the caret
        auto erase_forward() -> bool;                       // the selection, else the atom after the caret
        auto copy() const -> std::u32string;                // raw text of the selection; empty in secret mode
        auto cut() -> std::u32string;                       // copy() and erase; nothing in secret mode

        // Caret and selection. `extend` keeps the anchor (Shift), `word` jumps over a word (Ctrl).
        void move_left(bool extend, bool word);
        void move_right(bool extend, bool word);
        void move_home(bool extend);
        void move_end(bool extend);
        void set_caret(std::size_t visible_index, bool extend); // clamped and snapped to an atom boundary
        void select_all();
        void select_word_at(std::size_t visible_index);
        void clear_selection();

        auto caret() const -> std::size_t;
        auto has_selection() const -> bool { return anchor_ != caret_; }
        auto selection() const -> std::pair<std::size_t, std::size_t>; // visible [begin, end), begin <= end
        auto visible_length() const -> std::size_t { return visible_.size(); }

      private:
        struct atom {
            std::size_t raw_begin, raw_end, visible_begin, visible_end;
        };

        static auto tokenize(std::u32string_view text, bool secret, std::u32string &visible) -> std::vector<atom>;
        void retokenize();
        auto raw_to_visible(std::size_t raw) const -> std::size_t;
        auto visible_to_raw(std::size_t visible) const -> std::size_t;
        auto snap(std::size_t visible) const -> std::size_t;
        auto atom_at(std::size_t visible) const -> const atom *;
        auto accepts(char32_t cp) const -> bool;
        auto fits(std::size_t raw_begin, std::size_t raw_end, std::u32string_view with) const -> bool;
        auto replace(std::size_t raw_begin, std::size_t raw_end, std::u32string_view with) -> bool;
        auto selection_raw() const -> std::pair<std::size_t, std::size_t>;
        void move_to(std::size_t raw, bool extend);

        std::u32string text_;
        std::u32string visible_;
        std::vector<atom> atoms_;
        std::size_t caret_ = 0;  // raw index, always an atom boundary
        std::size_t anchor_ = 0; // raw index; equal to caret_ when nothing is selected
        std::size_t max_bytes_ = 0;
        std::size_t max_visible_bytes_ = 0;
        bool number_mode_ = false;
        bool secret_ = false;
    };
}
