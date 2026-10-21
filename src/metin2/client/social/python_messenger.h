#pragma once
#include <map>
#include <metin2/script/python_header.h>
#include <set>
#include <string>

namespace metin2::inline client::inline social {

    class python_messenger {
      public:
        using friend_name_map = std::set<std::string>;
        using guild_member_state_map = std::map<std::string, uint8_t>;

        enum messenger_group_index {
            MESSENGER_GRUOP_INDEX_FRIEND,
            MESSENGER_GRUOP_INDEX_GUILD,
        };

      public:
        python_messenger();
        ~python_messenger();

        void destroy();

        void remove_friend(const char *key);
        void on_friend_login(const char *key);
        void on_friend_logout(const char *key);
        void set_mobile(const char *key, uint8_t state);
        auto is_friend_by_key(const char *key) -> bool;
        auto is_friend_by_name(const char *name) -> bool;

        void append_guild_member(const char *name);
        void remove_guild_member(const char *name);
        void remove_all_guild_member();
        void login_guild_member(const char *name);
        void logout_guild_member(const char *name);
        void refresh_guild_member();

        void set_messenger_handler(PyObject *handler);

      protected:
        friend_name_map friend_name_map_value_;
        guild_member_state_map guild_member_state_map_value_;

      private:
        PyObject *messenger_handler_;
    };

    inline auto get_python_messenger() -> python_messenger & {
        static python_messenger instance;
        return instance;
    }
}
