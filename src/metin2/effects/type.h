#pragma once

#include <metin2/core/text_file_loader.h>

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include <metin2/math/math.h>

namespace metin2::inline effects {

    enum mesh_bill_board_type {
        MESH_BILLBOARD_TYPE_NONE,

        MESH_BILLBOARD_TYPE_ALL,
        MESH_BILLBOARD_TYPE_Y,

        MESH_BILLBOARD_TYPE_MOVE
    };

    enum bill_board_type {
        BILLBOARD_TYPE_NONE,

        BILLBOARD_TYPE_ALL,
        BILLBOARD_TYPE_Y,

        BILLBOARD_TYPE_LIE, // Lying flat on the ground.

        BILLBOARD_TYPE_2FACE, //     / and backslash
        BILLBOARD_TYPE_3FACE, //     / and \ and -

    };

    enum moving_type {
        MOVING_TYPE_DIRECT,
        MOVING_TYPE_BEZIER_CURVE,
    };

    struct effect_position {
        float time;

        math::vector3 position_value;

        // For Bezier curves.
        int moving_type_value;
        math::vector3 control_point;
    };

    inline auto operator<(const effect_position &lhs, const effect_position &rhs) -> bool {
        return lhs.time < rhs.time;
    }
    inline auto operator<(const float &lhs, const effect_position &rhs) -> bool {
        return lhs < rhs.time;
    }
    inline auto operator<(const effect_position &lhs, const float &rhs) -> bool {
        return lhs.time < rhs;
    }

    template <typename T>
    class time_event {
      public:
        time_event() = default;
        ~time_event() = default;

        float time;
        T value;
    };
#define AG_MASK 0xff00ff00
#define RB_MASK 0x00ff00ff

    struct dwordcolor {
        uint32_t color_value = 0;

        dwordcolor() = default;
        dwordcolor(const dwordcolor &r) = default;

        auto operator=(const dwordcolor &r) -> dwordcolor & = default;

        auto operator*=(float f) -> dwordcolor & {
            auto idx = uint32_t(f * 256);
            color_value =
                (((uint32_t)(((color_value & AG_MASK) >> 8) * idx)) & AG_MASK) + ((uint32_t)(((color_value & RB_MASK) * idx) >> 8) & RB_MASK);
            return *this;
        }
        auto operator+=(const dwordcolor &r) -> dwordcolor & {
            color_value += r.color_value;
            return *this;
        }
        operator uint32_t() {
            return color_value;
        }
    };
#undef AG_MASK
#undef RB_MASK

    inline auto operator*(dwordcolor dc, float f) -> dwordcolor {
        dwordcolor tmp(dc);
        tmp *= f;
        return tmp;
    }

    inline auto operator*(float f, dwordcolor dc) -> dwordcolor {
        dwordcolor tmp(dc);
        tmp *= f;
        return tmp;
    }

    template <typename T>
    inline auto operator<(const time_event<T> &lhs, const time_event<T> &rhs) -> bool {
        return lhs.time < rhs.time;
    }

    template <typename T>
    inline auto operator<(const time_event<T> &lhs, const float &rhs) -> bool {
        return lhs.time < rhs;
    }

    template <typename T>
    inline auto operator<(const float &lhs, const time_event<T> &rhs) -> bool {
        return lhs < rhs.time;
    }

    using time_event_type_float = time_event<float>;
    using time_event_type_color = time_event<dwordcolor>;

    using time_event_table_position = std::vector<effect_position>;
    using time_event_table_float = std::vector<time_event_type_float>;
    using time_event_table_color = std::vector<time_event_type_color>;

    // The time event value functions sometimes must not hand a value back, so they return a
    // pointer instead of the value itself.

    template <typename T>
    inline void get_time_event_blend_value(float elapsed_time, std::vector<time_event<T>> &vector, T *return_value) {
        if (vector.empty()) {
            *return_value = T();
            return;
        }

        if (vector.begin() + 1 == vector.end()) {
            *return_value = vector.front().value;
            return;
        }

        if (elapsed_time < vector.front().time) {
            *return_value = vector.front().value;
            return;
        }

        if (elapsed_time > vector.back().time) {
            *return_value = vector.back().value;
            return;
        }

        using iterator = typename std::vector<time_event<T>>::iterator;

        std::pair<iterator, iterator> result = std::equal_range(vector.begin(), vector.end(), elapsed_time);

        if (result.first != result.second)
            *return_value = result.first->value;
        else {
            --result.first;
            float head = (result.second->time - elapsed_time) / (result.second->time - result.first->time);
            *return_value = T((result.first->value - result.second->value) * head + (result.second->value));
        }
    }

    extern auto get_token_time_event_float(text_file_loader &text_file_loader, const char *key, time_event_table_float *time_event_table_float_value) -> bool;
}
