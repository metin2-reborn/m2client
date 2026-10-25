#include "type.h"

namespace metin2::inline effects {

    auto read_time_events(asset_table &table, std::string_view key, time_event_table_float &events) -> bool {
        const toml::array *array = table.array(key);
        if (!array) {
            return true;
        }
        events.clear();
        for (const toml::node &node : *array) {
            float pair[2];
            if (!read_numbers(node, pair)) {
                table.error(key, "must hold [time, value] pairs");
                events.clear();
                return false;
            }
            time_event_type_float &event = events.emplace_back();
            event.time = pair[0];
            event.value = pair[1];
        }
        return true;
    }

    auto read_required_time_events(asset_table &table, std::string_view key, time_event_table_float &events) -> bool {
        if (!table.array(key)) {
            table.error(key, "is missing");
            return false;
        }
        return read_time_events(table, key, events);
    }
}
