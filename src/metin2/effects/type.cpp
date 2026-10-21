#include "type.h"
#include <metin2/core/parse.h>

namespace metin2::inline effects {

    auto get_token_time_event_float(text_file_loader &text_file_loader, const char *key, time_event_table_float *time_event_table_float_value) -> bool {
        token_vector *token_vector;
        if (!text_file_loader.get_token_vector(key, &token_vector))
            return false;

        time_event_table_float_value->clear();
        time_event_table_float_value->resize(token_vector->size() / 2);

        uint32_t index = 0;
        for (uint32_t i = 0; i < token_vector->size(); i += 2, ++index) {
            time_event_table_float_value->at(index).time = metin2::to_number<double>(token_vector->at(i).c_str());
            time_event_table_float_value->at(index).value = metin2::to_number<double>(token_vector->at(i + 1).c_str());
        }

        return true;
    }
}
