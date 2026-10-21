#pragma once

#include <memory>
#include <string>

namespace metin2::inline sound {

    // A music file decoded while it plays, in a loop, through a few queued OpenAL buffers.
    // Not thread-safe: the sound manager calls it under its music lock, from the game and from
    // the thread that keeps the buffers filled.
    class music_stream {
      public:
        music_stream();
        ~music_stream();
        music_stream(const music_stream &) = delete;
        auto operator=(const music_stream &) -> music_stream & = delete;

        // Replaces whatever the stream was playing.
        auto open(const char *file_name) -> bool;
        void close();
        auto is_open() const -> bool;

        void play();
        void set_volume(float volume);

        // Requeues the buffers that finished playing; restarts the source after an underrun.
        void refill();

        auto status() const -> std::string;

      private:
        struct state;
        std::unique_ptr<state> state_;
    };
}
