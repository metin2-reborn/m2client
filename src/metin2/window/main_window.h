#pragma once

#include "events.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace metin2::inline window {

    // The game window, created and driven through raylib. There is only one.
    class main_window {
      public:
        main_window();
        virtual ~main_window();

        // Creates the window hidden, together with the OpenGL context the graphics device renders to.
        // With vsync the buffer swap waits for the display.
        auto create(const char *title, int width, int height, bool vsync) -> bool;
        void destroy();

        // The first show reports the window size (resize_event), which ends the minimized state the
        // application starts in.
        void show();

        void set_position(int x, int y);
        void set_size(int width, int height);
        auto size() const -> point;

        auto is_active() const -> bool { return is_active_; }
        auto is_visible() const -> bool { return is_visible_; }

      protected:
        bool is_active_ = false;
        bool is_visible_ = false;
    };

    // Resizes or maximizes the window as the user would; the game sees a resize_event (automation).
    void resize_window(point size);
    void maximize_window();

    // Size of the monitor the window is on; 1920x1080 before the window exists.
    auto monitor_size() -> point;

    struct display_mode {
        uint32_t width;
        uint32_t height;
        uint32_t refresh_rate;
        uint32_t bits_per_pixel;
    };

    // Common resolutions plus the monitor's own, sorted, each offered at 32 and 16 bits per pixel
    // (they look the same; saved settings may name either).
    auto display_modes() -> std::vector<display_mode>;

    // Pointer position in window coordinates: the automation override, or the real pointer.
    auto mouse_position() -> point;
    // Moves the pointer. The camera drag pulls it back to where the drag started on every move.
    void set_mouse_position(point position);
    void set_mouse_override(std::optional<point> position);

    auto clipboard_text() -> std::string;
    void set_clipboard_text(std::string_view text);

    // Turns raylib's window and input state into queued events. Runs at the top of every loop and
    // after every presented frame; the automation harness takes its turn here too.
    void collect_events();
    void on_frame_presented();
}
