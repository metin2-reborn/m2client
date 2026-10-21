#include "application.h"

#include <type_traits>

namespace metin2::inline window {

    application::application() = default;

    application::~application() = default;

    void application::run() {
        while (!quit_requested()) {
            collect_events();
            while (!quit_requested()) {
                std::optional<event> e = pop_event();
                if (!e)
                    break;
                dispatch(*e);
            }
            if (quit_requested() || !process())
                break;
        }
    }

    void application::on_close_request() {
        request_quit();
    }

    void application::dispatch(const event &e) {
        std::visit(
            [this](const auto &ev) {
                using type = std::decay_t<decltype(ev)>;
                if constexpr (std::is_same_v<type, close_event>) {
                    on_close_request();
                } else if constexpr (std::is_same_v<type, activate_event>) {
                    is_active_ = ev.active;
                    on_activate(ev.active);
                } else if constexpr (std::is_same_v<type, resize_event>) {
                    is_active_ = !ev.minimized;
                    is_visible_ = !ev.minimized;
                    on_resize(ev.minimized);
                } else if constexpr (std::is_same_v<type, mouse_button_event>) {
                    on_mouse_button(ev.button, ev.down, ev.position);
                } else if constexpr (std::is_same_v<type, mouse_wheel_event>) {
                    on_mouse_wheel(ev.delta);
                } else if constexpr (std::is_same_v<type, key_event>) {
                    if (ev.down)
                        on_ime_key_down(ev.vk);
                } else if constexpr (std::is_same_v<type, char_event>) {
                    on_char(ev.codepoint);
                }
            },
            e);
    }
}
