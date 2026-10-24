#pragma once

#include <cstdint>

namespace metin2::inline sound {

    class sound_data;

    // One OpenAL source. A plain source plays at the listener; a positional one is placed in the
    // world (positions as the sound manager scales them) and fades with distance.
    class sound_source {
      public:
        sound_source() = default;
        ~sound_source();
        sound_source(const sound_source &) = delete;
        auto operator=(const sound_source &) -> sound_source & = delete;

        void create(bool positional);
        void destroy();

        auto set_sound(sound_data &data) -> bool;
        void play(bool loop);
        void stop();
        void set_volume(float volume);
        // World position in the manager's scaled units; the world's z axis points the other way in
        // OpenAL.
        void set_position(float x, float y, float z);
        // Full volume up to reference_distance, then inverse-distance falloff until max_distance,
        // in the same units as set_position.
        void set_distances(float reference_distance, float max_distance);

        // A source that is not playing can take a new sound.
        auto is_done() const -> bool;
        auto is_looping() const -> bool { return looping_; }

      private:
        uint32_t source_ = 0; // ALuint
        bool looping_ = false;
    };
}
