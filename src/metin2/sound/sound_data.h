#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace metin2::inline sound {

    // Interleaved 16-bit samples.
    struct pcm_sound {
        int channels = 0;
        int rate = 0;
        std::vector<int16_t> samples;
    };

    // Decodes a WAV (PCM or ADPCM) or MP3 file in memory.
    auto decode_sound(const uint8_t *data, size_t size, pcm_sound &pcm) -> bool;

    // Reads a file of the game data (archives or loose files) into memory.
    auto read_sound_file(const char *file_name, std::vector<uint8_t> &data) -> bool;

    // A sound file decoded once into an OpenAL buffer that every source playing it shares.
    class sound_data {
      public:
        explicit sound_data(std::string file_name);
        ~sound_data();
        sound_data(const sound_data &) = delete;
        auto operator=(const sound_data &) -> sound_data & = delete;

        // The buffer, loaded on first use; 0 when the file cannot be read or decoded.
        auto buffer() -> uint32_t;
        auto file_name() const -> const std::string & { return file_name_; }

      private:
        std::string file_name_;
        uint32_t buffer_ = 0; // ALuint
        bool loaded_ = false;
    };
}
