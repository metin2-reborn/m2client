#include "python_application.h"

#include <metin2/config/state_dir.h>
#include <metin2/core/random.h>
#include <metin2/core/tea.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/image_file.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/actors/game_util.h>

#include <algorithm>
#include <dirent.h>
#include <fnmatch.h>
#include <strings.h>
#include <vector>

#include "locale.h"

namespace metin2::inline client::inline characters {
    extern bool hair_color_enable;
}

namespace metin2::inline client::inline player {
    extern bool skill_effect_upgrade_enable;
}

namespace metin2::inline client::inline social {
    extern void text_tail_set_living_time(int32_t living_time);
}

namespace metin2::inline client::inline characters {
    extern bool use_armor_specular;
}

namespace metin2::inline world::inline actors {
    extern bool use_weapon_specular;
}

namespace metin2::inline client::inline app {
    extern bool perf_checker_render_game;
    extern math::color specular_color;
    bool visible_notice = true;
    bool test_server_flag = false;
    int two_handed_weapon_attack_speed_decrease = 0;

    // The Windows client embedded Internet Explorer for these; there is no browser on Linux, so the page
    // is only logged and the browser always reports itself hidden.
    auto app_show_web_page(PyObject *self, PyObject *args) -> PyObject * {
        char *web_page;
        if (!py_tuple_get_string(args, 0, &web_page)) {
            return py_build_exception();
        }

        metin2::log::info("No embedded web browser; not showing {}", web_page);
        return py_build_none();
    }

    auto app_move_web_page(PyObject *self, PyObject *args) -> PyObject * {
        return py_build_none();
    }

    auto app_hide_web_page(PyObject *self, PyObject *args) -> PyObject * {
        return py_build_none();
    }

    auto app_is_web_page_mode(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", 0);
    }

    auto app_enable_performance_time(PyObject *self, PyObject *args) -> PyObject * {
        char *mode;
        if (!py_tuple_get_string(args, 0, &mode)) {
            return py_build_exception();
        }

        int enable;
        if (!py_tuple_get_integer(args, 1, &enable)) {
            return py_build_exception();
        }

        bool is_enable = enable ? true : false;

        if (strcmp(mode, "RENDER_GAME") == 0) {
            perf_checker_render_game = is_enable;
        }

        return py_build_none();
    }

    extern bool ride_horse_enable;
    extern double specular_spd;

    auto app_set_text_tail_living_time(PyObject *self, PyObject *args) -> PyObject * {
        float living_time;
        if (!py_tuple_get_float(args, 0, &living_time)) {
            return py_build_exception();
        }

        text_tail_set_living_time(living_time * 1000);

        return py_build_none();
    }

    auto app_set_hair_color_enable(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!py_tuple_get_integer(args, 0, &enable)) {
            return py_build_exception();
        }

        hair_color_enable = enable;

        return py_build_none();
    }

    auto app_set_armor_specular_enable(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!py_tuple_get_integer(args, 0, &enable)) {
            return py_build_exception();
        }

        use_armor_specular = enable;

        return py_build_none();
    }

    auto app_set_weapon_specular_enable(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!py_tuple_get_integer(args, 0, &enable)) {
            return py_build_exception();
        }

        use_weapon_specular = enable;

        return py_build_none();
    }

    auto app_set_skill_effect_upgrade_enable(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!py_tuple_get_integer(args, 0, &enable)) {
            return py_build_exception();
        }

        skill_effect_upgrade_enable = enable;

        return py_build_none();
    }

    auto set_two_handed_weapon_att_speed_decrease_value(PyObject *self, PyObject *args) -> PyObject * {
        int value;
        if (!py_tuple_get_integer(args, 0, &value)) {
            return py_build_exception();
        }

        two_handed_weapon_attack_speed_decrease = value;

        return py_build_none();
    }

    auto app_set_camera_max_distance(PyObject *self, PyObject *args) -> PyObject * {
        float max_value;
        if (!py_tuple_get_float(args, 0, &max_value)) {
            return py_build_exception();
        }

        camera::set_camera_max_distance(max_value);
        return py_build_none();
    }

    // Set the x87 FPU to 24-bit precision on Windows; x86-64 does its float math in SSE, so nothing to do.
    auto app_set_control_fp(PyObject *self, PyObject *args) -> PyObject * {
        return py_build_none();
    }

    auto app_set_min_fog(PyObject *self, PyObject *args) -> PyObject * {
        float min_fog;
        if (!py_tuple_get_float(args, 0, &min_fog)) {
            return py_build_exception();
        }

        get_python_application().set_min_fog(min_fog);
        return py_build_none();
    }

    auto app_set_frame_skip(PyObject *self, PyObject *args) -> PyObject * {
        int frame_skip;
        if (!py_tuple_get_integer(args, 0, &frame_skip)) {
            return py_build_exception();
        }

        get_python_application().set_frame_skip(frame_skip ? true : false);
        return py_build_none();
    }

    auto load_locale_data() -> bool;

    auto app_load_locale_data(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", load_locale_data());
    }

    auto app_get_state_path(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name)) {
            return py_build_exception();
        }

        return Py_BuildValue("s", metin2::state::path(name).c_str());
    }

    auto app_get_image_info(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name)) {
            return py_build_exception();
        }

        bgra_image image;
        const bool can_load = load_image_file(file_name, image);
        return Py_BuildValue("iii", can_load, image.width, image.height);
    }

    auto app_is_exist_file(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name)) {
            return py_build_exception();
        }

        bool is_exist = get_file_manager().is_exist(file_name);

        return Py_BuildValue("i", is_exist);
    }

    auto app_get_file_list(PyObject *self, PyObject *args) -> PyObject * {
        char *sz_filter;
        if (!py_tuple_get_string(args, 0, &sz_filter)) {
            return py_build_exception();
        }

        PyObject *list = PyList_New(0);

        // Asset folders (BGM/*.mp3) are listed through the file manager. upload/ holds the player's
        // own guild mark images, which the mark uploader reads from disk, so it is listed there.
        std::string st_filter(sz_filter);
        std::ranges::replace(st_filter, '\\', '/');
        if (strncasecmp(st_filter.c_str(), "upload/", 7) != 0) {
            auto asset_path = eter_file_manager::parse_path(st_filter);
            std::string::size_type slash = asset_path.path.rfind('/');
            std::string folder = slash == std::string::npos ? std::string() : asset_path.path.substr(0, slash + 1);
            std::string mask = asset_path.path.substr(folder.size());

            std::vector<std::string> names_value;
            get_file_manager().for_each(asset_path.archive_value, folder, [&](const std::string &c_rst_name) -> void {
                std::string st_name = c_rst_name.substr(folder.size());
                if (st_name.find('/') == std::string::npos && fnmatch(mask.c_str(), st_name.c_str(), FNM_CASEFOLD) == 0) {
                    names_value.push_back(st_name);
                }
            });

            std::ranges::sort(names_value);

            for (const std::string &name : names_value) {
                PyObject *file_name = PyString_FromString(name.c_str());
                PyList_Append(list, file_name);
                Py_DECREF(file_name);
            }
            return list;
        }

        const std::string::size_type slash = st_filter.rfind('/');
        const std::string folder = st_filter.substr(0, slash);
        const std::string mask = st_filter.substr(slash + 1);

        if (DIR *dir = opendir(folder.c_str())) {
            while (const dirent *entry = readdir(dir)) {
                if (fnmatch(mask.c_str(), entry->d_name, FNM_CASEFOLD) == 0) {
                    PyObject *file_name = PyString_FromString(entry->d_name);
                    PyList_Append(list, file_name);
                    Py_DECREF(file_name);
                }
            }

            closedir(dir);
        }

        return list;
    }

    auto app_update_game(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().update_game();
        return py_build_none();
    }

    auto app_render_game(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().render_game();
        return py_build_none();
    }

    auto app_set_mouse_handler(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handler;
        if (!py_tuple_get_object(args, 0, &handler)) {
            return py_build_exception();
        }

        get_python_application().set_mouse_handler(handler);
        return py_build_none();
    }

    auto app_create(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name)) {
            return py_build_exception();
        }

        int width_value;
        if (!py_tuple_get_integer(args, 1, &width_value)) {
            return py_build_exception();
        }

        int height;
        if (!py_tuple_get_integer(args, 2, &height)) {
            return py_build_exception();
        }

        int windowed;
        if (!py_tuple_get_integer(args, 3, &windowed)) {
            return py_build_exception();
        }

        auto &app = get_python_application();
        if (!app.create(self, name, width_value, height, windowed)) {
            return nullptr;
        }

        return py_build_none();
    }

    auto app_loop(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().run();
        return py_build_none();
    }

    auto app_get_info(PyObject *self, PyObject *args) -> PyObject * {
        int n_info;
        if (!py_tuple_get_integer(args, 0, &n_info)) {
            return py_build_exception();
        }

        std::string st_info;
        get_python_application().get_info(n_info, &st_info);
        return Py_BuildValue("s", st_info.c_str());
    }

    auto app_abort(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().abort();
        return py_build_none();
    }

    auto app_exit(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().exit();
        return py_build_none();
    }

    auto app_set_camera(PyObject *self, PyObject *args) -> PyObject * {
        float distance, pitch, rotation, destination_height;
        if (!py_tuple_get_float(args, 0, &distance) ||
            !py_tuple_get_float(args, 1, &pitch) ||
            !py_tuple_get_float(args, 2, &rotation) ||
            !py_tuple_get_float(args, 3, &destination_height)) {
            return py_build_exception();
        }

        get_python_application().set_camera(distance, pitch, rotation, destination_height);
        return py_build_none();
    }

    auto app_get_camera(PyObject *self, PyObject *args) -> PyObject * {
        float distance, pitch, rotation, destination_height;
        get_python_application().get_camera(&distance, &pitch, &rotation, &destination_height);

        return Py_BuildValue("ffff", distance, pitch, rotation, destination_height);
    }

    auto app_get_camera_pitch(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("f", get_python_application().get_pitch());
    }

    auto app_get_time(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("f", get_python_application().global_time());
    }

    auto app_get_global_time(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_application().server_time());
    }

    auto app_get_global_time_stamp(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_application().server_time_stamp());
    }

    auto app_get_update_fps(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_application().update_fps());
    }

    auto app_get_render_fps(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_application().render_fps());
    }

    auto app_rotate_camera(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!py_tuple_get_integer(args, 0, &direction)) {
            return py_build_exception();
        }

        get_python_application().rotate_camera(direction);
        return py_build_none();
    }

    auto app_pitch_camera(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!py_tuple_get_integer(args, 0, &direction)) {
            return py_build_exception();
        }

        get_python_application().pitch_camera(direction);
        return py_build_none();
    }

    auto app_zoom_camera(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!py_tuple_get_integer(args, 0, &direction)) {
            return py_build_exception();
        }

        get_python_application().zoom_camera(direction);
        return py_build_none();
    }

    auto app_movie_rotate_camera(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!py_tuple_get_integer(args, 0, &direction)) {
            return py_build_exception();
        }

        get_python_application().movie_rotate_camera(direction);
        return py_build_none();
    }

    auto app_movie_pitch_camera(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!py_tuple_get_integer(args, 0, &direction)) {
            return py_build_exception();
        }

        get_python_application().movie_pitch_camera(direction);
        return py_build_none();
    }

    auto app_movie_zoom_camera(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!py_tuple_get_integer(args, 0, &direction)) {
            return py_build_exception();
        }

        get_python_application().movie_zoom_camera(direction);
        return py_build_none();
    }

    auto app_movie_reset_camera(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().movie_reset_camera();
        return py_build_none();
    }

    auto app_get_face_speed(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("f", get_python_application().face_speed());
    }

    auto app_get_render_time(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("fi",
            get_python_application().ave_render_time(),
            get_python_application().cur_render_time());
    }

    auto app_get_update_time(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_application().cur_update_time());
    }

    auto app_get_face_count(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_application().face_count());
    }

    auto app_get_avaiable_texture_mememory(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", graphic_base::available_texture_memory());
    }

    auto app_set_global_center_position(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!py_tuple_get_integer(args, 0, &x) ||
            !py_tuple_get_integer(args, 1, &y)) {
            return py_build_exception();
        }

        get_python_application().set_global_center_position(x, y);
        return py_build_none();
    }

    auto app_set_center_position(PyObject *self, PyObject *args) -> PyObject * {
        float fx, fy, fz;
        if (!py_tuple_get_float(args, 0, &fx) ||
            !py_tuple_get_float(args, 1, &fy) ||
            !py_tuple_get_float(args, 2, &fz)) {
            return py_build_exception();
        }

        get_python_application().set_center_position(fx, -fy, fz);
        return py_build_none();
    }

    auto app_get_cursor_position(PyObject *self, PyObject *args) -> PyObject * {
        int32_t lx, ly;

        widgets::get_window_manager().mouse_position(lx, ly);

        return Py_BuildValue("ii", lx, ly);
    }

    auto app_is_pressed(PyObject *self, PyObject *args) -> PyObject * {
        int key;
        if (!py_tuple_get_integer(args, 0, &key)) {
            return py_build_exception();
        }

        return Py_BuildValue("i", get_python_application().is_pressed(key));
    }

    auto app_set_cursor(PyObject *self, PyObject *args) -> PyObject * {
        int cursor_num;
        if (!py_tuple_get_integer(args, 0, &cursor_num)) {
            return py_build_exception();
        }

        if (!get_python_application().set_cursor_num(cursor_num)) {
            return py_build_exception("Wrong Cursor Name [%d]", cursor_num);
        }

        return py_build_none();
    }

    auto app_get_cursor(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_application().cursor_num());
    }

    auto app_show_cursor(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().set_cursor_visible(true);
        return py_build_none();
    }

    auto app_hide_cursor(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().set_cursor_visible(false);
        return py_build_none();
    }

    auto app_is_show_cursor(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", true == get_python_application().cursor_visible());
    }

    auto app_is_liar_cursor_on(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", true == get_python_application().liar_cursor_on());
    }

    auto app_get_random(PyObject *self, PyObject *args) -> PyObject * {
        int from, to;
        if (!py_tuple_get_integer(args, 0, &from) ||
            !py_tuple_get_integer(args, 1, &to)) {
            return py_build_exception();
        }

        if (from > to) {
            int tmp = from;
            from = to;
            to = tmp;
        }

        return Py_BuildValue("i", metin2::random::number(from, to));
    }

    auto app_get_rotating_direction(PyObject *self, PyObject *args) -> PyObject * {
        float source, target;
        if (!py_tuple_get_float(args, 0, &source) ||
            !py_tuple_get_float(args, 1, &target)) {
            return py_build_exception();
        }

        return Py_BuildValue("i", get_rotating_direction(source, target));
    }

    auto app_get_degree_difference(PyObject *self, PyObject *args) -> PyObject * {
        float source, target;
        if (!py_tuple_get_float(args, 0, &source) ||
            !py_tuple_get_float(args, 1, &target)) {
            return py_build_exception();
        }

        return Py_BuildValue("f", get_degree_difference(source, target));
    }

    auto app_set_default_font_name(PyObject *self, PyObject *args) -> PyObject * {
        char *font_name;
        if (!py_tuple_get_string(args, 0, &font_name)) {
            return py_build_exception();
        }

        default_font_set_name(font_name);
        return py_build_none();
    }

    auto app_set_guild_symbol_path(PyObject *self, PyObject *args) -> PyObject * {
        char *path_name;
        if (!py_tuple_get_string(args, 0, &path_name)) {
            return py_build_exception();
        }

        set_guild_symbol_path(path_name);
        return py_build_none();
    }

    auto app_enable_special_camera_mode(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().enable_special_camera_mode();
        return py_build_none();
    }

    auto app_set_camera_speed(PyObject *self, PyObject *args) -> PyObject * {
        int percentage;
        if (!py_tuple_get_integer(args, 0, &percentage)) {
            return py_build_exception();
        }

        get_python_application().set_camera_speed(percentage);

        auto *camera = get_camera_manager().current_camera();
        if (camera) {
            camera->set_resistance(float(percentage) / 100.0f);
        }

        return py_build_none();
    }

    auto app_set_camera_setting(PyObject *self, PyObject *args) -> PyObject * {
        int ix, iy, iz;
        if (!py_tuple_get_integer(args, 0, &ix) ||
            !py_tuple_get_integer(args, 1, &iy) ||
            !py_tuple_get_integer(args, 2, &iz)) {
            return py_build_exception();
        }

        int zoom, rotation, pitch;
        if (!py_tuple_get_integer(args, 3, &zoom) ||
            !py_tuple_get_integer(args, 4, &rotation) ||
            !py_tuple_get_integer(args, 5, &pitch)) {
            return py_build_exception();
        }

        python_application::camera_setting camera_setting;

        memset(static_cast<void *>(&camera_setting), 0, sizeof(camera_setting));
        camera_setting.center_position.x = float(ix);
        camera_setting.center_position.y = float(iy);
        camera_setting.center_position.z = float(iz);
        camera_setting.zoom = float(zoom);
        camera_setting.rotation = float(rotation);
        camera_setting.pitch = float(pitch);

        get_python_application().set_event_camera(camera_setting);

        return py_build_none();
    }

    auto app_save_camera_setting(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name)) {
            return py_build_exception();
        }

        get_python_application().save_camera_setting(file_name);
        return py_build_none();
    }

    auto app_load_camera_setting(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name)) {
            return py_build_exception();
        }

        bool result = get_python_application().load_camera_setting(file_name);
        return Py_BuildValue("i", result);
    }

    auto app_set_default_camera(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().set_default_camera();
        return py_build_none();
    }

    auto app_set_sight_range(PyObject *self, PyObject *args) -> PyObject * {
        int range;
        if (!py_tuple_get_integer(args, 0, &range)) {
            return py_build_exception();
        }

        get_python_application().set_force_sight_range(range);
        return py_build_none();
    }

    extern int accumulation_time;

    auto app_set_visible_notice(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!py_tuple_get_integer(args, 0, &flag)) {
            return py_build_exception();
        }

        visible_notice = flag;
        return py_build_none();
    }

    auto app_is_visible_notice(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", visible_notice);
    }

    auto app_enable_test_server_flag(PyObject *self, PyObject *args) -> PyObject * {
        test_server_flag = true;
        return py_build_none();
    }

    auto app_is_enable_test_server_flag(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", test_server_flag);
    }

    class text_line_loader {
      public:
        text_line_loader(const char *file_name) {
            const void *data;
            mapped_file file;

            if (!get_file_manager().get(file, file_name, &data)) {
                return;
            }

            text_file_loader_value_.bind(file.size(), data);
        }

        auto line_count() -> uint32_t {
            return text_file_loader_value_.line_count();
        }

        auto get_line(uint32_t index) -> const char * {
            if (index >= line_count())
                return "";

            return text_file_loader_value_.line_string(index).c_str();
        }

      protected:
        memory_text_file_loader text_file_loader_value_;
    };

    auto app_open_text_file(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name)) {
            return py_build_exception();
        }

        auto *text_line_loader_value = new text_line_loader(file_name);

        return Py_BuildValue("n", (Py_ssize_t)text_line_loader_value);
    }

    auto app_close_text_file(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle)) {
            return py_build_exception();
        }

        auto *text_file_loader = (text_line_loader *)handle;
        delete text_file_loader;

        return py_build_none();
    }

    auto app_get_text_file_line_count(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle)) {
            return py_build_exception();
        }

        auto *text_file_loader = (text_line_loader *)handle;
        return Py_BuildValue("i", text_file_loader->line_count());
    }

    auto app_get_text_file_line(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle)) {
            return py_build_exception();
        }

        int line_index;
        if (!py_tuple_get_integer(args, 1, &line_index)) {
            return py_build_exception();
        }

        auto *text_file_loader = (text_line_loader *)handle;
        return Py_BuildValue("s", text_file_loader->get_line(line_index));
    }

    auto app_set_guild_mark_path(PyObject *self, PyObject *args) -> PyObject * {
        char *path;
        if (!py_tuple_get_string(args, 0, &path)) {
            return py_build_exception();
        }

        std::string new_path = path;
        if (const size_t ext = new_path.find(".tga"); ext != std::string::npos) {
            new_path.resize(ext);
        }

        get_guild_mark_manager().set_mark_path_prefix(new_path.c_str());
        return py_build_none();
    }

    auto app_is_dev_stage(PyObject *self, PyObject *args) -> PyObject * {
        int is_development_stage = 0;
        return Py_BuildValue("i", is_development_stage);
    }

    auto app_logo_open(PyObject *self, PyObject *args) -> PyObject * {
        // The logo movies are not shipped and there is no movie player: report success so the
        // logo phase continues, then OnLogoUpdate reports the (empty) movie as finished.
        char *name;
        if (!py_tuple_get_string(args, 0, &name)) {
            return py_build_exception();
        }

        return Py_BuildValue("i", 1);
    }

    auto app_logo_update(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", 0);
    }

    auto app_logo_render(PyObject *self, PyObject *args) -> PyObject * {
        return py_build_none();
    }

    auto app_logo_close(PyObject *self, PyObject *args) -> PyObject * {
        return py_build_none();
    }

    void init_app_module() {
        static PyMethodDef methods[] = {
            {"IsDevStage", app_is_dev_stage, METH_VARARGS},

            {"SetTextTailLivingTime", app_set_text_tail_living_time, METH_VARARGS},

            {"EnablePerformanceTime", app_enable_performance_time, METH_VARARGS},
            {"SetHairColorEnable", app_set_hair_color_enable, METH_VARARGS},

            {"SetArmorSpecularEnable", app_set_armor_specular_enable, METH_VARARGS},
            {"SetWeaponSpecularEnable", app_set_weapon_specular_enable, METH_VARARGS},
            {"SetSkillEffectUpgradeEnable", app_set_skill_effect_upgrade_enable, METH_VARARGS},
            {"SetTwoHandedWeaponAttSpeedDecreaseValue", set_two_handed_weapon_att_speed_decrease_value, METH_VARARGS},

            {"SetCameraMaxDistance", app_set_camera_max_distance, METH_VARARGS},
            {"SetMinFog", app_set_min_fog, METH_VARARGS},
            {"SetFrameSkip", app_set_frame_skip, METH_VARARGS},
            {"GetImageInfo", app_get_image_info, METH_VARARGS},
            {"GetInfo", app_get_info, METH_VARARGS},
            {"UpdateGame", app_update_game, METH_VARARGS},
            {"RenderGame", app_render_game, METH_VARARGS},
            {"Loop", app_loop, METH_VARARGS},
            {"Create", app_create, METH_VARARGS},
            {"Exit", app_exit, METH_VARARGS},
            {"Abort", app_abort, METH_VARARGS},
            {"SetMouseHandler", app_set_mouse_handler, METH_VARARGS},
            {"IsExistFile", app_is_exist_file, METH_VARARGS},
            {"GetFileList", app_get_file_list, METH_VARARGS},

            {"SetCamera", app_set_camera, METH_VARARGS},
            {"GetCamera", app_get_camera, METH_VARARGS},
            {"GetCameraPitch", app_get_camera_pitch, METH_VARARGS},
            {"GetTime", app_get_time, METH_VARARGS},
            {"GetGlobalTime", app_get_global_time, METH_VARARGS},
            {"GetGlobalTimeStamp", app_get_global_time_stamp, METH_VARARGS},
            {"GetUpdateFPS", app_get_update_fps, METH_VARARGS},
            {"GetRenderFPS", app_get_render_fps, METH_VARARGS},
            {"RotateCamera", app_rotate_camera, METH_VARARGS},
            {"PitchCamera", app_pitch_camera, METH_VARARGS},
            {"ZoomCamera", app_zoom_camera, METH_VARARGS},
            {"MovieRotateCamera", app_movie_rotate_camera, METH_VARARGS},
            {"MoviePitchCamera", app_movie_pitch_camera, METH_VARARGS},
            {"MovieZoomCamera", app_movie_zoom_camera, METH_VARARGS},
            {"MovieResetCamera", app_movie_reset_camera, METH_VARARGS},

            {"GetAvailableTextureMemory", app_get_avaiable_texture_mememory, METH_VARARGS},
            {"GetRenderTime", app_get_render_time, METH_VARARGS},
            {"GetUpdateTime", app_get_update_time, METH_VARARGS},
            {"GetFaceSpeed", app_get_face_speed, METH_VARARGS},
            {"GetFaceCount", app_get_face_count, METH_VARARGS},
            {"SetGlobalCenterPosition", app_set_global_center_position, METH_VARARGS},
            {"SetCenterPosition", app_set_center_position, METH_VARARGS},
            {"GetCursorPosition", app_get_cursor_position, METH_VARARGS},

            {"GetRandom", app_get_random, METH_VARARGS},
            {"IsWebPageMode", app_is_web_page_mode, METH_VARARGS},
            {"ShowWebPage", app_show_web_page, METH_VARARGS},
            {"MoveWebPage", app_move_web_page, METH_VARARGS},
            {"HideWebPage", app_hide_web_page, METH_VARARGS},
            {"IsPressed", app_is_pressed, METH_VARARGS},
            {"SetCursor", app_set_cursor, METH_VARARGS},
            {"GetCursor", app_get_cursor, METH_VARARGS},
            {"ShowCursor", app_show_cursor, METH_VARARGS},
            {"HideCursor", app_hide_cursor, METH_VARARGS},
            {"IsShowCursor", app_is_show_cursor, METH_VARARGS},
            {"IsLiarCursorOn", app_is_liar_cursor_on, METH_VARARGS},

            {"GetRotatingDirection", app_get_rotating_direction, METH_VARARGS},
            {"GetDegreeDifference", app_get_degree_difference, METH_VARARGS},
            {"SetDefaultFontName", app_set_default_font_name, METH_VARARGS},
            {"SetGuildSymbolPath", app_set_guild_symbol_path, METH_VARARGS},

            {"EnableSpecialCameraMode", app_enable_special_camera_mode, METH_VARARGS},
            {"SetCameraSpeed", app_set_camera_speed, METH_VARARGS},

            {"SaveCameraSetting", app_save_camera_setting, METH_VARARGS},
            {"LoadCameraSetting", app_load_camera_setting, METH_VARARGS},
            {"SetDefaultCamera", app_set_default_camera, METH_VARARGS},
            {"SetCameraSetting", app_set_camera_setting, METH_VARARGS},

            {"SetSightRange", app_set_sight_range, METH_VARARGS},

            {"OpenTextFile", app_open_text_file, METH_VARARGS},
            {"CloseTextFile", app_close_text_file, METH_VARARGS},
            {"GetTextFileLineCount", app_get_text_file_line_count, METH_VARARGS},
            {"GetTextFileLine", app_get_text_file_line, METH_VARARGS},

            {"GetStatePath", app_get_state_path, METH_VARARGS},

            {"LoadLocaleData", app_load_locale_data, METH_NOARGS},

            {"SetControlFP", app_set_control_fp, METH_VARARGS},

            {"SetVisibleNotice", app_set_visible_notice, METH_VARARGS},
            {"IsVisibleNotice", app_is_visible_notice, METH_VARARGS},
            {"EnableTestServerFlag", app_enable_test_server_flag, METH_VARARGS},
            {"IsEnableTestServerFlag", app_is_enable_test_server_flag, METH_VARARGS},

            {"SetGuildMarkPath", app_set_guild_mark_path, METH_VARARGS},

            {"OnLogoUpdate", app_logo_update, METH_VARARGS},
            {"OnLogoRender", app_logo_render, METH_VARARGS},
            {"OnLogoOpen", app_logo_open, METH_VARARGS},
            {"OnLogoClose", app_logo_close, METH_VARARGS},

            {nullptr, nullptr},
        };

        PyObject *module_value = init_python_module("metin2.app", methods);

        PyModule_AddIntConstant(module_value, "INFO_ITEM", python_application::INFO_ITEM);
        PyModule_AddIntConstant(module_value, "INFO_ACTOR", python_application::INFO_ACTOR);
        PyModule_AddIntConstant(module_value, "INFO_EFFECT", python_application::INFO_EFFECT);
        PyModule_AddIntConstant(module_value, "INFO_TEXTTAIL", python_application::INFO_TEXTTAIL);

        PyModule_AddIntConstant(module_value, "DEGREE_DIRECTION_SAME", DEGREE_DIRECTION_SAME);
        PyModule_AddIntConstant(module_value, "DEGREE_DIRECTION_RIGHT", DEGREE_DIRECTION_RIGHT);
        PyModule_AddIntConstant(module_value, "DEGREE_DIRECTION_LEFT", DEGREE_DIRECTION_LEFT);

        PyModule_AddIntConstant(module_value, "VK_LEFT", VK_LEFT);
        PyModule_AddIntConstant(module_value, "VK_RIGHT", VK_RIGHT);
        PyModule_AddIntConstant(module_value, "VK_UP", VK_UP);
        PyModule_AddIntConstant(module_value, "VK_DOWN", VK_DOWN);
        PyModule_AddIntConstant(module_value, "VK_HOME", VK_HOME);
        PyModule_AddIntConstant(module_value, "VK_END", VK_END);
        PyModule_AddIntConstant(module_value, "VK_DELETE", VK_DELETE);

        PyModule_AddIntConstant(module_value, "DIK_ESCAPE", DIK_ESCAPE);
        PyModule_AddIntConstant(module_value, "DIK_ESC", DIK_ESCAPE);
        PyModule_AddIntConstant(module_value, "DIK_1", DIK_1);
        PyModule_AddIntConstant(module_value, "DIK_2", DIK_2);
        PyModule_AddIntConstant(module_value, "DIK_3", DIK_3);
        PyModule_AddIntConstant(module_value, "DIK_4", DIK_4);
        PyModule_AddIntConstant(module_value, "DIK_5", DIK_5);
        PyModule_AddIntConstant(module_value, "DIK_6", DIK_6);
        PyModule_AddIntConstant(module_value, "DIK_7", DIK_7);
        PyModule_AddIntConstant(module_value, "DIK_8", DIK_8);
        PyModule_AddIntConstant(module_value, "DIK_9", DIK_9);
        PyModule_AddIntConstant(module_value, "DIK_0", DIK_0);
        PyModule_AddIntConstant(module_value, "DIK_MINUS", DIK_MINUS);
        PyModule_AddIntConstant(module_value, "DIK_EQUALS", DIK_EQUALS);
        PyModule_AddIntConstant(module_value, "DIK_BACK", DIK_BACK);
        PyModule_AddIntConstant(module_value, "DIK_TAB", DIK_TAB);
        PyModule_AddIntConstant(module_value, "DIK_Q", DIK_Q);
        PyModule_AddIntConstant(module_value, "DIK_W", DIK_W);
        PyModule_AddIntConstant(module_value, "DIK_E", DIK_E);
        PyModule_AddIntConstant(module_value, "DIK_R", DIK_R);
        PyModule_AddIntConstant(module_value, "DIK_T", DIK_T);
        PyModule_AddIntConstant(module_value, "DIK_Y", DIK_Y);
        PyModule_AddIntConstant(module_value, "DIK_U", DIK_U);
        PyModule_AddIntConstant(module_value, "DIK_I", DIK_I);
        PyModule_AddIntConstant(module_value, "DIK_O", DIK_O);
        PyModule_AddIntConstant(module_value, "DIK_P", DIK_P);
        PyModule_AddIntConstant(module_value, "DIK_LBRACKET", DIK_LBRACKET);
        PyModule_AddIntConstant(module_value, "DIK_RBRACKET", DIK_RBRACKET);
        PyModule_AddIntConstant(module_value, "DIK_RETURN", DIK_RETURN);
        PyModule_AddIntConstant(module_value, "DIK_LCONTROL", DIK_LCONTROL);
        PyModule_AddIntConstant(module_value, "DIK_A", DIK_A);
        PyModule_AddIntConstant(module_value, "DIK_S", DIK_S);
        PyModule_AddIntConstant(module_value, "DIK_D", DIK_D);
        PyModule_AddIntConstant(module_value, "DIK_F", DIK_F);
        PyModule_AddIntConstant(module_value, "DIK_G", DIK_G);
        PyModule_AddIntConstant(module_value, "DIK_H", DIK_H);
        PyModule_AddIntConstant(module_value, "DIK_J", DIK_J);
        PyModule_AddIntConstant(module_value, "DIK_K", DIK_K);
        PyModule_AddIntConstant(module_value, "DIK_L", DIK_L);
        PyModule_AddIntConstant(module_value, "DIK_SEMICOLON", DIK_SEMICOLON);
        PyModule_AddIntConstant(module_value, "DIK_APOSTROPHE", DIK_APOSTROPHE);
        PyModule_AddIntConstant(module_value, "DIK_GRAVE", DIK_GRAVE);
        PyModule_AddIntConstant(module_value, "DIK_LSHIFT", DIK_LSHIFT);
        PyModule_AddIntConstant(module_value, "DIK_BACKSLASH", DIK_BACKSLASH);
        PyModule_AddIntConstant(module_value, "DIK_Z", DIK_Z);
        PyModule_AddIntConstant(module_value, "DIK_X", DIK_X);
        PyModule_AddIntConstant(module_value, "DIK_C", DIK_C);
        PyModule_AddIntConstant(module_value, "DIK_V", DIK_V);
        PyModule_AddIntConstant(module_value, "DIK_B", DIK_B);
        PyModule_AddIntConstant(module_value, "DIK_N", DIK_N);
        PyModule_AddIntConstant(module_value, "DIK_M", DIK_M);
        PyModule_AddIntConstant(module_value, "DIK_COMMA", DIK_COMMA);
        PyModule_AddIntConstant(module_value, "DIK_PERIOD", DIK_PERIOD);
        PyModule_AddIntConstant(module_value, "DIK_SLASH", DIK_SLASH);
        PyModule_AddIntConstant(module_value, "DIK_RSHIFT", DIK_RSHIFT);
        PyModule_AddIntConstant(module_value, "DIK_MULTIPLY", DIK_MULTIPLY);
        PyModule_AddIntConstant(module_value, "DIK_LALT", DIK_LMENU);
        PyModule_AddIntConstant(module_value, "DIK_SPACE", DIK_SPACE);
        PyModule_AddIntConstant(module_value, "DIK_CAPITAL", DIK_CAPITAL);
        PyModule_AddIntConstant(module_value, "DIK_F1", DIK_F1);
        PyModule_AddIntConstant(module_value, "DIK_F2", DIK_F2);
        PyModule_AddIntConstant(module_value, "DIK_F3", DIK_F3);
        PyModule_AddIntConstant(module_value, "DIK_F4", DIK_F4);
        PyModule_AddIntConstant(module_value, "DIK_F5", DIK_F5);
        PyModule_AddIntConstant(module_value, "DIK_F6", DIK_F6);
        PyModule_AddIntConstant(module_value, "DIK_F7", DIK_F7);
        PyModule_AddIntConstant(module_value, "DIK_F8", DIK_F8);
        PyModule_AddIntConstant(module_value, "DIK_F9", DIK_F9);
        PyModule_AddIntConstant(module_value, "DIK_F10", DIK_F10);
        PyModule_AddIntConstant(module_value, "DIK_NUMLOCK", DIK_NUMLOCK);
        PyModule_AddIntConstant(module_value, "DIK_SCROLL", DIK_SCROLL);
        PyModule_AddIntConstant(module_value, "DIK_NUMPAD7", DIK_NUMPAD7);
        PyModule_AddIntConstant(module_value, "DIK_NUMPAD8", DIK_NUMPAD8);
        PyModule_AddIntConstant(module_value, "DIK_NUMPAD9", DIK_NUMPAD9);
        PyModule_AddIntConstant(module_value, "DIK_SUBTRACT", DIK_SUBTRACT);
        PyModule_AddIntConstant(module_value, "DIK_NUMPAD4", DIK_NUMPAD4);
        PyModule_AddIntConstant(module_value, "DIK_NUMPAD5", DIK_NUMPAD5);
        PyModule_AddIntConstant(module_value, "DIK_NUMPAD6", DIK_NUMPAD6);
        PyModule_AddIntConstant(module_value, "DIK_ADD", DIK_ADD);
        PyModule_AddIntConstant(module_value, "DIK_NUMPAD1", DIK_NUMPAD1);
        PyModule_AddIntConstant(module_value, "DIK_NUMPAD2", DIK_NUMPAD2);
        PyModule_AddIntConstant(module_value, "DIK_NUMPAD3", DIK_NUMPAD3);
        PyModule_AddIntConstant(module_value, "DIK_NUMPAD0", DIK_NUMPAD0);
        PyModule_AddIntConstant(module_value, "DIK_DECIMAL", DIK_DECIMAL);
        PyModule_AddIntConstant(module_value, "DIK_F11", DIK_F11);
        PyModule_AddIntConstant(module_value, "DIK_F12", DIK_F12);
        PyModule_AddIntConstant(module_value, "DIK_NEXTTRACK", DIK_NEXTTRACK);
        PyModule_AddIntConstant(module_value, "DIK_NUMPADENTER", DIK_NUMPADENTER);
        PyModule_AddIntConstant(module_value, "DIK_RCONTROL", DIK_RCONTROL);
        PyModule_AddIntConstant(module_value, "DIK_MUTE", DIK_MUTE);
        PyModule_AddIntConstant(module_value, "DIK_CALCULATOR", DIK_CALCULATOR);
        PyModule_AddIntConstant(module_value, "DIK_PLAYPAUSE", DIK_PLAYPAUSE);
        PyModule_AddIntConstant(module_value, "DIK_MEDIASTOP", DIK_MEDIASTOP);
        PyModule_AddIntConstant(module_value, "DIK_VOLUMEDOWN", DIK_VOLUMEDOWN);
        PyModule_AddIntConstant(module_value, "DIK_VOLUMEUP", DIK_VOLUMEUP);
        PyModule_AddIntConstant(module_value, "DIK_WEBHOME", DIK_WEBHOME);
        PyModule_AddIntConstant(module_value, "DIK_NUMPADCOMMA", DIK_NUMPADCOMMA);
        PyModule_AddIntConstant(module_value, "DIK_DIVIDE", DIK_DIVIDE);
        PyModule_AddIntConstant(module_value, "DIK_SYSRQ", DIK_SYSRQ);
        PyModule_AddIntConstant(module_value, "DIK_RALT", DIK_RMENU);
        PyModule_AddIntConstant(module_value, "DIK_PAUSE", DIK_PAUSE);
        PyModule_AddIntConstant(module_value, "DIK_HOME", DIK_HOME);
        PyModule_AddIntConstant(module_value, "DIK_UP", DIK_UP);
        PyModule_AddIntConstant(module_value, "DIK_PGUP", DIK_PRIOR);
        PyModule_AddIntConstant(module_value, "DIK_LEFT", DIK_LEFT);
        PyModule_AddIntConstant(module_value, "DIK_RIGHT", DIK_RIGHT);
        PyModule_AddIntConstant(module_value, "DIK_END", DIK_END);
        PyModule_AddIntConstant(module_value, "DIK_DOWN", DIK_DOWN);
        PyModule_AddIntConstant(module_value, "DIK_PGDN", DIK_NEXT);
        PyModule_AddIntConstant(module_value, "DIK_INSERT", DIK_INSERT);
        PyModule_AddIntConstant(module_value, "DIK_DELETE", DIK_DELETE);
        PyModule_AddIntConstant(module_value, "DIK_LWIN", DIK_LWIN);
        PyModule_AddIntConstant(module_value, "DIK_RWIN", DIK_RWIN);
        PyModule_AddIntConstant(module_value, "DIK_APPS", DIK_APPS);

        // Cursor shapes
        PyModule_AddIntConstant(module_value, "NORMAL", python_application::CURSOR_SHAPE_NORMAL);
        PyModule_AddIntConstant(module_value, "ATTACK", python_application::CURSOR_SHAPE_ATTACK);
        PyModule_AddIntConstant(module_value, "TARGET", python_application::CURSOR_SHAPE_TARGET);
        PyModule_AddIntConstant(module_value, "TALK", python_application::CURSOR_SHAPE_TALK);
        PyModule_AddIntConstant(module_value, "CANT_GO", python_application::CURSOR_SHAPE_CANT_GO);
        PyModule_AddIntConstant(module_value, "PICK", python_application::CURSOR_SHAPE_PICK);

        PyModule_AddIntConstant(module_value, "DOOR", python_application::CURSOR_SHAPE_DOOR);
        PyModule_AddIntConstant(module_value, "CHAIR", python_application::CURSOR_SHAPE_CHAIR);
        PyModule_AddIntConstant(module_value, "MAGIC", python_application::CURSOR_SHAPE_MAGIC);
        PyModule_AddIntConstant(module_value, "BUY", python_application::CURSOR_SHAPE_BUY);
        PyModule_AddIntConstant(module_value, "SELL", python_application::CURSOR_SHAPE_SELL);

        PyModule_AddIntConstant(module_value, "CAMERA_ROTATE", python_application::CURSOR_SHAPE_CAMERA_ROTATE);
        PyModule_AddIntConstant(module_value, "HSIZE", python_application::CURSOR_SHAPE_HSIZE);
        PyModule_AddIntConstant(module_value, "VSIZE", python_application::CURSOR_SHAPE_VSIZE);
        PyModule_AddIntConstant(module_value, "HVSIZE", python_application::CURSOR_SHAPE_HVSIZE);

        PyModule_AddIntConstant(module_value, "CAMERA_TO_POSITIVE", python_application::CAMERA_TO_POSITIVE);
        PyModule_AddIntConstant(module_value, "CAMERA_TO_NEGATIVE", python_application::CAMERA_TO_NEGITIVE);
        PyModule_AddIntConstant(module_value, "CAMERA_STOP", python_application::CAMERA_STOP);

        PyModule_AddIntConstant(module_value, "ENABLE_COSTUME_SYSTEM", 1);

        PyModule_AddIntConstant(module_value, "ENABLE_ENERGY_SYSTEM", 1);

        PyModule_AddIntConstant(module_value, "ENABLE_DRAGON_SOUL_SYSTEM", 1);

        PyModule_AddIntConstant(module_value, "ENABLE_NEW_EQUIPMENT_SYSTEM", 1);

        PyModule_AddIntConstant(module_value, "USE_OPENID", 0);
        PyModule_AddIntConstant(module_value, "OPENID_TEST", 0);
    }
}
