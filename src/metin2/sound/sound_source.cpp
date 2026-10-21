#include "sound_source.h"
#include "sound_data.h"

#include <AL/al.h>

#include <algorithm>

namespace metin2::inline sound {

    sound_source::~sound_source() {
        destroy();
    }

    void sound_source::create(bool positional) {
        if (source_)
            return;
        alGenSources(1, &source_);
        alSourcef(source_, AL_GAIN, 1.0f);
        if (positional) {
            alSourcei(source_, AL_SOURCE_RELATIVE, AL_FALSE);
            alSourcef(source_, AL_REFERENCE_DISTANCE, 300.0f);
            alSourcef(source_, AL_MAX_DISTANCE, 5000.0f);
            alSourcef(source_, AL_ROLLOFF_FACTOR, 1.0f);
            alSource3f(source_, AL_POSITION, 0.0f, 0.0f, 0.0f);
        } else {
            // Straight ahead of the listener: centred, as the Windows client's 2D sounds were.
            alSourcei(source_, AL_SOURCE_RELATIVE, AL_TRUE);
            alSource3f(source_, AL_POSITION, 0.0f, 0.0f, -1.0f);
        }
    }

    void sound_source::destroy() {
        if (!source_)
            return;
        alSourceStop(source_);
        alSourcei(source_, AL_BUFFER, 0);
        alDeleteSources(1, &source_);
        source_ = 0;
    }

    auto sound_source::set_sound(sound_data &data) -> bool {
        const uint32_t buffer = data.buffer();
        if (!source_ || !buffer)
            return false;
        alSourceStop(source_);
        alSourcei(source_, AL_BUFFER, static_cast<ALint>(buffer));
        return true;
    }

    void sound_source::play(bool loop) {
        if (!source_)
            return;
        alSourcei(source_, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        alSourceRewind(source_);
        alSourcePlay(source_);
    }

    void sound_source::stop() {
        if (source_)
            alSourceStop(source_);
    }

    void sound_source::set_volume(float volume) {
        if (source_)
            alSourcef(source_, AL_GAIN, std::clamp(volume, 0.0f, 1.0f));
    }

    void sound_source::set_position(float x, float y, float z) {
        if (source_)
            alSource3f(source_, AL_POSITION, x, y, -z);
    }

    auto sound_source::is_done() const -> bool {
        if (!source_)
            return false;
        ALint state = AL_STOPPED;
        alGetSourcei(source_, AL_SOURCE_STATE, &state);
        return state != AL_PLAYING;
    }
}
