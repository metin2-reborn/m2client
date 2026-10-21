#include "check.h"

#include <metin2/core/cipher.h>

#include <random>
#include <vector>

using namespace metin2;

namespace {
    auto handshake(cipher &client, cipher &server) -> bool {
        unsigned char client_key[256];
        unsigned char server_key[256];
        size_t client_length = sizeof(client_key);
        size_t server_length = sizeof(server_key);
        const size_t server_agreed = server.prepare(server_key, &server_length);
        const size_t client_agreed = client.prepare(client_key, &client_length);
        if (server_agreed != cipher::kKeySize || client_agreed != cipher::kKeySize)
            return false;
        if (!client.activate(true, server_agreed, server_key, server_length) || !server.activate(false, client_agreed, client_key, client_length))
            return false;
        client.set_activated(true);
        server.set_activated(true);
        return true;
    }
}

TEST(cipher_traffic_round_trips_in_odd_chunks) {
    std::mt19937 rng(7);
    for (int round = 0; round < 200; ++round) {
        cipher client;
        cipher server;
        CHECK(handshake(client, server));
        for (int i = 0; i < 20; ++i) {
            std::vector<unsigned char> message(rng() % 300);
            for (auto &byte : message)
                byte = static_cast<unsigned char>(rng());
            std::vector<unsigned char> data = message;
            cipher &from = i % 2 ? client : server;
            cipher &to = i % 2 ? server : client;

            const size_t split = data.empty() ? 0 : rng() % data.size();
            from.encrypt(data.data(), split);
            from.encrypt(data.data() + split, data.size() - split);
            if (message.size() > 16)
                CHECK(data != message);

            const size_t split2 = data.empty() ? 0 : rng() % data.size();
            to.decrypt(data.data(), split2);
            to.decrypt(data.data() + split2, data.size() - split2);
            CHECK(data == message);
        }
    }
}

TEST(cipher_refuses_low_order_peer_key) {
    cipher cipher;
    unsigned char key[256];
    size_t length = sizeof(key);
    cipher.prepare(key, &length);
    const unsigned char zero[32] = {};
    CHECK(!cipher.activate(true, 32, zero, 32));
}
