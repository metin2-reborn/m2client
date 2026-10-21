#include "check.h"

#include <metin2/core/cipher.h>
#include <metin2/net/io.h>
#include <metin2/net/net_stream.h>

#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

#include <chrono>
#include <functional>
#include <thread>
#include <vector>

using namespace metin2;
using asio::ip::tcp;

namespace {
    class test_stream : public network_stream {
      public:
        test_stream() {
            set_recv_buffer_size(4096);
            set_send_buffer_size(4096);
        }

        int connected = 0;
        int failed = 0;
        int disconnected = 0;

        using network_stream::activate;
        using network_stream::activate_cipher;
        using network_stream::prepare;

      protected:
        void on_connect_success() override { ++connected; }
        void on_connect_failure() override { ++failed; }
        void on_remote_disconnect() override { ++disconnected; }
    };

    // Pumps the stream until the condition holds or the time runs out.
    auto run_until(test_stream &stream, const std::function<bool()> &done, std::chrono::milliseconds limit = std::chrono::seconds(5)) -> bool {
        const auto deadline = std::chrono::steady_clock::now() + limit;
        while (!done()) {
            if (std::chrono::steady_clock::now() > deadline)
                return false;
            stream.process();
            poll_io();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
    }

    struct server {
        tcp::acceptor acceptor{get_io(), tcp::endpoint(asio::ip::address_v4::loopback(), 0)};
        tcp::socket peer{get_io()};
        bool accepted = false;

        server() {
            acceptor.async_accept(peer, [this](const asio::error_code &ec) { accepted = !ec; });
        }

        // The aborted accept handler refers to this object, so it has to run before the object goes.
        ~server() {
            acceptor.close();
            poll_io();
        }

        auto port() const -> int { return acceptor.local_endpoint().port(); }
    };

    auto read_exactly(tcp::socket &socket, size_t size) -> std::vector<char> {
        std::vector<char> data(size);
        socket.non_blocking(false);
        asio::read(socket, asio::buffer(data));
        return data;
    }
}

TEST(stream_connects_and_exchanges_data_in_order) {
    server srv;
    test_stream stream;
    CHECK(stream.connect("127.0.0.1", srv.port()));
    CHECK(run_until(stream, [&] { return stream.connected == 1 && srv.accepted; }));
    CHECK(stream.is_online());

    std::vector<char> message(3000);
    for (size_t i = 0; i < message.size(); ++i)
        message[i] = static_cast<char>(i * 7);
    CHECK(stream.send(1000, message.data()));
    CHECK(stream.send(2000, message.data() + 1000));
    stream.process();
    CHECK(read_exactly(srv.peer, message.size()) == message);

    asio::write(srv.peer, asio::buffer("hello", 5));
    CHECK(run_until(stream, [&] { return stream.recv_buffer_size() == 5; }));
    char reply[5];
    CHECK(stream.recv(5, reply));
    CHECK(std::string_view(reply, 5) == "hello");
    CHECK_EQ(stream.failed, 0);
}

TEST(stream_resolves_host_names) {
    server srv;
    test_stream stream;
    CHECK(stream.connect("localhost", srv.port()));
    CHECK(run_until(stream, [&] { return stream.connected == 1 || stream.failed == 1; }));
    CHECK_EQ(stream.connected, 1);
}

TEST(stream_reports_a_refused_connection) {
    int port = 0;
    {
        server srv;
        port = srv.port();
    }
    test_stream stream;
    CHECK(stream.connect("127.0.0.1", port));
    CHECK(run_until(stream, [&] { return stream.failed == 1; }));
    CHECK_EQ(stream.connected, 0);
    CHECK(!stream.is_online());
}

TEST(stream_gives_up_after_the_time_limit) {
    // An address that is not routed: the connect either hangs until the limit or fails at once.
    test_stream stream;
    CHECK(stream.connect("10.255.255.1", 9, 1));
    CHECK(run_until(stream, [&] { return stream.failed == 1; }, std::chrono::seconds(3)));
    CHECK_EQ(stream.connected, 0);
}

TEST(stream_reports_the_peer_closing) {
    server srv;
    test_stream stream;
    CHECK(stream.connect("127.0.0.1", srv.port()));
    CHECK(run_until(stream, [&] { return stream.connected == 1 && srv.accepted; }));
    srv.peer.close();
    CHECK(run_until(stream, [&] { return stream.disconnected == 1; }));
    CHECK(!stream.is_online());
}

TEST(stream_encrypts_each_byte_once_and_decrypts_buffered_input) {
    server srv;
    test_stream stream;
    CHECK(stream.connect("127.0.0.1", srv.port()));
    CHECK(run_until(stream, [&] { return stream.connected == 1 && srv.accepted; }));

    cipher peer;
    unsigned char client_key[256];
    unsigned char server_key[256];
    size_t client_length = sizeof(client_key);
    size_t server_length = sizeof(server_key);
    const size_t server_agreed = peer.prepare(server_key, &server_length);
    const size_t client_agreed = stream.prepare(client_key, &client_length);
    CHECK(stream.activate(server_agreed, server_key, server_length));
    CHECK(peer.activate(false, client_agreed, client_key, client_length));
    peer.set_activated(true);

    // Encrypted bytes that arrive before the stream turns its cipher on are decrypted retroactively.
    std::vector<char> early(64, 'e');
    std::vector<char> wire = early;
    peer.encrypt(wire.data(), wire.size());
    asio::write(srv.peer, asio::buffer(wire));
    CHECK(run_until(stream, [&] { return stream.recv_buffer_size() == 64; }));
    stream.activate_cipher();
    std::vector<char> got(64);
    CHECK(stream.recv(64, got.data()));
    CHECK(got == early);

    // Several sends between pumps are encrypted exactly once.
    std::vector<char> message(1500);
    for (size_t i = 0; i < message.size(); ++i)
        message[i] = static_cast<char>(i);
    CHECK(stream.send(500, message.data()));
    stream.process();
    CHECK(stream.send(1000, message.data() + 500));
    stream.process();
    std::vector<char> received = read_exactly(srv.peer, message.size());
    peer.decrypt(received.data(), received.size());
    CHECK(received == message);
}
