#pragma once

#include <string>
#include <string_view>

// Everything the client writes (settings, logs, guild marks, screenshots) lives in ~/.metin2; the game
// directory is never written to.
namespace metin2::state {
    // ~/.metin2 (./.metin2 when HOME is not set), created on first use.
    auto directory() -> const std::string &;

    // directory()/name; a subdirectory named by it is created when create_directory is set.
    auto path(std::string_view name, bool create_directory = false) -> std::string;

    // mkdir -p
    void make_directories(const std::string &path);
}
