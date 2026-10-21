#include <metin2/files/eter_file_manager.h>

#include <cstdarg>
#include "text_file_loader.h"

namespace metin2::inline core {

    void printf_tabs(FILE *file, int tab_count, const char *string, ...) {
        va_list args;
        va_start(args, string);

        char buf[1024];
        vsnprintf(buf, sizeof(buf), string, args);
        va_end(args);

        for (int i = 0; i < tab_count; ++i)
            fputs("    ", file);

        fputs(buf, file);
    }

    auto load_multiple_text_data(const char *file_name, token_vector_map &token_vector_map_value) -> bool {
        const void *model_data;
        mapped_file file;

        if (!get_file_manager().get(file, file_name, &model_data))
            return false;

        uint32_t i;

        memory_text_file_loader text_file_loader_value;
        token_vector token_vector_value;

        text_file_loader_value.bind(file.size(), model_data);

        for (i = 0; i < text_file_loader_value.line_count(); ++i) {
            if (!text_file_loader_value.split_line(i, &token_vector_value))
                continue;

            stl_lowers(token_vector_value[0]);

            if (0 == token_vector_value[0].compare("start")) {
                token_vector sub_token_vector;

                stl_lowers(token_vector_value[1]);
                std::string key = token_vector_value[1];
                token_vector_value.clear();

                for (i = i + 1; i < text_file_loader_value.line_count(); ++i) {
                    if (!text_file_loader_value.split_line(i, &sub_token_vector))
                        continue;

                    stl_lowers(sub_token_vector[0]);

                    if (0 == sub_token_vector[0].compare("end")) {
                        break;
                    }

                    for (uint32_t j = 0; j < sub_token_vector.size(); ++j) {
                        token_vector_value.push_back(sub_token_vector[j]);
                    }
                }

                token_vector_map_value.insert(token_vector_map::value_type(key, token_vector_value));
            } else {
                std::string key = token_vector_value[0];
                token_vector_value.erase(token_vector_value.begin());
                token_vector_map_value.insert(token_vector_map::value_type(key, token_vector_value));
            }
        }

        return true;
    }

}
