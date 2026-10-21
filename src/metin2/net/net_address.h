#pragma once

#include <cstdint>
#include <string>

namespace metin2::inline net {

    // A host name or dotted IPv4 address and a port; resolved when a stream connects.
    class network_address {
      public:
        network_address() = default;
        network_address(std::string host, uint16_t port) : host_(std::move(host)), port_(port) {}

        void set(const char *host, uint16_t port) {
            host_ = host;
            port_ = port;
        }

        auto host() const -> const std::string & { return host_; }
        auto port() const -> uint16_t { return port_; }

      private:
        std::string host_;
        uint16_t port_ = 0;
    };
}
