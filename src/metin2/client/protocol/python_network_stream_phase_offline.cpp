#include <metin2/client/app/python_application.h>
#include "packet.h"
#include "python_network_stream.h"

namespace metin2::inline client::inline protocol {

    void python_network_stream::off_line_phase() {
        packet_header header;

        if (!check_packet(&header))
            return;

        switch (header) {
        case HEADER_GC_PHASE:
            if (recv_phase_packet())
                return;
            break;
        }

        recv_error_packet(header);
    }
}
