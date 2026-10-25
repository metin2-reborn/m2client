#include "type.h"

#include <metin2/core/mapped_file.h>
#include <metin2/files/asset_toml.h>
#include <metin2/files/eter_file_manager.h>

namespace metin2::inline sound {

    auto sound_types::load_sound_script(std::string_view text, std::string_view file_name, sound_types::sound_data_vector &sound_data_vector_value) -> bool {
        sound_data_vector_value.clear();
        const auto table = parse_asset_toml(text, file_name, "SoundScript");
        if (!table) {
            return false;
        }

        bool ok = true;
        asset_table root(*table, file_name);
        root.table("asset");
        if (const toml::array *sounds = root.array("sound")) {
            for (const toml::node &node : *sounds) {
                const toml::table *sound = node.as_table();
                if (!sound) {
                    root.error("sound", "must be an array of tables");
                    continue;
                }
                asset_table entry(*sound, file_name);
                sound_data_entry data{};
                entry.required("time", data.time);
                entry.required("file", data.sound_file_name);
                if (entry.finish()) {
                    sound_data_vector_value.push_back(std::move(data));
                } else {
                    ok = false;
                }
            }
        }
        return root.finish() && ok;
    }

    auto sound_types::load_sound_information_piece(const char *file_name, sound_types::sound_data_vector &sound_data_vector_value) -> bool {
        if (!get_file_manager().is_exist(file_name)) {
            return false;
        }

        mapped_file file;
        const void *data = nullptr;
        if (!get_file_manager().get(file, file_name, &data)) {
            return false;
        }
        return load_sound_script(std::string_view(static_cast<const char *>(data), file.size()), file_name, sound_data_vector_value);
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
