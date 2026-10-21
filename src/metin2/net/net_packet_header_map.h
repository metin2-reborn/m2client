#pragma once

#include <map>

namespace metin2::inline net {

    class network_packet_header_map {
      public:
        struct packet_type {
            packet_type(int size = 0, bool flag = false) {
                packet_size = size;
                is_dynamic_size_packet = flag;
            }

            int packet_size;
            bool is_dynamic_size_packet;
        };

      public:
        network_packet_header_map();
        virtual ~network_packet_header_map();

        void set(int header, packet_type packet_type_value);
        auto get(int header, packet_type *packet_type_value) -> bool;

      protected:
        std::map<int, packet_type> header_map_;
    };
}
