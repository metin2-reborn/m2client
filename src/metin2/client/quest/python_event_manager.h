#pragma once

#include <metin2/core/parser.h>

#include <map>
#include <metin2/graphics/grp_text_instance.h>
#include <metin2/script/python_header.h>
#include <string>

namespace metin2::inline client::inline quest {

    class python_event_manager {
      public:
        struct text_line {
            int ix_local, iy_local;
            graphic_text_instance *instance;
        };

        using script_text_line_list = std::list<text_line>;
        using event_answer_map = std::map<int, std::string>;

        enum {
            EVENT_POSITION_START = 0,
            EVENT_POSITION_END = 1,

            BOX_VISIBLE_LINE_COUNT = 5,
        };

        enum button_type {
            BUTTON_TYPE_NEXT,
            BUTTON_TYPE_DONE,
            BUTTON_TYPE_CANCEL,
        };

        enum event_type {
            EVENT_TYPE_LETTER,
            EVENT_TYPE_COLOR,
            EVENT_TYPE_DELAY,
            EVENT_TYPE_ENTER,
            EVENT_TYPE_WAIT,
            EVENT_TYPE_CLEAR,
            EVENT_TYPE_QUESTION,
            EVENT_TYPE_NEXT,
            EVENT_TYPE_DONE,

            EVENT_TYPE_LEFT_IMAGE,
            EVENT_TYPE_TOP_IMAGE,
            EVENT_TYPE_BACKGROUND_IMAGE,
            EVENT_TYPE_IMAGE,

            EVENT_TYPE_ADD_MAP_SIGNAL,
            EVENT_TYPE_CLEAR_MAP_SIGNAL,
            EVENT_TYPE_SET_MESSAGE_POSITION,
            EVENT_TYPE_ADJUST_MESSAGE_POSITION,
            EVENT_TYPE_SET_CENTER_MAP_POSITION,

            EVENT_TYPE_QUEST,

            EVENT_TYPE_SLEEP,
            EVENT_TYPE_SET_CAMERA,
            EVENT_TYPE_BLEND_CAMERA,
            EVENT_TYPE_RESTORE_CAMERA,
            EVENT_TYPE_FADE_OUT,
            EVENT_TYPE_FADE_IN,
            EVENT_TYPE_WHITE_OUT,
            EVENT_TYPE_WHITE_IN,
            EVENT_TYPE_CLEAR_TEXT,
            EVENT_TYPE_TEXT_HORIZONTAL_ALIGN_CENTER,
            EVENT_TYPE_TITLE_IMAGE,

            EVENT_TYPE_RUN_CINEMA,
            EVENT_TYPE_DUNGEON_RESULT,

            EVENT_TYPE_ITEM_NAME,
            EVENT_TYPE_MONSTER_NAME,

            EVENT_TYPE_COLOR256,
            EVENT_TYPE_WINDOW_SIZE,

            EVENT_TYPE_INPUT,
            EVENT_TYPE_CONFIRM_WAIT,
            EVENT_TYPE_END_CONFIRM_WAIT,

            EVENT_TYPE_INSERT_IMAGE,

            EVENT_TYPE_SELECT_ITEM,
        };

      private:
        std::map<std::string, int> event_type_map_;

      public:
        struct event_set {
            int ix, iy;
            int width;
            int iy_local;

            bool is_lock;

            int32_t last_delay_time;

            int current_letter;

            math::color current_color;
            std::string current_line;

            graphic_text_instance *current_text_line;
            script_text_line_list script_text_line_list_value;

            bool is_confirm_wait;
            graphic_text_instance *confirm_time_text_line;
            int confirm_end_time;

            event_script::group script_group;

            char file_name[32 + 1];

            int visible_start_line;
            int visible_line_count;

            int adjust_line;

            math::color diffuse_color;
            int32_t waiting_time;
            int restricted_character_count;

            int answer;

            bool is_text_center_mode;
            bool is_wait_flag;

            PyObject *event_handler_value;

            event_set() = default;
            virtual ~event_set() = default;
        };

        using event_set_vector = std::vector<event_set *>;

      public:
        python_event_manager();
        ~python_event_manager();

        void destroy();

        auto register_event_set(const char *file_name) -> int;
        auto register_event_set_from_string(const std::string &script) -> int;
        void clear_event_seti(int index);
        void clear_event_setp(event_set *event_set_value);

        void set_event_handler(int index, PyObject *event_handler);
        void set_restricted_count(int index, int count);

        auto event_set_local_y_position(int index) -> int;
        void add_event_set_local_y_position(int index, int add_value);
        void insert_text(int index, const char *text, int x_pos = 0);

        void update_event_set(int index, int ix, int iy);
        void render_event_set(int index);
        void set_event_set_width(int index, int width_value);

        void skip(int index);
        void finish(int index);
        auto is_wait(int index) -> bool;
        void end_event_process(int index);

        void select_answer(int index, int answer);
        void set_visible_start_line(int index, int start_line);
        auto visible_start_line(int index) -> int;
        auto line_count(int index) -> int;
        void set_visible_line_count(int index, int line_count);

        void set_interface_window(PyObject *interface_value);
        void set_left_time_string(const char *string);

      protected:
        void init_event_set(event_set &event_set_value);
        void insert_line(event_set &event_set_value, bool is_center = false, int x_pos = 0);
        void add_space(event_set &event_set_value, int space);

        auto empty_event_set_slot() -> uint32_t;

        auto check_event_set_index(int index) -> bool;

        void process_event_set(event_set *event_set_value);

        void clear_line(event_set *event_set_value);
        void refresh_line_position(event_set *event_set_value);
        auto script_event_index(const char *name, int *event_position, int *event_type_value) -> bool;

        void make_question(event_set *event_set_value, event_script::arg_list &argument_list);
        void make_next_button(event_set *event_set_value, int button_type_value);

      protected:
        event_set_vector event_set_vector_value_;
        bool is_quest_confirm_wait_;

      private:
        dynamic_pool<event_set> event_set_pool_;
        dynamic_pool<graphic_text_instance> script_text_line_pool_;
        PyObject *interface_;
        std::string left_time_string_;
    };

    inline auto get_python_event_manager() -> python_event_manager & {
        static python_event_manager instance;
        return instance;
    }
}
