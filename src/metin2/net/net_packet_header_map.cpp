#include "net_packet_header_map.h"

namespace metin2::inline net {

    void network_packet_header_map::set(int header, packet_type packet_type_value) {
        header_map_[header] = packet_type_value;
    }
    auto network_packet_header_map::get(int header, packet_type *packet_type_value) -> bool {
        auto f = header_map_.find(header);

        if (header_map_.end() == f)
            return false;

        *packet_type_value = f->second;

        return true;
    }

    network_packet_header_map::network_packet_header_map() = default;

    network_packet_header_map::~network_packet_header_map() = default;
}
