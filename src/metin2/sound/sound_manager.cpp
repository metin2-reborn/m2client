#include "sound_manager.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <format>
#include <limits>

#include <AL/al.h>
#include <AL/alc.h>

#include <metin2/core/crc32.h>
#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/log/log.h>

namespace metin2::inline sound {

    namespace {
        // In game units: a sound is at full volume within the first, fades with the inverse of the
        // distance, and is not attenuated further beyond the second (character sounds are not even
        // started that far away).
        constexpr float FULL_VOLUME_DISTANCE = 500.0f;
        constexpr float AUDIBLE_DISTANCE = 5000.0f;
    }

    sound_manager::sound_manager() {
        is_sound_disable_ = false;

        fx_position_ = 0.0f;
        fy_position_ = 0.0f;
        fz_position_ = 0.0f;

        sound_scale_ = 200.0f;
        ambience_sound_scale_ = 1000.0f;
        sound_volume_ = 1.0f;
        music_volume_ = 1.0f;

        backup_music_volume_ = 0.0f;
        backup_sound_volume_ = 0.0f;

        for (int i = 0; i < MUSIC_COUNT; ++i) {
            music_instance &instance = music_instances_[i];
            instance.music_file_name_crc = 0;
            instance.music_state_value = MUSIC_STATE_OFF;
            instance.volume = 0.0f;
            instance.limit_volume = 0.0f;
            instance.volume_speed = 0.0f;
        }
    }

    sound_manager::~sound_manager() {
        destroy();
    }

    auto sound_manager::create() -> bool {
        open_device();
        return true;
    }

    void sound_manager::destroy() {
        close_device();
        play_sound_history_map_.clear();
    }

    void sound_manager::open_device() {
        if (device_)
            return;

        device_ = alcOpenDevice(nullptr);
        if (!device_) {
            metin2::log::warn("No audio device; sound is off");
            return;
        }

        const ALCint attributes[] = {ALC_FREQUENCY, 44100, 0};
        context_ = alcCreateContext(device_, attributes);
        if (!context_ || !alcMakeContextCurrent(context_)) {
            metin2::log::warn("Cannot create an audio context; sound is off");
            if (context_)
                alcDestroyContext(context_);
            alcCloseDevice(device_);
            context_ = nullptr;
            device_ = nullptr;
            return;
        }

        alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
        alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);

        for (sound_source &source : sources2d_)
            source.create(false);
        for (sound_source &source : sources3d_)
            source.create(true);
        apply_source_distances();

        music_thread_ = std::jthread([this](std::stop_token stop) {
            while (!stop.stop_requested()) {
                music_service();
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        });

        metin2::log::debug("Audio device: {}", alcGetString(device_, ALC_DEVICE_SPECIFIER));
    }

    void sound_manager::close_device() {
        if (!device_)
            return;

        music_thread_ = {};
        for (music_stream &music : music_)
            music.close();
        for (sound_source &source : sources2d_)
            source.destroy();
        for (sound_source &source : sources3d_)
            source.destroy();
        data_.clear();

        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context_);
        alcCloseDevice(device_);
        context_ = nullptr;
        device_ = nullptr;
    }

    void sound_manager::music_service() {
        std::lock_guard lock(music_mutex_);
        for (music_stream &music : music_)
            music.refill();
    }

    auto sound_manager::debug_status() -> std::string {
        if (!device_)
            return "device=none";

        int playing = 0;
        for (const sound_source &source : sources2d_)
            playing += source.is_done() ? 0 : 1;
        for (const sound_source &source : sources3d_)
            playing += source.is_done() ? 0 : 1;

        std::string status = std::format("device=open sounds={} playing={}", data_.size(), playing);
        std::lock_guard lock(music_mutex_);
        for (const music_stream &music : music_)
            if (music.is_open())
                status += " | stream " + music.status();
        return status;
    }

    auto sound_manager::data(const char *file_name) -> sound_data & {
        const uint32_t crc = get_crc32(file_name, strlen(file_name));
        std::unique_ptr<sound_data> &entry = data_[crc];
        if (!entry)
            entry = std::make_unique<sound_data>(file_name);
        return *entry;
    }

    auto sound_manager::free_source2d(const char *file_name) -> sound_source * {
        const uint32_t start = next2d_;
        next2d_ = (next2d_ + 1) % SOUND2D_COUNT;
        for (uint32_t i = 0; i < SOUND2D_COUNT; ++i) {
            sound_source &source = sources2d_[(start + i) % SOUND2D_COUNT];
            if (!source.is_done())
                continue;
            if (!source.set_sound(data(file_name)))
                return nullptr;
            return &source;
        }
        return nullptr;
    }

    auto sound_manager::free_source3d(const char *file_name) -> int {
        const uint32_t start = next3d_;
        next3d_ = (next3d_ + 1) % SOUND3D_COUNT;
        int index = -1;
        for (uint32_t i = 0; i < SOUND3D_COUNT && index < 0; ++i) {
            const int candidate = static_cast<int>((start + i) % SOUND3D_COUNT);
            if (sources3d_[candidate].is_done())
                index = candidate;
        }
        if (index < 0) {
            float oldest = std::numeric_limits<float>::max();
            for (int i = 0; i < SOUND3D_COUNT; ++i) {
                if (!sources3d_[i].is_looping() && started3d_[i] < oldest) {
                    oldest = started3d_[i];
                    index = i;
                }
            }
            if (index < 0)
                return -1;
        }
        if (!sources3d_[index].set_sound(data(file_name)))
            return -1;
        started3d_[index] = get_timer().current_second();
        return index;
    }

    auto sound_manager::source3d(int index) -> sound_source * {
        if (index < 0 || index >= SOUND3D_COUNT)
            return nullptr;
        return &sources3d_[index];
    }

    void sound_manager::apply_source_distances() {
        for (sound_source &source : sources3d_)
            source.set_distances(FULL_VOLUME_DISTANCE / sound_scale_, AUDIBLE_DISTANCE / sound_scale_);
    }

    // The listener moves through the world in OpenAL units (game units / sound_scale_), so a sound
    // stays where it was played while the character walks away from it.
    void sound_manager::set_position(float fx, float fy, float fz) {
        fx_position_ = fx;
        fy_position_ = fy;
        fz_position_ = fz;
        if (device_)
            alListener3f(AL_POSITION, fx / sound_scale_, fy / sound_scale_, -fz / sound_scale_);
    }

    // The world's z axis points the other way in OpenAL.
    void sound_manager::set_direction(float fx_dir, float fy_dir, float fz_dir, float fx_up, float fy_up, float fz_up) {
        if (!device_)
            return;
        const float orientation[6] = {fx_dir, fy_dir, -fz_dir, fx_up, fy_up, -fz_up};
        alListenerfv(AL_ORIENTATION, orientation);
    }

    void sound_manager::update() {
        // The fade speeds are volume steps per frame at 60 fps.
        const float fade_frames = get_timer().elapsed_second() * 60.0f;
        for (int i = 0; i < MUSIC_COUNT; ++i) {
            music_instance &music_instance_value = music_instances_[i];
            if (MUSIC_STATE_OFF == music_instance_value.music_state_value)
                continue;

            switch (music_instance_value.music_state_value) {
            case MUSIC_STATE_OFF:
            case MUSIC_STATE_PLAY:
                break;

            case MUSIC_STATE_FADE_IN:
                music_instance_value.volume += music_instance_value.volume_speed * fade_frames;

                if (music_instance_value.volume >= music_volume()) {
                    music_instance_value.volume = music_volume();
                    music_instance_value.volume_speed = 0.0f;
                    music_instance_value.music_state_value = MUSIC_STATE_PLAY;
                }

                {
                    std::lock_guard lock(music_mutex_);
                    music_[i].set_volume(music_instance_value.volume);
                }
                break;

            case MUSIC_STATE_FADE_LIMIT_OUT:
                music_instance_value.volume -= music_instance_value.volume_speed * fade_frames;
                if (music_instance_value.volume <= music_instance_value.limit_volume) {
                    music_instance_value.volume = music_instance_value.limit_volume;
                    music_instance_value.volume_speed = 0.0f;
                    music_instance_value.music_state_value = MUSIC_STATE_PLAY;
                }
                {
                    std::lock_guard lock(music_mutex_);
                    music_[i].set_volume(music_instance_value.volume);
                }
                break;
            case MUSIC_STATE_FADE_OUT:
                music_instance_value.volume -= music_instance_value.volume_speed * fade_frames;
                if (music_instance_value.volume <= 0.0f) {
                    music_instance_value.volume = 0.0f;
                    music_instance_value.volume_speed = 0.0f;
                    music_instance_value.music_state_value = MUSIC_STATE_OFF;

                    stop_music(i);
                }
                {
                    std::lock_guard lock(music_mutex_);
                    music_[i].set_volume(music_instance_value.volume);
                }
                break;
            }
        }
    }

    void sound_manager::update_sound_instance(float fx, float fy, float fz, uint32_t dwcur_frame, const sound_types::sound_instance_vector *sound_instance_vector, bool check_frequency) {
        for (size_t i = 0; i < sound_instance_vector->size(); ++i) {
            const sound_types::sound_instance_entry &sound_instance = sound_instance_vector->at(i);
            if (sound_instance.frame == dwcur_frame) {
                play_character_sound3d(fx, fy, fz, sound_instance.sound_file_name.c_str(), check_frequency);
            }
        }
    }

    void sound_manager::update_sound_instance(uint32_t dwcur_frame, const sound_types::sound_instance_vector *sound_instance_vector) {
        for (size_t i = 0; i < sound_instance_vector->size(); ++i) {
            const sound_types::sound_instance_entry &sound_instance = sound_instance_vector->at(i);

            if (sound_instance.frame == dwcur_frame) {
                play_sound2d(sound_instance.sound_file_name.c_str());
            }
        }
    }

    void sound_manager::set_sound_scale(float scale) {
        sound_scale_ = scale;
        apply_source_distances();
    }

    void sound_manager::set_ambience_sound_scale(float scale) {
        ambience_sound_scale_ = scale;
    }

    void sound_manager::set_sound_volume(float volume) {
        if (is_sound_disable_) {
            backup_sound_volume_ = volume;
            return;
        }

        volume = f_max(volume, 0.0f);
        volume = f_min(volume, 1.0f);
        sound_volume_ = volume;

        if (!is_sound_disable_) {
            backup_sound_volume_ = volume;
        }
    }

    void sound_manager::set_music_volume_impl(float volume) {
        if (is_sound_disable_) {
            backup_music_volume_ = volume;
            return;
        }

        volume = f_max(volume, 0.0f);
        volume = f_min(volume, 1.0f);
        music_volume_ = volume;

        if (!is_sound_disable_) {
            backup_music_volume_ = volume;
        }

        for (int i = 0; i < MUSIC_COUNT; ++i) {
            music_instance &music_instance_value = music_instances_[i];
            if (MUSIC_STATE_OFF == music_instance_value.music_state_value)
                continue;
            if (MUSIC_STATE_FADE_OUT == music_instance_value.music_state_value)
                continue;

            music_instance_value.volume = volume;

            std::lock_guard lock(music_mutex_);
            music_[i].set_volume(volume);
        }
    }

    auto sound_manager::convert_ratio_volume_to_apply_volume(float ratio_volume) -> float {
        if (0.1f > ratio_volume)
            return ratio_volume;

        return powf(10.0f, -1.0f + ratio_volume);
    }

    auto sound_manager::convert_grade_volume_to_apply_volume(int grade_volume) -> float {
        return convert_ratio_volume_to_apply_volume(grade_volume / 5.0f);
    }

    void sound_manager::set_sound_volume_grade(int grade) {
        float volume = convert_grade_volume_to_apply_volume(grade);
        set_sound_volume(volume);
    }

    void sound_manager::set_sound_volume_ratio(float ratio) {
        float volume = convert_ratio_volume_to_apply_volume(ratio);
        set_sound_volume(volume);
    }

    void sound_manager::set_music_volume(float volume) {
        set_music_volume_impl(volume);
    }

    auto sound_manager::sound_volume() -> float {
        return sound_volume_;
    }

    auto sound_manager::music_volume() -> float {
        return music_volume_;
    }

    void sound_manager::play_sound2d(const char *file_name) {
        if (!device_ || 0.0f == sound_volume())
            return;

        sound_source *source = free_source2d(file_name);
        if (!source)
            return;

        source->set_volume(sound_volume());
        source->play(false);
    }

    void sound_manager::play_sound3d(float fx, float fy, float fz, const char *file_name, int play_count) {
        if (!device_ || 0.0f == sound_volume())
            return;

        sound_source *source = source3d(free_source3d(file_name));
        if (!source)
            return;

        source->set_position(fx / sound_scale_, fy / sound_scale_, fz / sound_scale_);

        source->set_volume(sound_volume());
        source->play(play_count == 0);
    }

    auto sound_manager::play_ambience_sound3d(float fx, float fy, float fz, const char *file_name, int play_count) -> int {
        if (!device_ || 0.0f == sound_volume())
            return -1;

        const int index = free_source3d(file_name);
        sound_source *source = source3d(index);
        if (!source)
            return -1;

        // Ambience is pulled towards the listener by its own scale, from where it is when it starts.
        source->set_position((fx_position_ + (fx - fx_position_) * sound_scale_ / ambience_sound_scale_) / sound_scale_,
            (fy_position_ + (fy - fy_position_) * sound_scale_ / ambience_sound_scale_) / sound_scale_,
            (fz_position_ + (fz - fz_position_) * sound_scale_ / ambience_sound_scale_) / sound_scale_);

        source->set_volume(sound_volume());
        source->play(play_count == 0);

        return index;
    }

    void sound_manager::play_character_sound3d(float fx, float fy, float fz, const char *file_name, bool check_frequency) {
        if (!device_ || 0.0f == sound_volume())
            return;

        if (check_frequency) {
            static float limit_distance = 5000 * 5000;
            float fdx = (fx - fx_position_) * (fx - fx_position_);
            float fdy = (fy - fy_position_) * (fy - fy_position_);

            if (fdx + fdy > limit_distance)
                return;

            auto itor = play_sound_history_map_.find(file_name);
            if (play_sound_history_map_.end() != itor) {
                float time = itor->second;
                if (get_timer().current_second() - time < 0.3f) {
                    return;
                }
            }

            play_sound_history_map_.erase(file_name);
            play_sound_history_map_.insert(std::map<std::string, float>::value_type(file_name, get_timer().current_second()));
        }

        sound_source *source = source3d(free_source3d(file_name));
        if (!source)
            return;

        source->set_position(fx / sound_scale_, fy / sound_scale_, fz / sound_scale_);

        source->set_volume(sound_volume());
        source->play(false);
    }

    void sound_manager::stop_sound3d(int index) {
        if (sound_source *source = source3d(index))
            source->stop();
    }

    void sound_manager::set_sound_volume3d(int index, float volume) {
        if (sound_source *source = source3d(index))
            source->set_volume(volume);
    }

    void sound_manager::stop_all_sound3d() {
        for (int i = 0; i < SOUND3D_COUNT; ++i) {
            stop_sound3d(i);
        }

        play_sound_history_map_.clear();
    }

    void sound_manager::play_music(const char *file_name) {
        play_music(0, file_name, music_volume(), 0.0f);
    }

    void sound_manager::fade_in_music(const char *file_name, float volume_speed) {
        uint32_t index;
        if (music_index(file_name, &index)) {
            music_instances_[index].music_state_value = MUSIC_STATE_FADE_IN;
            music_instances_[index].volume_speed = volume_speed;
            return;
        }

        fade_out_all_music();

        for (int i = 0; i < MUSIC_COUNT; ++i) {
            music_instance &music_instance_value = music_instances_[i];
            if (MUSIC_STATE_OFF != music_instance_value.music_state_value)
                continue;

            play_music(i, file_name, 0.0f, volume_speed);
            return;
        }
    }

    void sound_manager::fade_limit_out_music(const char *file_name, float limit_volume, float volume_speed) {
        uint32_t index;
        if (!music_index(file_name, &index)) {
            metin2::log::debug("Cannot fade out music {}: not playing", file_name);
            return;
        }

        if (index >= MUSIC_COUNT) {
            metin2::log::debug("Cannot fade out music {}: instance index out of range", file_name);
            return;
        }

        music_instance &music_inst = music_instances_[index];
        music_inst.music_state_value = MUSIC_STATE_FADE_LIMIT_OUT;
        music_inst.volume_speed = volume_speed;
        music_inst.limit_volume = convert_ratio_volume_to_apply_volume(limit_volume);
    }

    void sound_manager::fade_out_music(const char *file_name, float volume_speed) {
        uint32_t index;
        if (!music_index(file_name, &index)) {
            metin2::log::debug("Cannot fade out music {}: not playing", file_name);
            return;
        }

        if (index >= MUSIC_COUNT) {
            metin2::log::debug("Cannot fade out music {}: instance index out of range", file_name);
            return;
        }

        music_instances_[index].music_state_value = MUSIC_STATE_FADE_OUT;
        music_instances_[index].volume_speed = volume_speed;
    }

    void sound_manager::fade_out_all_music() {
        for (int i = 0; i < MUSIC_COUNT; ++i) {
            if (MUSIC_STATE_OFF == music_instances_[i].music_state_value)
                continue;

            music_instances_[i].music_state_value = MUSIC_STATE_FADE_OUT;
            music_instances_[i].volume_speed = 0.01f;
        }
    }

    void sound_manager::save_volume() {
        // A second save is ignored.
        if (is_sound_disable_)
            return;

        float backup_music_volume = music_volume_;
        float backup_sound_volume = sound_volume_;
        set_music_volume_impl(0.0f);
        set_sound_volume(0.0f);
        backup_music_volume_ = backup_music_volume;
        backup_sound_volume_ = backup_sound_volume;
        is_sound_disable_ = true;
    }

    void sound_manager::restore_volume() {
        is_sound_disable_ = false;
        set_music_volume_impl(backup_music_volume_);
        set_sound_volume(backup_sound_volume_);
    }

    void sound_manager::play_music(uint32_t index, const char *c_sz_file_name, float volume, float volume_speed) {
        if (index >= MUSIC_COUNT)
            return;

        if (!device_)
            return;

        {
            std::lock_guard lock(music_mutex_);
            music_stream &music = music_[index];
            if (!music.open(c_sz_file_name)) {
                metin2::log::error("Cannot load music stream {}", c_sz_file_name);
                return;
            }

            music.set_volume(volume);
            music.play();
        }

        music_instance &music_instance_value = music_instances_[index];
        music_instance_value.volume = volume;
        music_instance_value.volume_speed = volume_speed;
        music_instance_value.music_state_value = MUSIC_STATE_FADE_IN;

        std::string str_file_name;
        string_path(c_sz_file_name, str_file_name);
        music_instance_value.music_file_name_crc = get_case_crc32(str_file_name.c_str(), str_file_name.length());
    }

    void sound_manager::stop_music(uint32_t index) {
        if (index >= MUSIC_COUNT)
            return;

        {
            std::lock_guard lock(music_mutex_);
            music_[index].close();
        }

        music_instance &music_instance_value = music_instances_[index];
        music_instance_value.volume = 0.0f;
        music_instance_value.volume_speed = 0.0f;
        music_instance_value.music_state_value = MUSIC_STATE_OFF;
        music_instance_value.music_file_name_crc = 0;
    }

    auto sound_manager::music_index(const char *c_sz_file_name, uint32_t *index) -> bool {
        std::string str_file_name;
        string_path(c_sz_file_name, str_file_name);
        uint32_t crc = get_case_crc32(str_file_name.c_str(), str_file_name.length());

        for (int i = 0; i < MUSIC_COUNT; ++i) {
            const music_instance &music_instance_value = music_instances_[i];
            if (MUSIC_STATE_OFF != music_instance_value.music_state_value && music_instance_value.music_file_name_crc == crc) {
                *index = i;
                return true;
            }
        }

        return false;
    }
}
