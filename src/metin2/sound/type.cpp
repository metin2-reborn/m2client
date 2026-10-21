#include "type.h"
#include <metin2/core/parse.h>

#include <cstdio>
#include <cstdlib>

#include <metin2/core/text_file_loader.h>
#include <metin2/log/log.h>

namespace metin2::inline sound {

    auto sound_types::load_sound_information_piece(const char *file_name, sound_types::sound_data_vector &sound_data_vector_value, const char *path_header) -> bool {
        text_file_loader *pk_text_file_loader = text_file_loader::cache(file_name);
        if (!pk_text_file_loader)
            return false;

        text_file_loader &rk_text_file_loader = *pk_text_file_loader;
        // Most motions have no sound script; a missing or empty one is not an error.
        if (rk_text_file_loader.is_empty())
            return false;

        rk_text_file_loader.set_top();

        int count;
        if (!rk_text_file_loader.token_integer("sounddatacount", &count)) {
            metin2::log::error("Sound script {}: SoundDataCount not found", file_name);
            return false;
        }

        sound_data_vector_value.clear();
        sound_data_vector_value.resize(count);

        char sound_data_header[32 + 1];
        for (size_t i = 0; i < sound_data_vector_value.size(); ++i) {
            snprintf(sound_data_header, sizeof(sound_data_header), "sounddata%02zu", i);
            token_vector *token_vector;
            if (!rk_text_file_loader.get_token_vector(sound_data_header, &token_vector)) {
                metin2::log::error("Sound script {}: {} not found", file_name, sound_data_header);
                return false;
            }

            if (2 != token_vector->size()) {
                metin2::log::error("Sound script {}: {} does not have two values", file_name, sound_data_header);
                return false;
            }

            sound_data_vector_value[i].time = (float)metin2::to_number<double>(token_vector->at(0).c_str());
            if (path_header) {
                sound_data_vector_value[i].sound_file_name = path_header;
                sound_data_vector_value[i].sound_file_name += token_vector->at(1).c_str();
            } else {
                sound_data_vector_value[i].sound_file_name = token_vector->at(1).c_str();
            }
        }

        return true;
    }

    void sound_types::data_to_instance(const sound_data_vector &sound_data_vector_value, sound_instance_vector *sound_instance_vector_value) {
        if (sound_data_vector_value.empty())
            return;

        const float frame_time_value = 1.0f / 60.0f;

        sound_instance_vector_value->clear();
        sound_instance_vector_value->resize(sound_data_vector_value.size());
        for (size_t i = 0; i < sound_data_vector_value.size(); ++i) {
            const sound_data_entry &sound_data = sound_data_vector_value[i];
            sound_instance_entry &sound_instance = sound_instance_vector_value->at(i);

            sound_instance.frame = uint32_t(sound_data.time / frame_time_value);
            sound_instance.sound_file_name = sound_data.sound_file_name;
        }
    }
}
