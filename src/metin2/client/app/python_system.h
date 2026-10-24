#pragma once
#include <metin2/script/python_header.h>

#include <string_view>

namespace metin2::inline client::inline app {
    // English is the only locale shipped; the scripts and the loaders below read everything from here.
    inline constexpr std::string_view LOCALE_DIR = "locale/en";

    class python_system {
      public:
        enum window {
            WINDOW_STATUS,
            WINDOW_INVENTORY,
            WINDOW_ABILITY,
            WINDOW_SOCIETY,
            WINDOW_JOURNAL,
            WINDOW_COMMAND,

            WINDOW_QUICK,
            WINDOW_GAUGE,
            WINDOW_MINIMAP,
            WINDOW_CHAT,

            WINDOW_MAX_NUM,
        };

        enum {
            FREQUENCY_MAX_NUM = 30,
            RESOLUTION_MAX_NUM = 100
        };

        struct resolution {
            uint32_t width;
            uint32_t height;
            uint32_t bpp; // bits per pixel (high-color = 16bpp, true-color = 32bpp)

            uint32_t frequency[20];
            uint8_t frequency_count;
        };

        struct window_status {
            int is_visible;
            int is_minimized;

            int ix_position;
            int iy_position;
            int height;
        };

        struct config {
            uint32_t width;
            uint32_t height;
            uint32_t bpp;
            uint32_t frequency;

            bool is_software_cursor;
            bool is_object_culling;
            int distance;
            int shadow_level;

            float music_volume;
            uint8_t voice_volume;

            int gamma;

            int is_save_id;
            char save_id[20];

            bool windowed;
            bool vsync;
            int fps_limit; // 0: as fast as vsync or the machine allows
            bool antialias;
            bool font_antialias;
            float font_scale;
            bool decompress_dds;
            bool no_sound_card;
            uint8_t software_tiling;
            bool view_chat;
            bool always_show_name;
            bool show_damage;
            bool show_sales_text;
            char renderer[16]; // gpu backend name
        };

      public:
        python_system();

        ~python_system();

        void clear();
        void set_interface_handler(PyObject *handler);
        void destroy_interface_handler();

        void set_default_config();
        auto load_config() -> bool;
        auto save_config() -> bool;
        void change_system();

        auto load_interface_status() -> bool;
        void save_interface_status();

        auto width() -> uint32_t;
        auto height() -> uint32_t;
        auto bpp() -> uint32_t;
        auto frequency() -> uint32_t;
        auto is_software_cursor() -> bool;
        auto is_windowed() -> bool;
        auto is_view_chat() -> bool;
        auto is_always_show_name() -> bool;
        auto is_show_damage() -> bool;
        auto is_show_sales_text() -> bool;
        auto is_no_sound_card() -> bool;
        auto is_auto_tiling() -> bool;
        auto is_software_tiling() -> bool;
        auto renderer() const -> const char * { return config_value_.renderer; }
        auto is_vsync() const -> bool { return config_value_.vsync; }
        auto fps_limit() const -> int { return config_value_.fps_limit; }
        auto gamma() const -> int { return config_value_.gamma; }
        auto is_antialias() const -> bool { return config_value_.antialias; }
        auto is_font_antialias() const -> bool { return config_value_.font_antialias; }
        auto font_scale() const -> float { return config_value_.font_scale; }
        void set_software_tiling(bool is_enable);
        void set_view_chat_flag(int flag);
        void set_always_show_name_flag(int flag);
        void set_show_damage_flag(int flag);
        void set_show_sales_text_flag(int flag);

        void display_settings();

        auto get_resolution(int index, uint32_t *width_value, uint32_t *height, uint32_t *bpp) -> bool;
        auto frequency(int index, int freq_index, uint32_t *frequncy) -> bool;

        auto music_volume() -> float;
        auto sound_volume() -> int;
        void set_music_volume(float volume);
        void set_sound_volumef(float volume);

        auto distance() -> int;
        auto shadow_level() -> int;
        void set_shadow_level(unsigned int level);

      protected:
        resolution resolution_list_[RESOLUTION_MAX_NUM];
        int resolution_count_;
        config config_value_;
        config old_config_;
        bool is_interface_config_;
        PyObject *interface_handler_;
        window_status window_status_value_[WINDOW_MAX_NUM];
    };

    inline auto get_python_system() -> python_system & {
        static python_system instance;

        return instance;
    }
}
