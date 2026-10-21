#include "sound_data.h"

#include <metin2/core/mapped_file.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>

#include <AL/al.h>

#include <cstring>

#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include <dr_wav.h>
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_STDIO
#include <minimp3_ex.h>

namespace metin2::inline sound {

    namespace {
        auto is_wav(const uint8_t *data, size_t size) -> bool {
            return size >= 12 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WAVE", 4) == 0;
        }

        auto is_mp3(const uint8_t *data, size_t size) -> bool {
            if (size >= 3 && data[0] == 'I' && data[1] == 'D' && data[2] == '3')
                return true;
            return size >= 2 && data[0] == 0xFF && (data[1] & 0xE0) == 0xE0;
        }
    }

    auto decode_sound(const uint8_t *data, size_t size, pcm_sound &pcm) -> bool {
        if (is_wav(data, size)) {
            drwav wav;
            if (!drwav_init_memory(&wav, data, size, nullptr))
                return false;
            pcm.channels = static_cast<int>(wav.channels);
            pcm.rate = static_cast<int>(wav.sampleRate);
            pcm.samples.resize(static_cast<size_t>(wav.totalPCMFrameCount) * wav.channels);
            const drwav_uint64 frames = drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, pcm.samples.data());
            pcm.samples.resize(static_cast<size_t>(frames) * wav.channels);
            drwav_uninit(&wav);
            return frames > 0;
        }

        if (is_mp3(data, size)) {
            mp3dec_ex_t decoder;
            if (mp3dec_ex_open_buf(&decoder, data, size, MP3D_SEEK_TO_SAMPLE) != 0)
                return false;
            pcm.channels = decoder.info.channels;
            pcm.rate = decoder.info.hz;
            pcm.samples.resize(static_cast<size_t>(decoder.samples));
            pcm.samples.resize(mp3dec_ex_read(&decoder, pcm.samples.data(), decoder.samples));
            mp3dec_ex_close(&decoder);
            return !pcm.samples.empty();
        }

        return false;
    }

    auto read_sound_file(const char *file_name, std::vector<uint8_t> &data) -> bool {
        mapped_file file;
        const void *bytes = nullptr;
        if (!get_file_manager().get(file, file_name, &bytes) || !bytes || file.size() == 0)
            return false;
        const auto *begin = static_cast<const uint8_t *>(bytes);
        data.assign(begin, begin + file.size());
        return true;
    }

    sound_data::sound_data(std::string file_name) : file_name_(std::move(file_name)) {
    }

    sound_data::~sound_data() {
        if (buffer_)
            alDeleteBuffers(1, &buffer_);
    }

    auto sound_data::buffer() -> uint32_t {
        if (loaded_)
            return buffer_;
        loaded_ = true;

        std::vector<uint8_t> file;
        if (!read_sound_file(file_name_.c_str(), file)) {
            metin2::log::error("Cannot read sound {}", file_name_);
            return 0;
        }

        pcm_sound pcm;
        if (!decode_sound(file.data(), file.size(), pcm) || pcm.channels < 1 || pcm.channels > 2) {
            metin2::log::error("Cannot decode sound {}", file_name_);
            return 0;
        }

        alGenBuffers(1, &buffer_);
        alBufferData(buffer_, pcm.channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, pcm.samples.data(), static_cast<ALsizei>(pcm.samples.size() * sizeof(int16_t)), pcm.rate);
        return buffer_;
    }
}
