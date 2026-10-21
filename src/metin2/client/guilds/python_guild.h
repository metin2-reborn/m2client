#pragma once

#include <metin2/client/protocol/packet.h>

namespace metin2::inline client::inline guilds {

    class python_guild {
      public:
        enum {
            GUILD_SKILL_MAX_NUM = 12,
            ENEMY_GUILD_SLOT_MAX_COUNT = 6,
        };

        struct gulid_info {
            uint32_t guild_id;
            char guild_name[GUILD_NAME_MAX_LEN + 1];
            uint32_t master_pid;
            uint32_t guild_level;
            uint32_t current_experience;
            uint32_t current_member_count;
            uint32_t max_member_count;
            uint32_t guild_money;
            bool has_land;
        };
        using guild_info = gulid_info;

        struct guild_grade_data {
            guild_grade_data() = default;
            guild_grade_data(uint8_t authority_flag_value, const char *name) : authority_flag(authority_flag_value), name_value(name) {}
            uint8_t authority_flag;
            std::string name_value;
        };
        using grade_data_map = std::map<uint8_t, guild_grade_data>;

        struct guild_member_data {
            uint32_t pid;

            std::string name_value;
            uint8_t grade;
            uint8_t job;
            uint8_t level_value;
            uint8_t general_flag;
            uint32_t offer;
        };
        using guild_member_data_vector = std::vector<guild_member_data>;

        struct guild_board_comment_data {
            uint32_t comment_id;
            std::string name_value;
            std::string comment;
        };
        using guild_board_comment_data_vector = std::vector<guild_board_comment_data>;

        struct guild_skill_data {
            uint8_t skill_point;
            uint8_t skill_level[GUILD_SKILL_MAX_NUM];
            uint16_t guild_point;
            uint16_t max_guild_point;
        };

        using guild_name_map = std::map<uint32_t, std::string>;

      public:
        python_guild();
        ~python_guild();

        void destroy();

        void enable_guild();
        void set_guild_money(uint32_t money);
        void set_guild_exp(uint8_t level, uint32_t exp);
        void set_grade_data(uint8_t grade_number, guild_grade_data &guild_grade_data_value);
        void set_grade_name(uint8_t grade_number, const char *name);
        void set_grade_authority(uint8_t grade_number, uint8_t authority);
        void clear_comment();
        void register_comment(uint32_t comment_id, const char *name, const char *comment);
        void register_member(guild_member_data &guild_member_data_value);
        void change_guild_member_grade(uint32_t pid, uint8_t grade);
        void change_guild_member_general_flag(uint32_t pid, uint8_t flag);
        void remove_member(uint32_t pid);
        void register_guild_name(uint32_t id, const char *name);

        auto is_main_player(uint32_t pid) -> bool;
        auto is_guild_enable() -> bool;
        auto guild_info_ref() -> guild_info &;
        auto grade_data_ptr(uint32_t grade_number, guild_grade_data **data) -> bool;
        auto guild_board_comment_vector() -> const guild_board_comment_data_vector &;
        auto member_count() -> uint32_t;
        auto member_data_ptr(uint32_t index, guild_member_data **data) -> bool;
        auto member_data_ptr_by_pid(uint32_t pid, guild_member_data **data) -> bool;
        auto member_data_ptr_by_name(const char *name, guild_member_data **data) -> bool;
        auto guild_member_level_average() -> uint32_t;
        auto guild_experience_summary() -> uint32_t;
        auto guild_skill_data_ref() -> guild_skill_data &;
        auto guild_name(uint32_t id, std::string *pstr_guild_name) -> bool;
        auto guild_id() -> uint32_t;
        auto has_guild_land() -> bool;

        void start_guild_war(uint32_t enemy_guild_id);
        void end_guild_war(uint32_t enemy_guild_id);
        auto enemy_guild_id(uint32_t index) -> uint32_t;
        auto is_doing_guild_war() -> bool;

      protected:
        void calculate_level_average();
        void sort_member();
        auto is_grade_data(uint8_t grade_number) -> bool;

        void initialize_impl();

      protected:
        guild_info guild_info_value_;
        grade_data_map grade_data_map_value_;
        guild_member_data_vector guild_member_data_vector_value_;
        guild_board_comment_data_vector guild_board_comment_vector_;
        guild_skill_data guild_skill_data_value_;
        guild_name_map guild_name_map_value_;
        uint32_t enemy_guild_id_[ENEMY_GUILD_SLOT_MAX_COUNT];

        uint32_t member_level_summary_;
        uint32_t member_level_average_;
        uint32_t member_experience_summary_;

        bool guild_enable_;
    };

    inline auto get_python_guild() -> python_guild & {
        static python_guild instance;
        return instance;
    }
}
