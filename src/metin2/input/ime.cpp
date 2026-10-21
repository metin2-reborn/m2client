// Text input buffer. The original drove the Windows Input Method Manager / Text Services
// Framework for CJK composition. On Linux characters arrive fully composed (raylib's
// GetCharPressed), so this keeps only the text-buffer editing (insert, delete, caret,
// paste, limits, colour tags). Text is UTF-8 outside, UTF-32 inside.
#include "ime.h"
#include <metin2/core/utf8.h>
#include <metin2/graphics/text_tag.h>
#include <metin2/window/main_window.h>

#include <algorithm>
#include <cwctype>

namespace metin2::inline input {

    int cime::curpos;
    int cime::lastpos;
    wchar_t cime::wide_text[IMESTR_MAXLEN];
    bool cime::initialized = false;
    bool cime::ime_enabled = false;
    bool cime::capture_input = false;
    ime_event_sink *cime::event = nullptr;

    namespace {
        // UTF-8 size of wide text.
        auto utf8_size(const wchar_t *w, int len) -> int {
            return static_cast<int>(utf8::from_wide(std::wstring_view(w, static_cast<std::size_t>(len))).size());
        }
    }

    cime::cime() {
        clear();
        max_ = 0;
        user_max_ = 0;
        only_number_mode_ = false;
        enable_paste_ = false;
        use_default_ime_ = false;
    }

    cime::~cime() = default;

    auto cime::initialize() -> bool {
        if (initialized) return true;
        initialized = true;
        ime_enabled = true;
        return true;
    }

    void cime::use_default_ime() { use_default_ime_ = true; }
    void cime::enable_ime(bool enable) { ime_enabled = enable; }
    void cime::disable_ime() { enable_ime(false); }
    void cime::enable_capture_input() { capture_input = true; }
    void cime::disable_capture_input() { capture_input = false; }

    void cime::clear() {
        lastpos = 0;
        curpos = 0;
    }

    void cime::set_max(int max_value) { max_ = max_value; }
    void cime::set_user_max(int max_value) { user_max_ = max_value; }

    void cime::set_text(const char *text, int len) {
        std::wstring w = utf8::to_wide(std::string_view(text, static_cast<std::size_t>(len)));
        lastpos = std::min(static_cast<int>(w.size()), static_cast<int>(IMESTR_MAXLEN) - 1);
        std::copy(w.begin(), w.begin() + lastpos, wide_text);
        wide_text[lastpos] = 0;
        curpos = std::min(curpos, lastpos);
    }

    auto cime::get_text(std::string &text) -> int {
        text += utf8::from_wide(std::wstring_view(wide_text, static_cast<std::size_t>(lastpos)));
        return static_cast<int>(text.size());
    }

    void cime::set_number_mode() { only_number_mode_ = true; }
    void cime::set_string_mode() { only_number_mode_ = false; }
    void cime::add_except_key(wchar_t key) { except_key_.push_back(key); }
    void cime::clear_except_key() { except_key_.clear(); }
    auto cime::is_writable(wchar_t key) -> bool { return except_key_.end() == std::find(except_key_.begin(), except_key_.end(), key); }
    void cime::enable_paste(bool flag) { enable_paste_ = flag; }

    void cime::paste_text_from_clip_board() {
        if (!enable_paste_) return;
        const std::string clipboard = window::clipboard_text();
        if (clipboard.empty()) return;
        paste_string(clipboard.c_str());
    }

    void cime::inc_cur_pos() {
        if (curpos < lastpos) {
            int pos = find_color_tag_end_position(wide_text + curpos, lastpos - curpos);
            if (pos > 0)
                curpos = std::min(lastpos, std::max(0, curpos + (pos + 1)));
            else
                ++curpos;
        }
    }

    void cime::dec_cur_pos() {
        if (curpos > 0) {
            int pos = find_color_tag_start_position(wide_text + curpos - 1, curpos);
            if (pos > 0)
                curpos = std::min(lastpos, std::max(0, curpos - (pos + 1)));
            else
                --curpos;
        }
    }

    auto cime::cur_pos() -> int { return get_text_tag_output_len(wide_text, curpos); }

    void cime::set_cur_pos(int offset) {
        if (offset < 0 || offset > lastpos) {
            curpos = lastpos;
            return;
        }
        curpos = std::min(lastpos, get_text_tag_internal_pos_from_render_pos(wide_text, lastpos, offset));
    }

    void cime::del_cur_pos() {
        if (curpos < lastpos) {
            int erase_count = find_color_tag_end_position(wide_text + curpos, lastpos - curpos) + 1;
            wmemmove(wide_text + curpos, wide_text + curpos + erase_count, (size_t)(lastpos - curpos - erase_count));
            lastpos -= erase_count;
            curpos = std::min(lastpos, curpos);
            wide_text[lastpos] = 0;
        }
    }

    void cime::paste_string(const char *str) {
        std::wstring w = utf8::to_wide(str);
        insert_string(w.c_str(), static_cast<int>(w.size()));
        if (event) event->on_update();
    }

    void cime::insert_string(const wchar_t *string, int size) {
        if (size <= 0 || is_max(string, size)) return;
        if (curpos < lastpos) memmove(wide_text + curpos + size, wide_text + curpos, sizeof(wchar_t) * (lastpos - curpos));
        memcpy(wide_text + curpos, string, sizeof(wchar_t) * size);
        curpos += size;
        lastpos += size;
    }

    void cime::on_char(wchar_t c) {
        if (only_number_mode_ && !iswdigit(c)) return;
        if (!is_writable(c)) return;
        insert_string(&c, 1);
    }

    // Limits are UTF-8 byte counts, so a character that does not fit as a whole is refused.
    auto cime::is_max(const wchar_t *w_input, int len) -> bool {
        if (lastpos + len >= IMESTR_MAXLEN) return true;
        if (utf8_size(wide_text, lastpos) + utf8_size(w_input, len) > max_) return true;
        if (user_max_ != 0 && max_ != user_max_) {
            std::wstring str = get_text_tag_output_string(wide_text, lastpos);
            std::wstring input = get_text_tag_output_string(w_input, len);
            return utf8_size(str.c_str(), (int)str.length()) + utf8_size(input.c_str(), (int)input.length()) > user_max_;
        }
        return false;
    }

    void cime::on_char_input(char32_t codepoint) {
        if (codepoint == 8) {
            if (!capture_input) return;
            if (curpos > 0) {
                dec_cur_pos();
                del_cur_pos();
            }
            if (event) event->on_update();
            return;
        }
        if (event && event->on_char_event(codepoint)) return;
        if (!capture_input) return;
        auto w = static_cast<wchar_t>(codepoint);
        if (w < 0x20 && w != L'\t') return; // Control characters other than tab.
        on_char(w);
        if (w == L'|') on_char(w);
        if (event) event->on_update();
    }
}
