#include <string>

namespace metin2::inline graphics {
    enum {
        TEXT_TAG_PLAIN,
        TEXT_TAG_TAG,             // ||
        TEXT_TAG_COLOR,           // |cffffffff
        TEXT_TAG_HYPERLINK_START, // |H
        TEXT_TAG_HYPERLINK_END,   // |h ex) |Hitem:1234:1:1:1|h
        TEXT_TAG_RESTORE_COLOR,
    };

    extern auto get_text_tag(const wchar_t *src, int max_len, int &tag_len, std::wstring &extra_info) -> int;
    extern auto get_text_tag_output_string(const wchar_t *src, int src_len) -> std::wstring;
    extern auto get_text_tag_output_len(const wchar_t *src, int len) -> int;
    extern auto find_color_tag_end_position(const wchar_t *src, int src_len) -> int;
    extern auto find_color_tag_start_position(const wchar_t *src, int src_len) -> int;
    extern auto get_text_tag_internal_pos_from_render_pos(const wchar_t *src, int src_len, int offset) -> int;
}
