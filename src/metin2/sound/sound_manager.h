#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "music_stream.h"
#include "sound_data.h"
#include "sound_source.h"
#include "type.h"

struct ALCdevice;
struct ALCcontext;

namespace metin2::inline sound {

    // Sound effects and music on OpenAL. Without an audio device (or with [sound] no_sound_card) the
    // game runs silently.
    class sound_manager {
      public:
        enum {
            SOUND2D_COUNT = 4,
            SOUND3D_COUNT = 32,
            MUSIC_COUNT = 3,
        };

        sound_manager();
        ~sound_manager();

        auto create() -> bool;
        void destroy();

        void set_position(float fx, float fy, float fz);
        void set_direction(float fx_dir, float fy_dir, float fz_dir, float fx_up, float fy_up, float fz_up);
        void update();

        void set_sound_scale(float scale);
        void set_ambience_sound_scale(float scale);
        void set_sound_volume(float volume);
        void set_sound_volume_ratio(float ratio);
        void set_music_volume(float volume);
        void set_sound_volume_grade(int grade);
        void save_volume();
        void restore_volume();
        auto sound_volume() -> float;
        auto music_volume() -> float;

        void play_sound2d(const char *file_name);
        void play_sound3d(float fx, float fy, float fz, const char *file_name, int play_count = 1);
        void stop_sound3d(int index);
        auto play_ambience_sound3d(float fx, float fy, float fz, const char *file_name, int play_count = 1) -> int;
        void play_character_sound3d(float fx, float fy, float fz, const char *file_name, bool check_frequency = false);
        void set_sound_volume3d(int index, float volume);
        void stop_all_sound3d();

        void play_music(const char *file_name);
        void fade_in_music(const char *file_name, float volume_speed = 0.016f);
        void fade_out_music(const char *file_name, float volume_speed = 0.016f);
        void fade_limit_out_music(const char *file_name, float limit_volume, float volume_speed = 0.016f);
        void fade_out_all_music();

        void update_sound_instance(float fx, float fy, float fz, uint32_t dwcur_frame, const sound_types::sound_instance_vector *sound_instance_vector, bool check_frequency = false);
        void update_sound_instance(uint32_t dwcur_frame, const sound_types::sound_instance_vector *sound_instance_vector);

        // One line for the automation harness: device, sources and music streams.
        auto debug_status() -> std::string;

      protected:
        enum music_state {
            MUSIC_STATE_OFF,
            MUSIC_STATE_PLAY,
            MUSIC_STATE_FADE_IN,
            MUSIC_STATE_FADE_OUT,
            MUSIC_STATE_FADE_LIMIT_OUT,
        };
        struct music_instance {
            uint32_t music_file_name_crc;
            music_state music_state_value;
            float volume;
            float limit_volume;
            float volume_speed;
        };

        void play_music(uint32_t index, const char *file_name, float volume, float volume_speed);
        void stop_music(uint32_t index);
        auto music_index(const char *file_name, uint32_t *index) -> bool;

      protected:
        auto convert_grade_volume_to_apply_volume(int volume_grade) -> float;
        auto convert_ratio_volume_to_apply_volume(float volume_ratio) -> float;
        void set_music_volume_impl(float volume);
        auto data(const char *file_name) -> sound_data &;
        // A free source of the pool with the sound set, or nullptr; the search starts one slot
        // further each time so the sources rotate.
        auto free_source2d(const char *file_name) -> sound_source *;
        auto free_source3d(const char *file_name) -> int;
        auto source3d(int index) -> sound_source *;

        void open_device();
        void close_device();
        void music_service();

      protected:
        bool is_sound_disable_;

        float fx_position_;
        float fy_position_;
        float fz_position_;

        float sound_scale_;
        float ambience_sound_scale_;
        float sound_volume_;
        float music_volume_;

        float backup_music_volume_;
        float backup_sound_volume_;

        music_instance music_instances_[MUSIC_COUNT];
        std::map<std::string, float> play_sound_history_map_;

        ALCdevice *device_ = nullptr;
        ALCcontext *context_ = nullptr;

        std::map<uint32_t, std::unique_ptr<sound_data>> data_;
        std::array<sound_source, SOUND2D_COUNT> sources2d_;
        std::array<sound_source, SOUND3D_COUNT> sources3d_;
        uint32_t next2d_ = 0;
        uint32_t next3d_ = 0;

        // The music buffers are refilled on their own thread, so a long frame (a map load) does not
        // make the music skip. Every music_stream call holds music_mutex_.
        std::array<music_stream, MUSIC_COUNT> music_;
        std::mutex music_mutex_;
        std::jthread music_thread_;
    };

    inline auto get_sound_manager() -> sound_manager & {
        static sound_manager instance;
        return instance;
    }
}
