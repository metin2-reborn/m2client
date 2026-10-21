#include "text_tag.h"

namespace metin2::inline graphics {

    auto get_text_tag(const wchar_t *src, int max_len, int &tag_len, std::wstring &extra_info) -> int {
        tag_len = 1;

        if (max_len < 2 || *src != L'|')
            return TEXT_TAG_PLAIN;

        const wchar_t *cur = ++src;

        if (*cur == L'c') {
            if (max_len < 10)
                return TEXT_TAG_PLAIN;

            tag_len = 10;
            extra_info.assign(++cur, 8);
            return TEXT_TAG_COLOR;
        } else if (*cur == L'|') // "||" is shown as "|".
        {
            tag_len = 2;
            return TEXT_TAG_TAG;
        } else if (*cur == L'r') {
            tag_len = 2;
            return TEXT_TAG_RESTORE_COLOR;
        } else if (*cur == L'H') // hyperlink |Hitem:10000:0:0:0:0|h[name]|h
        {
            tag_len = 2;
            return TEXT_TAG_HYPERLINK_START;
        } else if (*cur == L'h') {
            tag_len = 2;
            return TEXT_TAG_HYPERLINK_END;
        }

        return TEXT_TAG_PLAIN;
    }

    auto get_text_tag_output_string(const wchar_t *src, int src_len) -> std::wstring {
        int len;
        std::wstring dst;
        std::wstring extra_info;
        int hyperlink_step = 0;

        for (int i = 0; i < src_len;) {
            int tag = get_text_tag(&src[i], src_len - i, len, extra_info);

            if (tag == TEXT_TAG_PLAIN || tag == TEXT_TAG_TAG) {
                if (hyperlink_step == 0) {
                    dst += src[i];
                }
            } else if (tag == TEXT_TAG_HYPERLINK_START)
                hyperlink_step = 1;
            else if (tag == TEXT_TAG_HYPERLINK_END)
                hyperlink_step = 0;

            i += len;
        }
        return dst;
    }

    auto get_text_tag_internal_pos_from_render_pos(const wchar_t *src, int src_len, int offset) -> int {
        int len;
        std::wstring dst;
        std::wstring extra_info;
        int output_len = 0;
        int hyperlink_step = 0;
        bool color_tag = false;
        int internal_offset = 0;

        for (int i = 0; i < src_len;) {
            int tag = get_text_tag(&src[i], src_len - i, len, extra_info);

            if (tag == TEXT_TAG_COLOR) {
                color_tag = true;
                internal_offset = i;
            } else if (tag == TEXT_TAG_RESTORE_COLOR) {
                color_tag = false;
            } else if (tag == TEXT_TAG_PLAIN || tag == TEXT_TAG_TAG) {
                if (hyperlink_step == 0) {
                    if (!color_tag)
                        internal_offset = i;

                    if (offset <= output_len)
                        return internal_offset;

                    ++output_len;
                    dst += src[i];
                }
            } else if (tag == TEXT_TAG_HYPERLINK_START)
                hyperlink_step = 1;
            else if (tag == TEXT_TAG_HYPERLINK_END)
                hyperlink_step = 0;

            i += len;
        }

        return internal_offset;
    }

    auto get_text_tag_output_len(const wchar_t *src, int src_len) -> int {
        int len;
        std::wstring extra_info;
        int output_len = 0;
        int hyperlink_step = 0;

        for (int i = 0; i < src_len;) {
            int tag = get_text_tag(&src[i], src_len - i, len, extra_info);

            if (tag == TEXT_TAG_PLAIN || tag == TEXT_TAG_TAG) {
                if (hyperlink_step == 0)
                    ++output_len;
            } else if (tag == TEXT_TAG_HYPERLINK_START)
                hyperlink_step = 1;
            else if (tag == TEXT_TAG_HYPERLINK_END)
                hyperlink_step = 0;

            i += len;
        }
        return output_len;
    }

    auto find_color_tag_start_position(const wchar_t *src, int src_len) -> int {
        if (src_len < 2)
            return 0;

        const wchar_t *cur = src;

        // the |r case
        if (*cur == L'r' && *(cur - 1) == L'|') {
            int len = src_len;

            // "||r" is not a restore tag.
            if (len >= 2 && *(cur - 2) == L'|')
                return 1;

            cur -= 2;
            len -= 2;

            // Stop at the next "|c" and return the length up to its "|".
            while (len > 1) {
                if (*cur == L'c' && *(cur - 1) == L'|')
                    return (src - cur) + 1;

                --cur;
                --len;
            }
            return (src_len);
        }
        // the || case
        else if (*cur == L'|' && *(cur - 1) == L'|')
            return 1;

        return 0;
    }

    auto find_color_tag_end_position(const wchar_t *src, int src_len) -> int {
        const wchar_t *cur = src;

        if (src_len >= 4 && *cur == L'|' && *(cur + 1) == L'c') {
            int left = src_len - 2;
            cur += 2;

            while (left > 1) {
                if (*cur == L'|' && *(cur + 1) == L'r')
                    return (cur - src) + 1;

                --left;
                ++cur;
            }
        } else if (src_len >= 2 && *cur == L'|' && *(cur + 1) == L'|')
            return 1;

        return 0;
    }
}
