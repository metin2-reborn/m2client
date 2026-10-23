#include "server_state_checker.h"
#include "packet.h"

#include <metin2/net/net_address.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline protocol {

#pragma pack(push)
#pragma pack(1)

    using server_state_checker_header = unsigned char;
    using server_state_checker_key = uint32_t;
    using server_state_checker_index = uint32_t;
    using server_state_checker_state = unsigned char;

#pragma pack(pop)

    server_state_checker::server_state_checker() {
        initialize();
    }

    server_state_checker::~server_state_checker() {
        initialize();
        wnd_ = nullptr;
    }

    void server_state_checker::create(PyObject *wnd) {
        wnd_ = wnd;
    }

    void server_state_checker::add_channel(uint32_t server_index, const char *addr, uint32_t port) {
        channel c;
        c.server_index = server_index;
        c.addr = addr;
        c.port = port;

        channel_value_.push_back(c);
    }

    void server_state_checker::request() {
        if (channel_value_.empty()) {
            return;
        }

        if (!stream_value_.connect(channel_value_.begin()->addr.c_str(), channel_value_.begin()->port)) {
            for (auto it = channel_value_.begin(); it != channel_value_.end(); ++it) {
                py_call_class_member_func(wnd_, "notify_channel_state", Py_BuildValue("(ii)", it->server_index, 0));
            }
            return;
        }
        stream_value_.clear_recv_buffer();
        stream_value_.set_send_buffer_size(1024);
        stream_value_.set_recv_buffer_size(1024);

        uint8_t header = HEADER_CG_STATE_CHECKER;
        if (!stream_value_.send(sizeof(header), &header)) {
            for (auto it = channel_value_.begin(); it != channel_value_.end(); ++it) {
                py_call_class_member_func(wnd_, "notify_channel_state", Py_BuildValue("(ii)", it->server_index, 0));
            }
            initialize();
            return;
        }
    }

    void server_state_checker::update() {
        stream_value_.process();

        uint8_t header;
        if (!stream_value_.recv(sizeof(header), &header)) {
            return;
        }
        if (HEADER_GC_RESPOND_CHANNELSTATUS != header) {
            return;
        }
        int size;
        if (!stream_value_.recv(sizeof(size), &size)) {
            return;
        }
        for (int i = 0; i < size; i++) {
            channel_status channel_status;
            if (!stream_value_.recv(sizeof(channel_status), &channel_status)) {
                return;
            }
            for (auto it = channel_value_.begin(); it != channel_value_.end(); ++it) {
                if (uint16_t(channel_status.port) == it->port) {
                    py_call_class_member_func(wnd_, "notify_channel_state", Py_BuildValue("(ii)", it->server_index, channel_status.status));
                    break;
                }
            }
        }
        initialize();
    }

    void server_state_checker::initialize() {
        channel_value_.clear();
        stream_value_.disconnect();
    }
}
