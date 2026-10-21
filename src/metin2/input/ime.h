#pragma once

#include <string>
#include <vector>

namespace metin2::inline input {
    class ime_event_sink {
      public:
        virtual auto on_char_event(char32_t codepoint) -> bool = 0;
        virtual void on_update() = 0;
    };

    class cime {
      public:
        enum {
            IMESTR_MAXLEN = 1024,
        };

      public:
        cime();

        virtual ~cime();

        auto initialize() -> bool;

        static void clear();

        // Limits in UTF-8 bytes (the size of the network or storage field the text goes to).
        void set_max(int max_value);
        void set_user_max(int max_value);
        void set_text(const char *text, int len);
        auto get_text(std::string &text) -> int;

        void enable_ime(bool enable = true);
        void disable_ime();

        void enable_capture_input();
        void disable_capture_input();

        void set_number_mode();
        void set_string_mode();
        auto is_writable(wchar_t key) -> bool;
        void add_except_key(wchar_t key);
        void clear_except_key();

        void paste_text_from_clip_board();
        void enable_paste(bool flag);
        void paste_string(const char *str);

        void use_default_ime();

        static auto cur_pos() -> int;

        // A typed character; 8 is backspace.
        void on_char_input(char32_t codepoint);

      protected:
        void inc_cur_pos();
        void dec_cur_pos();
        void set_cur_pos(int offset);
        void del_cur_pos();

        void insert_string(const wchar_t *string, int size);

        void on_char(wchar_t c);

        auto is_max(const wchar_t *input, int len) -> bool;

      protected:
        int max_;
        int user_max_;

        bool only_number_mode_;

        std::vector<wchar_t> except_key_;

        bool enable_paste_;
        bool use_default_ime_;

      public:
        static bool initialized;
        static bool ime_enabled;
        static bool capture_input;

        static ime_event_sink *event;

        static wchar_t wide_text[IMESTR_MAXLEN];

        static int curpos;
        static int lastpos;
    };
}
