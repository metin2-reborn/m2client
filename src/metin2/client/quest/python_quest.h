#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace metin2::inline client::inline quest {

    class python_quest {
      public:
        // The server's quest status (questpc.h quest_status).
        enum status : uint8_t {
            STATUS_ACTIVE = 0,
            STATUS_NEW = 1,
            STATUS_TURN_IN = 2,
        };

        struct quest_instance {
            quest_instance() {
                index_value = 0;
                clock_value = 0;
                counter_value = 0;
                start_time = 0;
                quest_status = STATUS_ACTIVE;
                has_letter = false;
            }

            uint32_t index_value;

            std::string icon_file_name;
            std::string title;

            std::string clock_name;
            std::string counter_name;

            int clock_value;
            int counter_value;

            int start_time;

            uint8_t quest_status;
            bool has_letter;
        };
        using quest_instance_container = std::vector<quest_instance>;

      public:
        python_quest();
        ~python_quest();

        void clear();

        void delete_quest_instance(uint32_t index);

        auto is_quest(uint32_t index) -> bool;
        void make_quest(uint32_t index);

        void set_quest_title(uint32_t index, const char *title);
        void set_quest_clock_name(uint32_t index, const char *clock_name);
        void set_quest_counter_name(uint32_t index, const char *counter_name);
        void set_quest_clock_value(uint32_t index, int clock_value);
        void set_quest_counter_value(uint32_t index, int counter_value);
        void set_quest_icon_file_name(uint32_t index, const char *icon_file_name);
        void set_quest_status(uint32_t index, uint8_t quest_status);
        void set_quest_letter(uint32_t index, bool has_letter);

        auto quest_count() -> int;
        auto get_quest_instance_ptr(uint32_t array_index, quest_instance **quest_instance_value) -> bool;

      protected:
        void initialize_impl();
        auto quest_instance_ptr(uint32_t quest_index, quest_instance **quest_instance_value) -> bool;

      protected:
        quest_instance_container quest_instance_container_value_;
    };

    inline auto get_python_quest() -> python_quest & {
        static python_quest instance;
        return instance;
    }
}
