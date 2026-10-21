#include "music_stream.h"
#include "sound_data.h"

#include <AL/al.h>
#include <dr_wav.h>
#include <minimp3_ex.h>

#include <algorithm>
#include <cstdio>
#include <vector>

namespace metin2::inline sound {

    namespace {
        constexpr int BUFFER_COUNT = 4;
        constexpr size_t BUFFER_FRAMES = 4096;
    }

    struct music_stream::state {
        std::string name;
        std::vector<uint8_t> file; // the decoders read from it
        bool mp3 = false;
        mp3dec_ex_t mp3_decoder{};
        drwav wav_decoder{};
        int channels = 0;
        int rate = 0;

        ALuint source = 0;
        ALuint buffers[BUFFER_COUNT] = {};
        bool playing = false;
        float gain = 1.0f;
        uint64_t frames_played = 0;
        std::vector<int16_t> pcm;

        ~state() {
            if (source) {
                alSourceStop(source);
                alSourcei(source, AL_BUFFER, 0);
                alDeleteSources(1, &source);
                alDeleteBuffers(BUFFER_COUNT, buffers);
            }
            if (mp3)
                mp3dec_ex_close(&mp3_decoder);
            else if (channels)
                drwav_uninit(&wav_decoder);
        }

        void rewind() {
            if (mp3)
                mp3dec_ex_seek(&mp3_decoder, 0);
            else
                drwav_seek_to_pcm_frame(&wav_decoder, 0);
        }

        // Decodes the next frames, starting over at the end of the file.
        auto decode(int16_t *out, size_t max_frames) -> size_t {
            size_t frames = 0;
            bool rewound = false;
            while (frames < max_frames) {
                size_t got;
                if (mp3)
                    got = mp3dec_ex_read(&mp3_decoder, out + frames * channels, (max_frames - frames) * channels) / static_cast<size_t>(channels);
                else
                    got = static_cast<size_t>(drwav_read_pcm_frames_s16(&wav_decoder, max_frames - frames, out + frames * channels));
                if (got == 0) {
                    if (rewound)
                        break; // nothing decodes at all
                    rewind();
                    rewound = true;
                    continue;
                }
                rewound = false;
                frames += got;
            }
            return frames;
        }

        auto fill(ALuint buffer) -> bool {
            pcm.resize(BUFFER_FRAMES * channels);
            const size_t frames = decode(pcm.data(), BUFFER_FRAMES);
            if (frames == 0)
                return false;
            alBufferData(buffer, channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, pcm.data(), static_cast<ALsizei>(frames * channels * sizeof(int16_t)), rate);
            return true;
        }
    };

    music_stream::music_stream() = default;

    music_stream::~music_stream() = default;

    auto music_stream::open(const char *file_name) -> bool {
        close();

        auto s = std::make_unique<state>();
        s->name = file_name;
        if (!read_sound_file(file_name, s->file))
            return false;

        if (s->file.size() >= 12 && std::equal(s->file.begin(), s->file.begin() + 4, "RIFF")) {
            if (!drwav_init_memory(&s->wav_decoder, s->file.data(), s->file.size(), nullptr))
                return false;
            s->channels = static_cast<int>(s->wav_decoder.channels);
            s->rate = static_cast<int>(s->wav_decoder.sampleRate);
        } else {
            if (mp3dec_ex_open_buf(&s->mp3_decoder, s->file.data(), s->file.size(), MP3D_SEEK_TO_SAMPLE) != 0)
                return false;
            s->mp3 = true;
            s->channels = s->mp3_decoder.info.channels;
            s->rate = s->mp3_decoder.info.hz;
        }
        if (s->channels < 1 || s->channels > 2)
            return false;

        alGenSources(1, &s->source);
        alGenBuffers(BUFFER_COUNT, s->buffers);
        alSourcei(s->source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(s->source, AL_POSITION, 0.0f, 0.0f, 0.0f);
        state_ = std::move(s);
        return true;
    }

    void music_stream::close() {
        state_.reset();
    }

    auto music_stream::is_open() const -> bool {
        return state_ != nullptr;
    }

    void music_stream::play() {
        if (!state_)
            return;
        state &s = *state_;
        alSourceStop(s.source);
        alSourcei(s.source, AL_BUFFER, 0);
        s.rewind();
        s.frames_played = 0;
        for (ALuint buffer : s.buffers) {
            if (!s.fill(buffer))
                break;
            alSourceQueueBuffers(s.source, 1, &buffer);
        }
        alSourcePlay(s.source);
        s.playing = true;
    }

    void music_stream::set_volume(float volume) {
        if (!state_)
            return;
        state_->gain = std::clamp(volume, 0.0f, 1.0f);
        alSourcef(state_->source, AL_GAIN, state_->gain);
    }

    void music_stream::refill() {
        if (!state_ || !state_->playing)
            return;
        state &s = *state_;
        ALint processed = 0;
        alGetSourcei(s.source, AL_BUFFERS_PROCESSED, &processed);
        while (processed-- > 0) {
            ALuint buffer = 0;
            alSourceUnqueueBuffers(s.source, 1, &buffer);
            ALint size = 0;
            alGetBufferi(buffer, AL_SIZE, &size);
            s.frames_played += static_cast<uint64_t>(size) / (s.channels * sizeof(int16_t));
            if (s.fill(buffer))
                alSourceQueueBuffers(s.source, 1, &buffer);
        }

        ALint source_state = 0;
        ALint queued = 0;
        alGetSourcei(s.source, AL_SOURCE_STATE, &source_state);
        alGetSourcei(s.source, AL_BUFFERS_QUEUED, &queued);
        if (source_state != AL_PLAYING && queued > 0)
            alSourcePlay(s.source);
    }

    auto music_stream::status() const -> std::string {
        if (!state_)
            return "closed";
        const state &s = *state_;
        ALint source_state = 0;
        alGetSourcei(s.source, AL_SOURCE_STATE, &source_state);
        char line[512];
        snprintf(line, sizeof(line), "%s %s pos=%.1fs gain=%.2f", s.name.c_str(), source_state == AL_PLAYING ? "playing" : "idle",
            s.rate ? static_cast<double>(s.frames_played) / s.rate : 0.0, s.gain);
        return line;
    }
}
