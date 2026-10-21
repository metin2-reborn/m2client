#pragma once

#include <cstdint>
#include <optional>
#include <variant>

namespace metin2::inline window {

    struct point {
        int x = 0;
        int y = 0;
    };

    enum class mouse_button {
        left,
        right,
        middle,
    };

    // The user asked to close the window (title bar button, Alt+F4).
    struct close_event {};

    // The window gained or lost the focus.
    struct activate_event {
        bool active;
    };

    // The window was shown, resized, minimized or restored.
    struct resize_event {
        bool minimized;
    };

    // Position in window coordinates at the time of the press or release.
    struct mouse_button_event {
        mouse_button button;
        bool down;
        point position;
    };

    // 120 per wheel notch, as on Windows.
    struct mouse_wheel_event {
        int delta;
    };

    // A key press (or auto-repeat) for text editing, as a Windows virtual-key code.
    struct key_event {
        int vk;
        bool down;
    };

    // A typed character. Backspace, Tab, Enter and Escape also arrive as characters (8, 9, 13, 27),
    // which is what the edit lines act on.
    struct char_event {
        char32_t codepoint;
    };

    using event = std::variant<close_event, activate_event, resize_event, mouse_button_event, mouse_wheel_event, key_event, char_event>;

    void push_event(const event &e);
    auto pop_event() -> std::optional<event>;

    // Queues the key event, plus the character Windows' TranslateMessage would add for it.
    void push_key(int vk, bool down);

    // Ends application::run() after the events already queued.
    void request_quit();
    auto quit_requested() -> bool;

    // Frames presented so far (the automation harness counts frames with it).
    auto frame_count() -> uint64_t;
}
