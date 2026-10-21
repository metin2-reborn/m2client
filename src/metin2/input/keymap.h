#pragma once

#include <string_view>

namespace metin2::inline input {

    // Translation between the game's DirectInput scan codes (input/keys.h), raylib key codes and the
    // Windows virtual-key codes that the Python edit lines expect (app.VK_*). Each returns 0 for a key
    // it does not know.
    auto dik_to_raylib(int dik) -> int;
    auto dik_to_vk(int dik) -> int;
    auto raylib_to_vk(int key) -> int;

    // "DIK_RETURN", "RETURN", "ENTER", "0x1C" or "28" -> DIK_RETURN.
    auto dik_from_name(std::string_view name) -> int;
}
