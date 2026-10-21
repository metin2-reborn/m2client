#include "events.h"

#include <deque>

namespace metin2::inline window {

    namespace {
        std::deque<event> queue;
        bool quit = false;
    }

    void push_event(const event &e) {
        queue.push_back(e);
    }

    auto pop_event() -> std::optional<event> {
        if (queue.empty())
            return std::nullopt;
        event e = queue.front();
        queue.pop_front();
        return e;
    }

    void push_key(int vk, bool down) {
        push_event(key_event{vk, down});
        if (down && (vk == 0x08 || vk == 0x09 || vk == 0x0D || vk == 0x1B))
            push_event(char_event{static_cast<char32_t>(vk)});
    }

    void request_quit() {
        quit = true;
    }

    auto quit_requested() -> bool {
        return quit;
    }
}
