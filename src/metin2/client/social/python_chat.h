#pragma once

#include <deque>
#include <list>
#include <metin2/client/app/abstract_chat.h>
#include <metin2/client/protocol/packet.h>
#include <metin2/graphics/grp_text_instance.h>
#include <metin2/script/python_header.h>
#include <set>

namespace metin2::inline client::inline social {

    class whisper {
      public:
        struct chat_line {
            graphic_text_instance instance;

            chat_line() = default;
            ~chat_line() {
                instance.destroy();
            }

            static void destroy_system();

            static auto new_value() -> chat_line *;
            static void delete_value(chat_line *chat_line);

            static dynamic_pool<chat_line> pool_value;
        };

        using chat_line_deque = std::deque<chat_line *>;
        using chat_line_list = std::list<chat_line *>;

      public:
        whisper();
        ~whisper();

        void destroy();

        void set_position(float position);
        void set_box_size(float width_value, float height);
        void append_chat(int type, const char *chat);
        void render(float fx, float fy);

      protected:
        void initialize_impl();
        void arrange_chat();

      protected:
        float line_step_;
        float width_;
        float height_;
        float fcur_position_;

        chat_line_deque chat_line_deque_value_;
        chat_line_list showing_chat_line_list_;

      public:
        static void destroy_system();

        static auto new_value() -> whisper *;
        static void delete_value(whisper *whisper);

        static dynamic_pool<whisper> pool_value;
    };

    class python_chat : public abstract_chat {
      public:
        enum whisper_type {
            WHISPER_TYPE_CHAT = 0,
            WHISPER_TYPE_NOT_EXIST = 1,
            WHISPER_TYPE_TARGET_BLOCKED = 2,
            WHISPER_TYPE_SENDER_BLOCKED = 3,
            WHISPER_TYPE_ERROR = 4,
            WHISPER_TYPE_GM = 5,
            WHISPER_TYPE_SYSTEM = 0xFF
        };

        enum board_state {
            BOARD_STATE_VIEW,
            BOARD_STATE_EDIT,
            BOARD_STATE_LOG,
        };

        enum {
            CHAT_LINE_MAX_NUM = 300,
            CHAT_LINE_COLOR_ARRAY_MAX_NUM = 3,
        };

        struct chat_line {
            int type_value;
            float appended_time;
            math::color color_value[CHAT_LINE_COLOR_ARRAY_MAX_NUM];
            graphic_text_instance instance;

            chat_line();
            virtual ~chat_line();

            void set_color(uint32_t id, uint32_t color);
            void set_color_all(uint32_t color);
            auto color_ref(uint32_t id) -> math::color &;
            static void destroy_system();

            static auto new_value() -> chat_line *;
            static void delete_value(chat_line *chat_line);

            static dynamic_pool<chat_line> pool_value;
        };

        struct wait_chat {
            int type_value;
            std::string chat;

            uint32_t appending_time;
        };

        using chat_line_deque = std::deque<chat_line *>;
        using chat_line_list = std::list<chat_line *>;

        using whisper_map = std::map<std::string, whisper *>;
        using ignore_character_set = std::set<std::string>;
        using wait_chat_list = std::list<wait_chat>;

        struct chat_set {
            int ix;
            int iy;
            int height;
            int step;
            float end_pos;

            int board_state_value;
            std::vector<int> mode_value;

            chat_line_list showing_chat_line_list;

            auto check_mode(uint32_t type) -> bool {
                if (type >= mode_value.size())
                    return false;

                return mode_value[type] ? true : false;
            }

            chat_set() {
                board_state_value = BOARD_STATE_VIEW;

                ix = 0;
                iy = 0;
                end_pos = 1.0f;
                height = 0;
                step = 15;

                mode_value.clear();
                mode_value.resize(chat_mode_size, 1);
            }

            static int chat_mode_size;
        };

        using chat_set_map = std::map<int, chat_set>;

      public:
        python_chat();
        ~python_chat();

        void set_chat_color(uint32_t type, uint32_t r, uint32_t g, uint32_t b);

        void destroy();
        void close();

        auto create_chat_set(uint32_t id) -> int;
        void update(uint32_t id);
        void render(uint32_t id);

        void set_board_state(uint32_t id, int state);
        void set_position(uint32_t id, int ix, int iy);
        void set_height(uint32_t id, int height);
        void toggle_chat_mode(uint32_t id, int mode);
        void enable_chat_mode(uint32_t id, int mode);
        void disable_chat_mode(uint32_t id, int mode);
        void set_end_pos(uint32_t id, float pos);

        auto visible_line_count(uint32_t id) -> int;
        auto editable_line_count(uint32_t id) -> int;
        auto line_count(uint32_t id) -> int;
        auto line_step(uint32_t id) -> int;

        void append_chat(int type, const char *chat) override;
        void append_chat_with_delay(int type, const char *chat, int delay);
        void arrange_showing_chat(uint32_t id);

        auto create_whisper(const char *name) -> whisper *;
        void append_whisper(int type, const char *name, const char *chat);
        void clear_whisper(const char *name);
        auto get_whisper(const char *name, whisper **whisper) -> bool;
        void init_whisper(PyObject *ppy_object);

      protected:
        void initialize_impl();
        void destroy_whisper_map();

        auto chat_line_list_ptr(uint32_t id) -> chat_line_list *;
        auto chat_set_ptr(uint32_t id) -> chat_set *;

        void update_view_mode(uint32_t id);
        void update_edit_mode(uint32_t id);
        void update_log_mode(uint32_t id);

        auto chat_color(int type) -> uint32_t;

      protected:
        chat_line_deque chat_line_deque_value_;
        chat_line_list showing_chat_line_list_;
        chat_set_map chat_set_map_value_;
        whisper_map whisper_map_value_;
        ignore_character_set ignore_character_set_value_;
        wait_chat_list wait_chat_list_value_;

        math::color chat_colors_[CHAT_TYPE_MAX_NUM];
    };

    inline auto get_python_chat() -> python_chat & {
        static python_chat instance;
        return instance;
    }
}
