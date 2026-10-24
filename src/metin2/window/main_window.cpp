#include "main_window.h"

#include <metin2/dev/automation.h>
#include <metin2/input/keymap.h>
#include <metin2/input/keys.h>

#include <raylib.h>
// raylib keeps its GLFW window; the pointer lock and the custom cursors need GLFW calls.
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace metin2::inline window {

    namespace {
        bool created = false;
        bool shown = false;
        bool was_focused = false;
        bool was_minimized = false;
        uint64_t frames = 0;
        double last_collect = -1.0;

        std::optional<point> mouse_override;

        // Wayland does not let a client warp a free pointer. While the camera drag keeps pulling it
        // back, the pointer is locked instead, and the warp is emulated as an offset between the real
        // pointer and the position the game sees.
        bool pointer_locked = false;
        point warp;
        point lock_position;

        // Held keys repeat their key_event (not their character; raylib repeats those) for the edit
        // line navigation.
        struct held_key {
            int key;
            int vk;
            double next_repeat;
        };
        std::vector<held_key> held_keys;

        constexpr int VK_ALT = 0x12;

        // raylib's GetWindowHandle() is the native X11 or Wayland handle; its GLFW window is the one
        // that owns the current OpenGL context.
        auto glfw_window() -> GLFWwindow * {
            return glfwGetCurrentContext();
        }

        auto real_mouse_position() -> point {
            const Vector2 position = GetMousePosition();
            return {static_cast<int>(position.x), static_cast<int>(position.y)};
        }

        auto any_mouse_button_down() -> bool {
            return IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        }

        void lock_pointer(bool lock) {
            // glfwSetInputMode directly: raylib's DisableCursor/EnableCursor recenter the pointer, which
            // the game would read as a large drag.
            glfwSetInputMode(glfw_window(), GLFW_CURSOR, lock ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        }

        void filter_raylib_log(int level, const char *text, va_list args) {
            char buffer[1024];
            vsnprintf(buffer, sizeof(buffer), text, args);
            // Wayland cannot report or set window positions; GLFW complains (65548) on every query.
            if (level <= LOG_WARNING && strstr(buffer, "65548"))
                return;
            static const char *names[] = {"", "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};
            fprintf(stderr, "[raylib %s] %s\n", level >= 0 && level <= 6 ? names[level] : "?", buffer);
        }
    }

    main_window::main_window() = default;

    main_window::~main_window() = default;

    auto main_window::create(const char *title, int width, int height, bool vsync, bool borderless) -> bool {
        if (created)
            return true;

        SetTraceLogCallback(filter_raylib_log);
        SetTraceLogLevel(LOG_WARNING);
        unsigned flags = FLAG_WINDOW_HIDDEN | (borderless ? FLAG_WINDOW_UNDECORATED : FLAG_WINDOW_RESIZABLE);
        if (vsync)
            flags |= FLAG_VSYNC_HINT;
        SetConfigFlags(flags);
        InitWindow(width > 0 ? width : 800, height > 0 ? height : 600, title);
        if (!IsWindowReady())
            return false;

        SetExitKey(KEY_NULL);
        SetTargetFPS(0);
        created = true;
        dev::init();
        return true;
    }

    void main_window::destroy() {
        if (!created)
            return;
        CloseWindow();
        created = false;
        shown = false;
        is_visible_ = false;
    }

    void main_window::show() {
        ClearWindowState(FLAG_WINDOW_HIDDEN);
        is_visible_ = true;
        if (!shown) {
            shown = true;
            push_event(resize_event{false});
        }
    }

    void main_window::set_position(int x, int y) {
        SetWindowPosition(x, y);
    }

    void main_window::set_size(int width, int height) {
        if (width > 0 && height > 0)
            SetWindowSize(width, height);
    }

    auto main_window::size() const -> point {
        return {GetScreenWidth(), GetScreenHeight()};
    }

    void resize_window(point size) {
        if (IsWindowMaximized())
            RestoreWindow();
        if (size.x > 0 && size.y > 0)
            SetWindowSize(size.x, size.y);
    }

    void maximize_window() {
        MaximizeWindow();
    }

    auto monitor_size() -> point {
        if (!created)
            return {1920, 1080};
        const int monitor = GetCurrentMonitor();
        return {GetMonitorWidth(monitor), GetMonitorHeight(monitor)};
    }

    auto display_modes() -> std::vector<display_mode> {
        static constexpr point common[] = {
            {640,  480 },
            {800,  600 },
            {1024, 768 },
            {1152, 864 },
            {1280, 720 },
            {1280, 768 },
            {1280, 800 },
            {1280, 960 },
            {1280, 1024},
            {1360, 768 },
            {1366, 768 },
            {1440, 900 },
            {1600, 900 },
            {1600, 1200},
            {1680, 1050},
            {1920, 1080},
            {1920, 1200},
            {2560, 1080},
            {2560, 1440},
            {2560, 1600},
            {3440, 1440},
            {3840, 2160},
        };
        std::vector<point> sizes(std::begin(common), std::end(common));
        const point monitor = monitor_size();
        if (monitor.x > 0 && monitor.y > 0 && std::ranges::find_if(sizes, [&](const point &p) { return p.x == monitor.x && p.y == monitor.y; }) == sizes.end())
            sizes.push_back(monitor);
        std::ranges::sort(sizes, [](const point &a, const point &b) { return a.x != b.x ? a.x < b.x : a.y < b.y; });
        std::vector<display_mode> modes;
        for (const point &p : sizes)
            for (uint32_t bits : {32u, 16u})
                modes.push_back({static_cast<uint32_t>(p.x), static_cast<uint32_t>(p.y), 60, bits});
        return modes;
    }

    auto mouse_position() -> point {
        if (mouse_override)
            return *mouse_override;
        if (!created)
            return {};
        const point real = real_mouse_position();
        return {real.x + warp.x, real.y + warp.y};
    }

    void set_mouse_position(point position) {
        if (!created)
            return;

        if (!pointer_locked && glfwGetPlatform() == GLFW_PLATFORM_WAYLAND && any_mouse_button_down()) {
            lock_position = real_mouse_position();
            lock_pointer(true);
            pointer_locked = true;
        }

        if (pointer_locked) {
            const point real = real_mouse_position();
            warp = {position.x - real.x, position.y - real.y};
        } else {
            SetMousePosition(position.x, position.y);
        }
    }

    void set_mouse_override(std::optional<point> position) {
        mouse_override = position;
    }

    auto clipboard_text() -> std::string {
        const char *text = created ? GetClipboardText() : nullptr;
        return text ? text : "";
    }

    void set_clipboard_text(std::string_view text) {
        if (created)
            SetClipboardText(std::string(text).c_str());
    }

    auto frame_count() -> uint64_t {
        return frames;
    }

    void on_frame_presented() {
        ++frames;
        collect_events();
    }

    void collect_events() {
        if (!created)
            return;

        // The loop collects right after a present has; once is enough.
        const double now = GetTime();
        if (last_collect >= 0.0 && now - last_collect < 0.001)
            return;
        last_collect = now;

        PollInputEvents();
        dev::tick();

        if (WindowShouldClose()) {
            push_event(close_event{});
            // raylib copies GLFW's flag on every poll; clear it so one click closes once.
            glfwSetWindowShouldClose(glfw_window(), GLFW_FALSE);
        }

        const bool focused = IsWindowFocused();
        if (focused != was_focused) {
            was_focused = focused;
            push_event(activate_event{focused});
        }

        const bool minimized = IsWindowMinimized();
        if (minimized != was_minimized) {
            was_minimized = minimized;
            push_event(resize_event{minimized});
        } else if (IsWindowResized()) {
            push_event(resize_event{false});
        }

        static const struct {
            int raylib_button;
            mouse_button button;
        } buttons[] = {
            {MOUSE_BUTTON_LEFT,   mouse_button::left  },
            {MOUSE_BUTTON_RIGHT,  mouse_button::right },
            {MOUSE_BUTTON_MIDDLE, mouse_button::middle},
        };
        for (const auto &b : buttons) {
            if (IsMouseButtonPressed(b.raylib_button)) {
                mouse_override.reset(); // a real click takes the pointer back from the automation harness
                push_event(mouse_button_event{b.button, true, mouse_position()});
            }
            if (IsMouseButtonReleased(b.raylib_button))
                push_event(mouse_button_event{b.button, false, mouse_position()});
        }

        if (pointer_locked && (!focused || !any_mouse_button_down())) {
            // The pointer reappears where the drag started, as on Windows.
            lock_pointer(false);
            pointer_locked = false;
            warp = {};
            SetMousePosition(lock_position.x, lock_position.y);
        }

        const float wheel = GetMouseWheelMoveV().y;
        if (wheel != 0.0f)
            push_event(mouse_wheel_event{static_cast<int>(wheel * 120)});

        while (const int key = GetKeyPressed()) {
            const int vk = raylib_to_vk(key);
            if (vk == 0 || vk == VK_ALT)
                continue;
            push_key(vk, true);
            held_keys.push_back({key, vk, now + 0.4});
        }
        for (auto it = held_keys.begin(); it != held_keys.end();) {
            if (!IsKeyDown(it->key)) {
                push_key(it->vk, false);
                it = held_keys.erase(it);
                continue;
            }
            if (now >= it->next_repeat) {
                // Printable keys repeat through their characters instead; Enter, Tab and Escape act once.
                const int vk = it->vk;
                const bool editing_key = vk < 0x30 && vk != VK_RETURN && vk != VK_TAB && vk != VK_ESCAPE;
                if (editing_key || (vk >= 0x5B && vk < 0x60) || vk >= 0x70)
                    push_key(vk, true);
                it->next_repeat = now + 0.033;
            }
            ++it;
        }

        while (const int codepoint = GetCharPressed())
            push_event(char_event{static_cast<char32_t>(codepoint)});
    }
}
