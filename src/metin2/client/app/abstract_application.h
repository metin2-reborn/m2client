#pragma once

#include <cstdint>
#include <ctime>
#include <metin2/math/math.h>

namespace metin2::inline client::inline app {

    class abstract_application {
      public:
        struct camera_pos {
            float up_dir;
            float view_dir;
            float cross_dir;

            camera_pos() : up_dir(0.0f), view_dir(0.0f), cross_dir(0.0f) {}
        };

        struct camera_setting {
            math::vector3 center_position;
            camera_pos cmr_pos;
            float rotation;
            float pitch;
            float zoom;

            camera_setting() : center_position(0.0f, 0.0f, 0.0f),
                               rotation(0.0f),
                               pitch(0.0f),
                               zoom(0.0f) {}
        };

      public:
        abstract_application() = default;
        virtual ~abstract_application() = default;

        virtual auto global_time() -> float = 0;
        virtual auto global_elapsed_time() -> float = 0;

        virtual void skip_render_buffering(uint32_t sleep_m_sec) = 0;
        virtual void set_server_time(time_t time) = 0;
        virtual void set_center_position(float fx, float fy, float fz) = 0;

        virtual void set_event_camera(const camera_setting &camera_setting_value) = 0;
        virtual void blend_event_camera(const camera_setting &camera_setting_value, float blend_time) = 0;
        virtual void set_default_camera() = 0;

        virtual void run_ime_update() = 0;
        virtual void run_ime_tab_event() = 0;
        virtual void run_ime_return_event() = 0;
    };

    auto get_abstract_application() -> abstract_application &;
}
