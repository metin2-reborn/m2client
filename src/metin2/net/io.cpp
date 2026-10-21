#include "io.h"

namespace metin2::inline net {

    auto get_io() -> asio::io_context & {
        static asio::io_context context(1);
        return context;
    }

    void poll_io() {
        asio::io_context &context = get_io();
        // poll() stops the context once it runs out of work.
        if (context.stopped())
            context.restart();
        context.poll();
    }
}
