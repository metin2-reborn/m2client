#pragma once

#include <metin2/net/net_stream.h>
#include <metin2/script/python_header.h>

#include <list>
#include <string>

namespace metin2::inline client::inline protocol {

    class server_state_checker {
      public:
        server_state_checker();
        ~server_state_checker();

        void create(PyObject *wnd);
        void add_channel(uint32_t server_index, const char *addr, uint32_t port);
        void request();
        void update();

        void initialize();

      private:
        struct channel {
            uint32_t server_index;
            std::string addr;
            uint32_t port;
        };

        PyObject *wnd_;

        std::list<channel> channel_value_;

        network_stream stream_value_;
    };

    inline auto get_server_state_checker() -> server_state_checker & {
        static server_state_checker instance;
        return instance;
    }
}
