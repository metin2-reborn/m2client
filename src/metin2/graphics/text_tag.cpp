#include "text_tag.h"

namespace metin2::inline graphics {

    namespace {
        template <class CharT>
        auto get_text_tag_impl(const CharT *src, int max_len, int &tag_len, std::basic_string<CharT> &extra_info) -> int {
            tag_len = 1;

            if (max_len < 2 || *src != CharT('|'))
                return TEXT_TAG_PLAIN;

            const CharT *cur = ++src;

            if (*cur == CharT('c')) {
                if (max_len < 10)
                    return TEXT_TAG_PLAIN;

                tag_len = 10;
                extra_info.assign(++cur, 8);
                return TEXT_TAG_COLOR;
            }
            if (*cur == CharT('|')) { // "||" is shown as "|".
                tag_len = 2;
                return TEXT_TAG_TAG;
            }
            if (*cur == CharT('r')) {
                tag_len = 2;
                return TEXT_TAG_RESTORE_COLOR;
            }
            if (*cur == CharT('H')) { // hyperlink |Hitem:10000:0:0:0:0|h[name]|h
                tag_len = 2;
                return TEXT_TAG_HYPERLINK_START;
            }
            if (*cur == CharT('h')) {
                tag_len = 2;
                return TEXT_TAG_HYPERLINK_END;
            }

            return TEXT_TAG_PLAIN;
        }
    }

    auto get_text_tag(const wchar_t *src, int max_len, int &tag_len, std::wstring &extra_info) -> int {
        return get_text_tag_impl(src, max_len, tag_len, extra_info);
    }

    auto get_text_tag(std::u32string_view src, int &tag_len, std::u32string &extra_info) -> int {
        return get_text_tag_impl(src.data(), static_cast<int>(src.size()), tag_len, extra_info);
    }
}
