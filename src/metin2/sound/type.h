#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace metin2::inline sound {

    namespace sound_types {
        struct sound_data_entry {
            float time;
            std::string sound_file_name;
        };
        struct sound_instance_entry {
            uint32_t frame;
            std::string sound_file_name;
        };
        using sound_data_vector = std::vector<sound_data_entry>;
        using sound_instance_vector = std::vector<sound_instance_entry>;

        // Parses the text of a sound script (TOML, [asset] type "SoundScript").
        auto load_sound_script(std::string_view text, std::string_view file_name, sound_data_vector &sound_data_vector_value) -> bool;
        // Reads a sound script through the file manager; false without a log when it doesn't exist,
        // since most motions and effects have none.
        auto load_sound_information_piece(const char *file_name, sound_data_vector &sound_data_vector_value) -> bool;
        void data_to_instance(const sound_data_vector &sound_data_vector_value, sound_instance_vector *sound_instance_vector_value);

    };
}
