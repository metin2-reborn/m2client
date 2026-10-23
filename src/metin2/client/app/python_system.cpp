#include "python_system.h"

#include <metin2/client/background/python_background.h>
#include <metin2/config/config_reader.h>
#include <metin2/config/state_dir.h>
#include <metin2/core/utf8.h>
#include <metin2/core/utils.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include <metin2/window/main_window.h>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <toml++/toml.hpp>
#include <type_traits>

namespace metin2::inline client::inline app {
    void python_system::set_interface_handler(PyObject *handler) {
        // The reference count is not changed; the remaining reference keeps Python from deleting it.
        // Instead __del__ calls Destroy, which sets the handler to null.

        interface_handler_ = handler;
    }

    void python_system::destroy_interface_handler() {
        interface_handler_ = nullptr;
    }

    void python_system::display_settings() {
        memset(resolution_list_, 0, sizeof(resolution) * RESOLUTION_MAX_NUM);

        resolution_count_ = 0;

        for (const metin2::window::display_mode &display_mode : metin2::window::display_modes()) {
            if (display_mode.width < 800 || display_mode.height < 600) {
                continue;
            }

            const uint32_t bpp = display_mode.bits_per_pixel;

            int check_res = false;

            for (int i = 0; !check_res && i < resolution_count_; ++i) {
                if (resolution_list_[i].bpp != bpp ||
                    resolution_list_[i].width != display_mode.width ||
                    resolution_list_[i].height != display_mode.height) {
                    continue;
                }

                int check_fre = false;

                // Same resolution, so only add the refresh rate.
                for (int j = 0; j < resolution_list_[i].frequency_count; ++j) {
                    if (resolution_list_[i].frequency[j] == display_mode.refresh_rate) {
                        check_fre = true;
                        break;
                    }
                }

                if (!check_fre) {
                    if (resolution_list_[i].frequency_count < FREQUENCY_MAX_NUM) {
                        resolution_list_[i].frequency[resolution_list_[i].frequency_count++] = display_mode.refresh_rate;
                    }
                }

                check_res = true;
            }

            if (!check_res) {
                if (resolution_count_ < RESOLUTION_MAX_NUM) {
                    resolution_list_[resolution_count_].width = display_mode.width;
                    resolution_list_[resolution_count_].height = display_mode.height;
                    resolution_list_[resolution_count_].bpp = bpp;
                    resolution_list_[resolution_count_].frequency[0] = display_mode.refresh_rate;
                    resolution_list_[resolution_count_].frequency_count = 1;

                    ++resolution_count_;
                }
            }
        }
    }

    auto python_system::get_resolution(int index, uint32_t *width_value, uint32_t *height, uint32_t *bpp) -> bool {
        if (index >= resolution_count_) {
            return false;
        }

        *width_value = resolution_list_[index].width;
        *height = resolution_list_[index].height;
        *bpp = resolution_list_[index].bpp;

        return true;
    }

    auto python_system::frequency(int index, int freq_index, uint32_t *frequncy) -> bool {
        if (index >= resolution_count_) {
            return false;
        }

        if (freq_index >= resolution_list_[index].frequency_count) {
            return false;
        }

        *frequncy = resolution_list_[index].frequency[freq_index];
        return true;
    }

    auto python_system::width() -> uint32_t {
        return config_value_.width;
    }

    auto python_system::height() -> uint32_t {
        return config_value_.height;
    }
    auto python_system::bpp() -> uint32_t {
        return config_value_.bpp;
    }
    auto python_system::frequency() -> uint32_t {
        return config_value_.frequency;
    }

    auto python_system::is_no_sound_card() -> bool {
        return config_value_.no_sound_card;
    }

    auto python_system::is_software_cursor() -> bool {
        return config_value_.is_software_cursor;
    }

    auto python_system::music_volume() -> float {
        return config_value_.music_volume;
    }

    auto python_system::sound_volume() -> int {
        return config_value_.voice_volume;
    }

    void python_system::set_music_volume(float volume) {
        config_value_.music_volume = volume;
    }

    void python_system::set_sound_volumef(float volume) {
        config_value_.voice_volume = int(5 * volume);
    }

    auto python_system::distance() -> int {
        return config_value_.distance;
    }

    auto python_system::shadow_level() -> int {
        return config_value_.shadow_level;
    }

    void python_system::set_shadow_level(unsigned int level) {
        config_value_.shadow_level = static_cast<int>(std::min(level, 5u));
        get_python_background().refresh_shadow_level();
    }

    void python_system::set_default_config() {
        memset(&config_value_, 0, sizeof(config_value_));

        config_value_.width = 1024;
        config_value_.height = 768;
        config_value_.bpp = 32;

        config_value_.windowed = false;

        config_value_.is_software_cursor = false;
        config_value_.is_object_culling = true;
        config_value_.distance = 3;

        config_value_.gamma = 3;
        config_value_.music_volume = 1.0f;
        config_value_.voice_volume = 5;

        config_value_.decompress_dds = 0;
        config_value_.software_tiling = 0;
        config_value_.shadow_level = 3;
        config_value_.view_chat = true;
        config_value_.always_show_name = true;
        config_value_.show_damage = true;
        config_value_.show_sales_text = true;
        utf8::copy_truncated(config_value_.renderer, sizeof(config_value_.renderer), "opengl");
        config_value_.vsync = false;
        config_value_.font_antialias = false;
        config_value_.font_scale = 1.0f;
    }

    auto python_system::is_windowed() -> bool {
        return config_value_.windowed;
    }

    auto python_system::is_view_chat() -> bool {
        return config_value_.view_chat;
    }

    void python_system::set_view_chat_flag(int flag) {
        config_value_.view_chat = flag == 1 ? true : false;
    }

    auto python_system::is_always_show_name() -> bool {
        return config_value_.always_show_name;
    }

    void python_system::set_always_show_name_flag(int flag) {
        config_value_.always_show_name = flag == 1 ? true : false;
    }

    auto python_system::is_show_damage() -> bool {
        return config_value_.show_damage;
    }

    void python_system::set_show_damage_flag(int flag) {
        config_value_.show_damage = flag == 1 ? true : false;
    }

    auto python_system::is_show_sales_text() -> bool {
        return config_value_.show_sales_text;
    }

    void python_system::set_show_sales_text_flag(int flag) {
        config_value_.show_sales_text = flag == 1 ? true : false;
    }

    auto python_system::is_auto_tiling() -> bool {
        return config_value_.software_tiling == 0;
    }

    void python_system::set_software_tiling(bool is_enable) {
        if (is_enable) {
            config_value_.software_tiling = 1;
        } else {
            config_value_.software_tiling = 2;
        }
    }

    auto python_system::is_software_tiling() -> bool {
        return config_value_.software_tiling == 1;
    }

    namespace {
        constexpr const char *CONFIG_FILE = "metin2.toml";
        constexpr const char *INTERFACE_FILE = "interface.dat";

        auto read_file(const std::string &path, std::string &out) -> bool {
            FILE *fp = fopen(path.c_str(), "rb");
            if (!fp) {
                return false;
            }

            char buffer_value[4096];

            size_t count;
            while ((count = fread(buffer_value, 1, sizeof(buffer_value), fp)) > 0) {
                out.append(buffer_value, count);
            }

            fclose(fp);
            return true;
        }

        auto parse_log_level(std::string_view name, metin2::log::level &out) -> bool {
            constexpr std::pair<std::string_view, metin2::log::level> levels[] = {
                {"debug", metin2::log::level::debug},
                {"info",  metin2::log::level::info },
                {"warn",  metin2::log::level::warn },
                {"error", metin2::log::level::error},
            };

            for (const auto &[text, value] : levels) {
                if (text == name) {
                    out = value;
                    return true;
                }
            }

            return false;
        }

        constexpr std::string_view TILING_NAMES[] = {"auto", "software", "hardware"};
    }

    auto python_system::load_config() -> bool {
        const std::string path = metin2::state::path(CONFIG_FILE);
        std::string text;
        if (!read_file(path, text)) {
            return false;
        }

        toml::parse_result parsed = toml::parse(std::string_view(text), std::string_view(path));
        if (!parsed) {
            const toml::parse_error &error = parsed.error();
            metin2::log::error("{}:{}: {}; the default settings are used", path, error.source().begin.line, error.description());
            return false;
        }

        metin2::config::reader reader(parsed.table(), path);
        reader.read("display", "width", config_value_.width);
        reader.read("display", "height", config_value_.height);
        reader.read("display", "bpp", config_value_.bpp);
        reader.read("display", "frequency", config_value_.frequency);
        reader.read("display", "windowed", config_value_.windowed);
        reader.read("display", "gamma", config_value_.gamma);
        reader.read("display", "software_cursor", config_value_.is_software_cursor);
        reader.read("display", "vsync", config_value_.vsync);

        std::string tiling = std::string(TILING_NAMES[std::min<uint8_t>(config_value_.software_tiling, 2)]);
        reader.read("graphics", "visibility", config_value_.distance);
        reader.read("graphics", "object_culling", config_value_.is_object_culling);
        reader.read("graphics", "shadow_level", config_value_.shadow_level);
        reader.read("graphics", "tiling", tiling);
        reader.read("graphics", "decompressed_textures", config_value_.decompress_dds);
        reader.read("graphics", "preloading_delay", loading_delay_time);
        std::string renderer = config_value_.renderer;
        reader.read("graphics", "renderer", renderer);
        utf8::copy_truncated(config_value_.renderer, sizeof(config_value_.renderer), renderer);

        const auto tiling_it = std::ranges::find(TILING_NAMES, tiling);
        if (tiling_it != std::end(TILING_NAMES)) {
            config_value_.software_tiling = static_cast<uint8_t>(tiling_it - std::begin(TILING_NAMES));
        } else {
            metin2::log::error("{}: graphics.tiling must be \"auto\", \"software\" or \"hardware\"; the default is used", path);
        }

        reader.read("sound", "music_volume", config_value_.music_volume);
        reader.read("sound", "voice_volume", config_value_.voice_volume);
        reader.read("sound", "no_sound_card", config_value_.no_sound_card);

        reader.read("interface", "view_chat", config_value_.view_chat);
        reader.read("interface", "always_show_name", config_value_.always_show_name);
        reader.read("interface", "show_damage", config_value_.show_damage);
        reader.read("interface", "show_sales_text", config_value_.show_sales_text);
        reader.read("interface", "font_antialias", config_value_.font_antialias);
        reader.read("interface", "font_scale", config_value_.font_scale);
        if (config_value_.font_scale <= 0.1f) {
            metin2::log::error("{}: interface.font_scale must be above 0.1; the default is used", path);
            config_value_.font_scale = 1.0f;
        }

        std::string saved_id;
        reader.read("login", "save_id", config_value_.is_save_id);
        reader.read("login", "id", saved_id);
        utf8::copy_truncated(config_value_.save_id, sizeof(config_value_.save_id), saved_id);

        std::string log_level = "info";
        reader.read("log", "level", log_level);
        metin2::log::level level;
        if (parse_log_level(log_level, level)) {
            metin2::log::set_level(level);
        } else {
            metin2::log::error("{}: log.level must be \"debug\", \"info\", \"warn\" or \"error\"", path);
        }

        reader.report_unknown_keys();

        if (config_value_.windowed) {
            const metin2::window::point monitor = metin2::window::monitor_size();
            unsigned screen_width = monitor.x;
            unsigned screen_height = monitor.y;

            if (config_value_.width >= screen_width) {
                config_value_.width = screen_width;
            }

            if (config_value_.height >= screen_height) {
                int config_height = config_value_.height;
                int difference = (config_height - screen_height) + 7;
                config_value_.height = config_height - difference;
            }
        }

        old_config_ = config_value_;
        return true;
    }

    auto python_system::save_config() -> bool {
        const toml::table root{
            {"display",
             toml::table{
                    {"width", static_cast<int64_t>(config_value_.width)},
                    {"height", static_cast<int64_t>(config_value_.height)},
                    {"bpp", static_cast<int64_t>(config_value_.bpp)},
                    {"frequency", static_cast<int64_t>(config_value_.frequency)},
                    {"windowed", config_value_.windowed},
                    {"gamma", static_cast<int64_t>(config_value_.gamma)},
                    {"software_cursor", config_value_.is_software_cursor},
                    {"vsync", config_value_.vsync},
                }},
            {"graphics",
             toml::table{
                    {"visibility", static_cast<int64_t>(config_value_.distance)},
                    {"object_culling", config_value_.is_object_culling},
                    {"shadow_level", static_cast<int64_t>(config_value_.shadow_level)},
                    {"tiling", std::string(TILING_NAMES[std::min<uint8_t>(config_value_.software_tiling, 2)])},
                    {"decompressed_textures", config_value_.decompress_dds},
                    {"preloading_delay", static_cast<int64_t>(loading_delay_time)},
                    {"renderer", std::string(config_value_.renderer)},
                }},
            {"sound",
             toml::table{
                    {"music_volume", static_cast<double>(config_value_.music_volume)},
                    {"voice_volume", static_cast<int64_t>(config_value_.voice_volume)},
                    {"no_sound_card", config_value_.no_sound_card},
                }},
            {"interface",
             toml::table{
                    {"view_chat", config_value_.view_chat},
                    {"always_show_name", config_value_.always_show_name},
                    {"show_damage", config_value_.show_damage},
                    {"show_sales_text", config_value_.show_sales_text},
                    {"font_antialias", config_value_.font_antialias},
                    {"font_scale", static_cast<double>(config_value_.font_scale)},
                }},
            {"login",
             toml::table{
                    {"save_id", static_cast<int64_t>(config_value_.is_save_id)},
                    {"id", std::string(config_value_.save_id, strnlen(config_value_.save_id, sizeof(config_value_.save_id)))},
                }},
            {"log",
             toml::table{
                    {"level", std::string(metin2::log::enabled(metin2::log::level::debug) ? "debug" : metin2::log::enabled(metin2::log::level::info) ? "info"
                                                                                                  : metin2::log::enabled(metin2::log::level::warn)   ? "warn"
                                                                                                                                                     : "error")},
                }},
        };

        std::ostringstream text;
        text << root << '\n';

        const std::string path = metin2::state::path(CONFIG_FILE);

        FILE *fp = fopen(path.c_str(), "w");
        if (!fp) {
            metin2::log::error("Cannot write {}", path);
            return false;
        }

        const std::string content = text.str();

        fwrite(content.data(), 1, content.size(), fp);
        fclose(fp);

        return true;
    }

    auto python_system::load_interface_status() -> bool {
        std::string data;
        if (!read_file(metin2::state::path(INTERFACE_FILE), data)) {
            return false;
        }

        if (data.size() != sizeof(window_status_value_)) {
            metin2::log::warn("{} has {} bytes instead of {}; the default window layout is used", INTERFACE_FILE, data.size(), sizeof(window_status_value_));
            return false;
        }

        memcpy(window_status_value_, data.data(), sizeof(window_status_value_));
        return true;
    }

    void python_system::save_interface_status() {
        if (!interface_handler_) {
            return;
        }

        py_call_class_member_func(interface_handler_, "on_save_interface_status", Py_BuildValue("()"));

        FILE *file;

        file = fopen(metin2::state::path(INTERFACE_FILE).c_str(), "wb");

        if (!file) {
            metin2::log::error("Cannot write {}", INTERFACE_FILE);
            return;
        }

        fwrite(window_status_value_, 1, sizeof(window_status_value_), file);
        fclose(file);
    }

    void python_system::change_system() {
        auto &sound_manager = get_sound_manager();

        sound_manager.set_music_volume(config_value_.music_volume);
        sound_manager.set_sound_volume_grade(config_value_.voice_volume);
    }

    void python_system::clear() {
        set_interface_handler(nullptr);
    }

    python_system::python_system() {
        memset(&config_value_, 0, sizeof(config));

        interface_handler_ = nullptr;

        set_default_config();

        load_config();

        change_system();

        if (load_interface_status()) {
            is_interface_config_ = true;
        } else {
            is_interface_config_ = false;
        }
    }

    python_system::~python_system() {
        assert(interface_handler_ == nullptr && "CPythonSystem MUST CLEAR!");
    }
}
