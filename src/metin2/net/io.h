#pragma once

#include <asio/io_context.hpp>

namespace metin2::inline net {

    // The one io_context of the client. It is polled on the main thread by network_stream::process(), so
    // completion handlers never run concurrently with the game.
    auto get_io() -> asio::io_context &;

    // Runs the handlers that are ready without blocking.
    void poll_io();
}
