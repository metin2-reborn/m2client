#include <string>
#include <string_view>

namespace metin2::inline graphics {
    enum {
        TEXT_TAG_PLAIN,
        TEXT_TAG_TAG,             // ||
        TEXT_TAG_COLOR,           // |cffffffff
        TEXT_TAG_HYPERLINK_START, // |H
        TEXT_TAG_HYPERLINK_END,   // |h ex) |Hitem:1234:1:1:1|h
        TEXT_TAG_RESTORE_COLOR,
    };

    // Classifies the markup at src[0]; tag_len is the number of characters it spans (1 for plain text)
    // and extra_info receives the colour digits of a TEXT_TAG_COLOR.
    extern auto get_text_tag(const wchar_t *src, int max_len, int &tag_len, std::wstring &extra_info) -> int;
    extern auto get_text_tag(std::u32string_view src, int &tag_len, std::u32string &extra_info) -> int;
}
