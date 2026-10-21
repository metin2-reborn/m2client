#pragma once

#include <memory>

namespace metin2::inline window {

    // A mouse cursor image loaded from one of the game's .cur files.
    class cursor {
      public:
        // The id is the cursor's resource id from the Windows client (client/app/resource_ids.h).
        static auto load(int resource_id) -> std::unique_ptr<cursor>;

        ~cursor();
        cursor(const cursor &) = delete;
        auto operator=(const cursor &) -> cursor & = delete;

        auto handle() const -> void * { return handle_; }

      private:
        explicit cursor(void *handle) : handle_(handle) {}

        void *handle_;
    };

    // nullptr selects the system arrow.
    void set_cursor(const cursor *c);
    void show_cursor(bool show);
}
