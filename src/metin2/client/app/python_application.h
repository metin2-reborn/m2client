#pragma once

#include <metin2/effects/effect_manager.h>
#include <metin2/graphics/grp_device.h>
#include <metin2/graphics/grp_light_manager.h>
#include <metin2/input/input.h>
#include <metin2/sound/sound_manager.h>
#include <metin2/ui/python_window_manager.h>
#include <metin2/window/application.h>
#include <metin2/window/cursor.h>
#include <metin2/world/actors/game_event_manager.h>
#include <metin2/world/actors/race_manager.h>
#include <metin2/world/flying/flying_object_manager.h>
#include <metin2/world/items/item_manager.h>

#include <metin2/client/background/python_mini_map.h>
#include <metin2/client/characters/python_character_manager.h>
#include <metin2/client/characters/python_non_player.h>
#include <metin2/client/guilds/python_guild.h>
#include <metin2/client/inventory/python_item.h>
#include <metin2/client/player/python_player.h>
#include <metin2/client/player/python_skill.h>
#include <metin2/client/protocol/python_network_stream.h>
#include <metin2/client/quest/python_event_manager.h>
#include <metin2/client/quest/python_quest.h>
#include <metin2/client/social/python_chat.h>
#include <metin2/client/social/python_messenger.h>
#include <metin2/client/social/python_text_tail.h>
#include <metin2/client/trade/python_exchange.h>
#include <metin2/client/trade/python_safe_box.h>
#include <metin2/client/trade/python_shop.h>
#include "python_ime.h"
#include "python_system.h"

#include <metin2/client/guilds/guild_mark_downloader.h>
#include <metin2/client/guilds/guild_mark_uploader.h>

#include <metin2/client/protocol/account_connector.h>

#include <metin2/client/protocol/server_state_checker.h>
#include <metin2/core/timer.h>
#include <metin2/script/resource.h>
#include <metin2/ui/python_graphic.h>
#include "abstract_application.h"

namespace metin2::inline client::inline app {

    class python_application : public application, public input_keyboard, public abstract_application {
      public:
        enum device_state {
            DEVICE_STATE_FALSE,
            DEVICE_STATE_SKIP,
            DEVICE_STATE_OK,
        };

        enum cursor_mode {
            CURSOR_MODE_HARDWARE,
            CURSOR_MODE_SOFTWARE,
        };

        enum cursor_shape {
            CURSOR_SHAPE_NORMAL,
            CURSOR_SHAPE_ATTACK,
            CURSOR_SHAPE_TARGET,
            CURSOR_SHAPE_TALK,
            CURSOR_SHAPE_CANT_GO,
            CURSOR_SHAPE_PICK,

            CURSOR_SHAPE_DOOR,
            CURSOR_SHAPE_CHAIR,
            CURSOR_SHAPE_MAGIC,
            CURSOR_SHAPE_BUY,
            CURSOR_SHAPE_SELL,

            CURSOR_SHAPE_CAMERA_ROTATE,
            CURSOR_SHAPE_HSIZE,
            CURSOR_SHAPE_VSIZE,
            CURSOR_SHAPE_HVSIZE,

            CURSOR_SHAPE_COUNT,

            // Short aliases, still used by the player input code.
            NORMAL = CURSOR_SHAPE_NORMAL,
            ATTACK = CURSOR_SHAPE_ATTACK,
            TARGET = CURSOR_SHAPE_TARGET,
            CAMERA_ROTATE = CURSOR_SHAPE_CAMERA_ROTATE,
            CURSOR_COUNT = CURSOR_SHAPE_COUNT,
        };

        enum info {
            INFO_ACTOR,
            INFO_EFFECT,
            INFO_ITEM,
            INFO_TEXTTAIL,
        };

        enum camera_control_direction {
            CAMERA_TO_POSITIVE = 1,
            CAMERA_TO_NEGITIVE = -1,
            CAMERA_STOP = 0,
        };

        enum {
            CAMERA_MODE_NORMAL = 0,
            CAMERA_MODE_STAND = 1,
            CAMERA_MODE_BLEND = 2,

            EVENT_CAMERA_NUMBER = 101,
        };

        struct camera_speed {
            float up_dir;
            float view_dir;
            float cross_dir;

            camera_speed() : up_dir(0.0f), view_dir(0.0f), cross_dir(0.0f) {}
        };

      public:
        python_application();
        ~python_application() override;

      public:
        void notify_hack(const char *format, ...);
        void get_info(uint32_t e_info, std::string *pst_info);

        void destroy();
        void clear();
        void exit();
        void abort();

        void set_min_fog(float min_fog);
        void set_frame_skip(bool is_enable);
        void skip_render_buffering(uint32_t sleep_m_sec) override;

        auto create(PyObject *self, const char *name, int width_value, int height, int windowed) -> bool;
        auto create_device(int width_value, int height) -> bool;

        void update_game();
        void render_game();

        auto process() -> bool override;

        auto create_cursors() -> bool;
        void destroy_cursors();

        auto set_cursor_num(int cursor_num) -> bool;
        void set_cursor_visible(bool flag, bool liar_cursor_on = false);
        auto cursor_visible() -> bool;
        auto liar_cursor_on() -> bool;
        void set_cursor_mode(int mode);
        auto get_cursor_mode() -> int;
        auto cursor_num() -> int { return cursor_num_; }

        void set_mouse_handler(PyObject *mouse_handler);

        auto width() -> int;
        auto height() -> int;

        void set_global_center_position(int32_t x, int32_t y);
        void set_center_position(float fx, float fy, float fz) override;
        void center_position(pixel_position *pixel_position);
        void set_camera(float distance, float pitch, float rotation, float destination_height);
        void get_camera(float *distance, float *pitch, float *rotation, float *destination_height);
        void rotate_camera(int direction);
        void pitch_camera(int direction);
        void zoom_camera(int direction);
        void movie_rotate_camera(int direction);
        void movie_pitch_camera(int direction);
        void movie_zoom_camera(int direction);
        void movie_reset_camera();
        void set_view_dir_camera_speed(float speed);
        void set_cross_dir_camera_speed(float speed);
        void set_up_dir_camera_speed(float speed);
        auto rotation() -> float;
        auto get_pitch() -> float;

        void set_server_time(time_t time) override;
        auto server_time() -> time_t;
        auto server_time_stamp() -> time_t;
        auto global_time() -> float override;
        auto global_elapsed_time() -> float override;

        auto face_speed() -> float { return face_spd_; }
        auto ave_render_time() -> float { return ave_render_time_; }
        auto cur_render_time() -> uint32_t { return cur_render_time_; }
        auto cur_update_time() -> uint32_t { return cur_update_time_; }
        auto update_fps() -> uint32_t { return update_fps_; }
        auto render_fps() -> uint32_t { return render_fps_; }
        auto face_count() -> uint32_t { return face_count_; }

        void run_ime_update() override;
        void run_ime_tab_event() override;
        void run_ime_return_event() override;
        void run_press_exit_key();

        void enable_special_camera_mode();
        void set_camera_speed(int percentage);

        auto is_lock_current_camera() -> bool;
        void set_event_camera(const camera_setting &camera_setting) override;
        void blend_event_camera(const camera_setting &camera_setting, float blend_time) override;
        void set_default_camera() override;

        void set_camera_setting(const camera_setting &camera_setting);
        void get_camera_setting(camera_setting *camera_setting);
        void save_camera_setting(const char *file_name);
        auto load_camera_setting(const char *file_name) -> bool;

        void set_force_sight_range(int range);

      protected:
        void on_close_request() override;
        void on_activate(bool active) override;
        void on_resize(bool minimized) override;
        void on_mouse_button(mouse_button button, bool down, point position) override;
        void on_char(char32_t codepoint) override;

        void on_camera_update();

        void on_ui_update();
        void on_ui_render();

        void on_mouse_update();
        void on_mouse_render();

        void on_mouse_wheel(int len) override;
        void on_mouse_move(int x, int y);
        void on_mouse_middle_button_down(int x, int y);
        void on_mouse_middle_button_up(int x, int y);
        void on_mouse_left_button_down(int x, int y);
        void on_mouse_left_button_up(int x, int y);
        void on_mouse_left_button_double_click(int x, int y);
        void on_mouse_right_button_down(int x, int y);
        void on_mouse_right_button_up(int x, int y);
        void on_key_down(int index) override;
        void on_key_up(int index) override;
        void on_ime_key_down(int index) override;

        auto is_continuous_change_type_cursor(int cursor_num) -> bool;

        void update_camera();

      protected:
        graphic_device grp_device_;

        PyObject *mouse_handler_;
        math::vector3 center_position_;

        unsigned int fps_;
        float ave_render_time_;
        uint32_t cur_render_time_;
        uint32_t cur_update_time_;
        uint32_t load_;
        uint32_t width_;
        uint32_t height_;

      protected:
        uint32_t last_idle_time_;
        uint32_t start_local_time_;
        time_t server_time_;
        time_t local_start_time_;
        float global_time_;
        float global_elapsed_time_;

        camera_setting default_camera_setting_;
        camera_setting event_camera_setting_;

        int camera_mode_;
        float blend_camera_start_time_;
        float blend_camera_blend_time_;
        camera_setting end_blend_camera_setting_;

        float rotation_speed_;
        float pitch_speed_;
        float zoom_speed_;
        float camera_rotate_speed_;
        float camera_pitch_speed_;
        float camera_zoom_speed_;

        camera_pos cmr_pos_;
        camera_speed cmr_spd_;

        bool is_special_camera_mode_;

        float face_spd_;
        uint32_t face_spd_sum_;
        uint32_t face_spd_count_;

        uint32_t face_acc_count_;
        uint32_t face_acc_time_;

        uint32_t update_fps_;
        uint32_t render_fps_;
        uint32_t face_count_;

        uint32_t l_button_down_time_;
        uint32_t l_button_up_time_;

        std::map<int, std::unique_ptr<window::cursor>> cursors_;
        int mouse_capture_count_;

        bool cursor_visible_;
        bool liar_cursor_on_;
        int cursor_mode_value_;
        bool is_windowed_;
        bool is_frame_skip_disable_;

        std::string ip_;
        int port_;

        bool is_minimized_wnd_;
        bool is_activate_wnd_;

        uint32_t buf_sleep_skip_time_;
        int force_sight_range_;

      protected:
        int cursor_num_;
        int continuous_cursor_num_;
    };

    inline auto get_python_application() -> python_application & {
        static python_application instance;
        return instance;
    }
}
