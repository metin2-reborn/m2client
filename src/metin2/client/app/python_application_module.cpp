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

    auto app_show_web_page(PyObject *self, PyObject *args) -> PyObject * {
        char *web_page;
        PyObject *rect = nullptr;

        if (!PyArg_ParseTuple(args, "s|O", &web_page, &rect)) {
            return nullptr;
        }

        metin2::log::info("No embedded web browser; not showing {}", web_page);
        Py_RETURN_NONE;
    }

    auto app_move_web_page(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *rect = nullptr;
        if (!PyArg_ParseTuple(args, "|O", &rect)) {
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    auto app_hide_web_page(PyObject *self, PyObject *args) -> PyObject * {
        Py_RETURN_NONE;
    }

    auto app_is_web_page_mode(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", 0);
    }

    auto app_enable_performance_time(PyObject *self, PyObject *args) -> PyObject * {
        char *mode;
        int enable;

        if (!PyArg_ParseTuple(args, "si", &mode, &enable)) {
            return nullptr;
        }

        if (strcmp(mode, "RENDER_GAME") == 0) {
            perf_checker_render_game = enable ? true : false;
        }

        Py_RETURN_NONE;
    }

    extern bool ride_horse_enable;
    extern double specular_spd;

    auto app_set_text_tail_living_time(PyObject *self, PyObject *args) -> PyObject * {
        float living_time;
        if (!PyArg_ParseTuple(args, "f", &living_time)) {
            return nullptr;
        }

        text_tail_set_living_time(living_time * 1000);
        Py_RETURN_NONE;
    }

    auto app_set_hair_color_enable(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!PyArg_ParseTuple(args, "i", &enable)) {
            return nullptr;
        }

        hair_color_enable = enable;

        Py_RETURN_NONE;
    }

    auto app_set_armor_specular_enable(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!PyArg_ParseTuple(args, "i", &enable)) {
            return nullptr;
        }

        use_armor_specular = enable;

        Py_RETURN_NONE;
    }

    auto app_set_weapon_specular_enable(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!PyArg_ParseTuple(args, "i", &enable)) {
            return nullptr;
        }

        use_weapon_specular = enable;
        Py_RETURN_NONE;
    }

    auto app_set_skill_effect_upgrade_enable(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!PyArg_ParseTuple(args, "i", &enable)) {
            return nullptr;
        }

        skill_effect_upgrade_enable = enable;
        Py_RETURN_NONE;
    }

    auto set_two_handed_weapon_att_speed_decrease_value(PyObject *self, PyObject *args) -> PyObject * {
        int value;
        if (!PyArg_ParseTuple(args, "i", &value)) {
            return nullptr;
        }

        two_handed_weapon_attack_speed_decrease = value;
        Py_RETURN_NONE;
    }

    auto app_set_camera_max_distance(PyObject *self, PyObject *args) -> PyObject * {
        float max_value;
        if (!PyArg_ParseTuple(args, "f", &max_value)) {
            return nullptr;
        }

        camera::set_camera_max_distance(max_value);
        Py_RETURN_NONE;
    }

    // Set the x87 FPU to 24-bit precision on Windows; x86-64 does its float math in SSE, so nothing to do.
    auto app_set_control_fp(PyObject *self, PyObject *args) -> PyObject * {
        Py_RETURN_NONE;
    }

    auto app_set_min_fog(PyObject *self, PyObject *args) -> PyObject * {
        float min_fog;
        if (!PyArg_ParseTuple(args, "f", &min_fog)) {
            return nullptr;
        }

        get_python_application().set_min_fog(min_fog);
        Py_RETURN_NONE;
    }

    auto load_locale_data() -> bool;

    auto app_load_locale_data(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", load_locale_data());
    }

    auto app_get_state_path(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!PyArg_ParseTuple(args, "s", &name)) {
            return nullptr;
        }

        return Py_BuildValue("s", metin2::state::path(name).c_str());
    }

    auto app_get_image_info(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        bgra_image image;
        const auto can_load = load_image_file(file_name, image);

        return Py_BuildValue("iii", can_load, image.width, image.height);
    }

    auto app_is_exist_file(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        auto exists = get_file_manager().is_exist(file_name);

        return Py_BuildValue("i", exists);
    }

    auto app_get_file_list(PyObject *self, PyObject *args) -> PyObject * {
        char *filter;
        if (!PyArg_ParseTuple(args, "s", &filter)) {
            return nullptr;
        }

        auto *list = PyList_New(0);

        // Asset folders (bgm:*.mp3) are listed through the file manager. upload/ holds the player's
        // own guild mark images, which the mark uploader reads from disk, so it is listed there.
        std::string st_filter(filter);
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

        const auto slash = st_filter.rfind('/');
        const auto folder = st_filter.substr(0, slash);
        const auto mask = st_filter.substr(slash + 1);

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
        Py_RETURN_NONE;
    }

    auto app_render_game(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().render_game();
        Py_RETURN_NONE;
    }

    auto app_set_mouse_handler(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handler;
        if (!PyArg_ParseTuple(args, "O", &handler)) {
            return nullptr;
        }

        get_python_application().set_mouse_handler(handler);
        Py_RETURN_NONE;
    }

    auto app_create(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        int width, height;
        int windowed;

        if (!PyArg_ParseTuple(args, "siii", &name, &width, &height, &windowed)) {
            return nullptr;
        }

        auto &app = get_python_application();
        if (!app.create(self, name, width, height, windowed)) {
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    auto app_loop(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().run();

        Py_RETURN_NONE;
    }

    auto app_get_info(PyObject *self, PyObject *args) -> PyObject * {
        int info_type;
        if (!PyArg_ParseTuple(args, "i", &info_type)) {
            return nullptr;
        }

        std::string info;
        get_python_application().get_info(info_type, &info);

        return Py_BuildValue("s", info.c_str());
    }

    auto app_abort(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().abort();
        Py_RETURN_NONE;
    }

    auto app_exit(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().exit();
        Py_RETURN_NONE;
    }

    auto app_set_camera(PyObject *self, PyObject *args) -> PyObject * {
        float distance, pitch, rotation, destination_height;
        if (!PyArg_ParseTuple(args, "ffff", &distance, &pitch, &rotation, &destination_height)) {
            return nullptr;
        }

        get_python_application().set_camera(distance, pitch, rotation, destination_height);
        Py_RETURN_NONE;
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
        if (!PyArg_ParseTuple(args, "i", &direction)) {
            return nullptr;
        }

        get_python_application().rotate_camera(direction);
        Py_RETURN_NONE;
    }

    auto app_pitch_camera(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!PyArg_ParseTuple(args, "i", &direction)) {
            return nullptr;
        }

        get_python_application().pitch_camera(direction);
        Py_RETURN_NONE;
    }

    auto app_zoom_camera(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!PyArg_ParseTuple(args, "i", &direction)) {
            return nullptr;
        }

        get_python_application().zoom_camera(direction);
        Py_RETURN_NONE;
    }

    auto app_movie_rotate_camera(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!PyArg_ParseTuple(args, "i", &direction)) {
            return nullptr;
        }

        get_python_application().movie_rotate_camera(direction);
        Py_RETURN_NONE;
    }

    auto app_movie_pitch_camera(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!PyArg_ParseTuple(args, "i", &direction)) {
            return nullptr;
        }

        get_python_application().movie_pitch_camera(direction);
        Py_RETURN_NONE;
    }

    auto app_movie_zoom_camera(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!PyArg_ParseTuple(args, "i", &direction)) {
            return nullptr;
        }

        get_python_application().movie_zoom_camera(direction);
        Py_RETURN_NONE;
    }

    auto app_movie_reset_camera(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().movie_reset_camera();
        Py_RETURN_NONE;
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

    auto app_get_available_texture_memory(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", graphic_base::available_texture_memory());
    }

    auto app_set_global_center_position(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!PyArg_ParseTuple(args, "ii", &x, &y)) {
            return nullptr;
        }

        get_python_application().set_global_center_position(x, y);
        Py_RETURN_NONE;
    }

    auto app_set_center_position(PyObject *self, PyObject *args) -> PyObject * {
        float x, y, z;
        if (!PyArg_ParseTuple(args, "fff", &x, &y, &z)) {
            return nullptr;
        }

        get_python_application().set_center_position(x, -y, z);
        Py_RETURN_NONE;
    }

    auto app_get_cursor_position(PyObject *self, PyObject *args) -> PyObject * {
        int32_t x, y;

        widgets::get_window_manager().mouse_position(x, y);

        return Py_BuildValue("ii", x, y);
    }

    auto app_is_pressed(PyObject *self, PyObject *args) -> PyObject * {
        int key;
        if (!PyArg_ParseTuple(args, "i", &key)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_application().is_pressed(key));
    }

    auto app_set_cursor(PyObject *self, PyObject *args) -> PyObject * {
        int cursor;
        if (!PyArg_ParseTuple(args, "i", &cursor)) {
            return nullptr;
        }

        if (!get_python_application().set_cursor_num(cursor)) {
            PyErr_Format(PyExc_RuntimeError, "Wrong cursor name [%d]", cursor);
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    auto app_get_cursor(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_application().cursor_num());
    }

    auto app_show_cursor(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().set_cursor_visible(true);
        Py_RETURN_NONE;
    }

    auto app_hide_cursor(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().set_cursor_visible(false);
        Py_RETURN_NONE;
    }

    auto app_is_show_cursor(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", true == get_python_application().cursor_visible());
    }

    auto app_is_liar_cursor_on(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", true == get_python_application().liar_cursor_on());
    }

    auto app_get_random(PyObject *self, PyObject *args) -> PyObject * {
        int from, to;
        if (!PyArg_ParseTuple(args, "ii", &from, &to)) {
            return nullptr;
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
        if (!PyArg_ParseTuple(args, "ff", &source, &target)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_rotating_direction(source, target));
    }

    auto app_get_degree_difference(PyObject *self, PyObject *args) -> PyObject * {
        float source, target;
        if (!PyArg_ParseTuple(args, "ff", &source, &target)) {
            return nullptr;
        }

        return Py_BuildValue("f", get_degree_difference(source, target));
    }

    auto app_set_default_font_name(PyObject *self, PyObject *args) -> PyObject * {
        char *font_name;
        if (!PyArg_ParseTuple(args, "s", &font_name)) {
            return nullptr;
        }

        default_font_set_name(font_name);
        Py_RETURN_NONE;
    }

    auto app_set_guild_symbol_path(PyObject *self, PyObject *args) -> PyObject * {
        char *path_name;
        if (!PyArg_ParseTuple(args, "s", &path_name)) {
            return nullptr;
        }

        set_guild_symbol_path(path_name);
        Py_RETURN_NONE;
    }

    auto app_enable_special_camera_mode(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().enable_special_camera_mode();
        Py_RETURN_NONE;
    }

    auto app_set_camera_speed(PyObject *self, PyObject *args) -> PyObject * {
        int percentage;
        if (!PyArg_ParseTuple(args, "i", &percentage)) {
            return nullptr;
        }

        get_python_application().set_camera_speed(percentage);

        auto *camera = get_camera_manager().current_camera();
        if (camera) {
            camera->set_resistance(float(percentage) / 100.0f);
        }

        Py_RETURN_NONE;
    }

    auto app_set_camera_setting(PyObject *self, PyObject *args) -> PyObject * {
        int x, y, z;
        int zoom, rotation, pitch;
        if (!PyArg_ParseTuple(args, "iiiiii", &x, &y, &z, &zoom, &rotation, &pitch)) {
            return nullptr;
        }

        python_application::camera_setting camera_setting;

        memset(static_cast<void *>(&camera_setting), 0, sizeof(camera_setting));
        camera_setting.center_position.x = float(x);
        camera_setting.center_position.y = float(y);
        camera_setting.center_position.z = float(z);
        camera_setting.zoom = float(zoom);
        camera_setting.rotation = float(rotation);
        camera_setting.pitch = float(pitch);

        get_python_application().set_event_camera(camera_setting);
        Py_RETURN_NONE;
    }

    auto app_save_camera_setting(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        get_python_application().save_camera_setting(file_name);
        Py_RETURN_NONE;
    }

    auto app_load_camera_setting(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        auto result = get_python_application().load_camera_setting(file_name);

        return Py_BuildValue("i", result);
    }

    auto app_set_default_camera(PyObject *self, PyObject *args) -> PyObject * {
        get_python_application().set_default_camera();
        Py_RETURN_NONE;
    }

    auto app_set_sight_range(PyObject *self, PyObject *args) -> PyObject * {
        int range;
        if (!PyArg_ParseTuple(args, "i", &range)) {
            return nullptr;
        }

        get_python_application().set_force_sight_range(range);
        Py_RETURN_NONE;
    }

    extern int accumulation_time;

    auto app_set_visible_notice(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!PyArg_ParseTuple(args, "i", &flag)) {
            return nullptr;
        }

        visible_notice = flag;
        Py_RETURN_NONE;
    }

    auto app_is_visible_notice(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", visible_notice);
    }

    auto app_enable_test_server_flag(PyObject *self, PyObject *args) -> PyObject * {
        test_server_flag = true;
        Py_RETURN_NONE;
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

    constexpr const char *TEXT_FILE_CAPSULE = "metin2.app.text_file";

    auto app_open_text_file(PyObject *self, PyObject *args) -> PyObject * {
        char *path;
        if (!PyArg_ParseTuple(args, "s", &path)) {
            return nullptr;
        }

        auto *loader = new text_line_loader(path);
        auto *loader_capsule = PyCapsule_New(loader, TEXT_FILE_CAPSULE, [](PyObject *capsule) -> void {
            delete static_cast<text_line_loader *>(PyCapsule_GetPointer(capsule, TEXT_FILE_CAPSULE));
        });

        if (loader_capsule == nullptr) {
            delete loader;
        }

        return loader_capsule;
    }

    auto app_close_text_file(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handle;
        if (!PyArg_ParseTuple(args, "O", &handle)) {
            return nullptr;
        }

        if (!PyCapsule_IsValid(handle, TEXT_FILE_CAPSULE)) {
            PyErr_SetString(PyExc_ValueError, "CloseTextFile: not a text file handle");
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    auto app_get_text_file_line_count(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handle;
        if (!PyArg_ParseTuple(args, "O", &handle)) {
            return nullptr;
        }

        auto *loader = static_cast<text_line_loader *>(PyCapsule_GetPointer(handle, TEXT_FILE_CAPSULE));
        if (!loader) {
            return nullptr;
        }

        return Py_BuildValue("i", loader->line_count());
    }

    auto app_get_text_file_line(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handle;
        int line_index;
        if (!PyArg_ParseTuple(args, "Oi", &handle, &line_index)) {
            return nullptr;
        }

        auto *loader = static_cast<text_line_loader *>(PyCapsule_GetPointer(handle, TEXT_FILE_CAPSULE));
        if (!loader) {
            return nullptr;
        }

        return Py_BuildValue("s", loader->get_line(line_index));
    }

    auto app_set_guild_mark_path(PyObject *self, PyObject *args) -> PyObject * {
        char *path;
        if (!PyArg_ParseTuple(args, "s", &path)) {
            return nullptr;
        }

        std::string new_path = path;
        if (const size_t ext = new_path.find(".tga"); ext != std::string::npos) {
            new_path.resize(ext);
        }

        get_guild_mark_manager().set_mark_path_prefix(new_path.c_str());
        Py_RETURN_NONE;
    }

    auto app_is_dev_stage(PyObject *self, PyObject *args) -> PyObject * {
        int is_development_stage = 0;
        return Py_BuildValue("i", is_development_stage);
    }

    auto app_logo_open(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!PyArg_ParseTuple(args, "s", &name)) {
            return nullptr;
        }

        return Py_BuildValue("i", 1);
    }

    auto app_logo_update(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", 0);
    }

    auto app_logo_render(PyObject *self, PyObject *args) -> PyObject * {
        Py_RETURN_NONE;
    }

    auto app_logo_close(PyObject *self, PyObject *args) -> PyObject * {
        Py_RETURN_NONE;
    }

    void init_app_module() {
        static PyMethodDef methods[] = {
            {"is_dev_stage", app_is_dev_stage, METH_VARARGS},

            {"set_text_tail_living_time", app_set_text_tail_living_time, METH_VARARGS},

            {"enable_performance_time", app_enable_performance_time, METH_VARARGS},
            {"set_hair_color_enable", app_set_hair_color_enable, METH_VARARGS},

            {"set_armor_specular_enable", app_set_armor_specular_enable, METH_VARARGS},
            {"set_weapon_specular_enable", app_set_weapon_specular_enable, METH_VARARGS},
            {"set_skill_effect_upgrade_enable", app_set_skill_effect_upgrade_enable, METH_VARARGS},
            {"set_two_handed_weapon_att_speed_decrease_value", set_two_handed_weapon_att_speed_decrease_value, METH_VARARGS},

            {"set_camera_max_distance", app_set_camera_max_distance, METH_VARARGS},
            {"set_min_fog", app_set_min_fog, METH_VARARGS},
            {"get_image_info", app_get_image_info, METH_VARARGS},
            {"get_info", app_get_info, METH_VARARGS},
            {"update_game", app_update_game, METH_VARARGS},
            {"render_game", app_render_game, METH_VARARGS},
            {"loop", app_loop, METH_VARARGS},
            {"create", app_create, METH_VARARGS},
            {"exit", app_exit, METH_VARARGS},
            {"abort", app_abort, METH_VARARGS},
            {"set_mouse_handler", app_set_mouse_handler, METH_VARARGS},
            {"is_exist_file", app_is_exist_file, METH_VARARGS},
            {"get_file_list", app_get_file_list, METH_VARARGS},

            {"set_camera", app_set_camera, METH_VARARGS},
            {"get_camera", app_get_camera, METH_VARARGS},
            {"get_camera_pitch", app_get_camera_pitch, METH_VARARGS},
            {"get_time", app_get_time, METH_VARARGS},
            {"get_global_time", app_get_global_time, METH_VARARGS},
            {"get_global_time_stamp", app_get_global_time_stamp, METH_VARARGS},
            {"get_update_fps", app_get_update_fps, METH_VARARGS},
            {"get_render_fps", app_get_render_fps, METH_VARARGS},
            {"rotate_camera", app_rotate_camera, METH_VARARGS},
            {"pitch_camera", app_pitch_camera, METH_VARARGS},
            {"zoom_camera", app_zoom_camera, METH_VARARGS},
            {"movie_rotate_camera", app_movie_rotate_camera, METH_VARARGS},
            {"movie_pitch_camera", app_movie_pitch_camera, METH_VARARGS},
            {"movie_zoom_camera", app_movie_zoom_camera, METH_VARARGS},
            {"movie_reset_camera", app_movie_reset_camera, METH_VARARGS},

            {"get_available_texture_memory", app_get_available_texture_memory, METH_VARARGS},
            {"get_render_time", app_get_render_time, METH_VARARGS},
            {"get_update_time", app_get_update_time, METH_VARARGS},
            {"get_face_speed", app_get_face_speed, METH_VARARGS},
            {"get_face_count", app_get_face_count, METH_VARARGS},
            {"set_global_center_position", app_set_global_center_position, METH_VARARGS},
            {"set_center_position", app_set_center_position, METH_VARARGS},
            {"get_cursor_position", app_get_cursor_position, METH_VARARGS},

            {"get_random", app_get_random, METH_VARARGS},
            {"is_web_page_mode", app_is_web_page_mode, METH_VARARGS},
            {"show_web_page", app_show_web_page, METH_VARARGS},
            {"move_web_page", app_move_web_page, METH_VARARGS},
            {"hide_web_page", app_hide_web_page, METH_VARARGS},
            {"is_pressed", app_is_pressed, METH_VARARGS},
            {"set_cursor", app_set_cursor, METH_VARARGS},
            {"get_cursor", app_get_cursor, METH_VARARGS},
            {"show_cursor", app_show_cursor, METH_VARARGS},
            {"hide_cursor", app_hide_cursor, METH_VARARGS},
            {"is_show_cursor", app_is_show_cursor, METH_VARARGS},
            {"is_liar_cursor_on", app_is_liar_cursor_on, METH_VARARGS},

            {"get_rotating_direction", app_get_rotating_direction, METH_VARARGS},
            {"get_degree_difference", app_get_degree_difference, METH_VARARGS},
            {"set_default_font_name", app_set_default_font_name, METH_VARARGS},
            {"set_guild_symbol_path", app_set_guild_symbol_path, METH_VARARGS},

            {"enable_special_camera_mode", app_enable_special_camera_mode, METH_VARARGS},
            {"set_camera_speed", app_set_camera_speed, METH_VARARGS},

            {"save_camera_setting", app_save_camera_setting, METH_VARARGS},
            {"load_camera_setting", app_load_camera_setting, METH_VARARGS},
            {"set_default_camera", app_set_default_camera, METH_VARARGS},
            {"set_camera_setting", app_set_camera_setting, METH_VARARGS},

            {"set_sight_range", app_set_sight_range, METH_VARARGS},

            {"open_text_file", app_open_text_file, METH_VARARGS},
            {"close_text_file", app_close_text_file, METH_VARARGS},
            {"get_text_file_line_count", app_get_text_file_line_count, METH_VARARGS},
            {"get_text_file_line", app_get_text_file_line, METH_VARARGS},

            {"get_state_path", app_get_state_path, METH_VARARGS},

            {"load_locale_data", app_load_locale_data, METH_NOARGS},

            {"set_control_fp", app_set_control_fp, METH_VARARGS},

            {"set_visible_notice", app_set_visible_notice, METH_VARARGS},
            {"is_visible_notice", app_is_visible_notice, METH_VARARGS},
            {"enable_test_server_flag", app_enable_test_server_flag, METH_VARARGS},
            {"is_enable_test_server_flag", app_is_enable_test_server_flag, METH_VARARGS},

            {"set_guild_mark_path", app_set_guild_mark_path, METH_VARARGS},

            {"on_logo_update", app_logo_update, METH_VARARGS},
            {"on_logo_render", app_logo_render, METH_VARARGS},
            {"on_logo_open", app_logo_open, METH_VARARGS},
            {"on_logo_close", app_logo_close, METH_VARARGS},

            {nullptr, nullptr},
        };

        auto *module = init_python_module("metin2.app", methods);

        PyModule_AddIntConstant(module, "INFO_ITEM", python_application::INFO_ITEM);
        PyModule_AddIntConstant(module, "INFO_ACTOR", python_application::INFO_ACTOR);
        PyModule_AddIntConstant(module, "INFO_EFFECT", python_application::INFO_EFFECT);
        PyModule_AddIntConstant(module, "INFO_TEXTTAIL", python_application::INFO_TEXTTAIL);

        PyModule_AddIntConstant(module, "DEGREE_DIRECTION_SAME", DEGREE_DIRECTION_SAME);
        PyModule_AddIntConstant(module, "DEGREE_DIRECTION_RIGHT", DEGREE_DIRECTION_RIGHT);
        PyModule_AddIntConstant(module, "DEGREE_DIRECTION_LEFT", DEGREE_DIRECTION_LEFT);

        PyModule_AddIntConstant(module, "VK_LEFT", VK_LEFT);
        PyModule_AddIntConstant(module, "VK_RIGHT", VK_RIGHT);
        PyModule_AddIntConstant(module, "VK_UP", VK_UP);
        PyModule_AddIntConstant(module, "VK_DOWN", VK_DOWN);
        PyModule_AddIntConstant(module, "VK_HOME", VK_HOME);
        PyModule_AddIntConstant(module, "VK_END", VK_END);
        PyModule_AddIntConstant(module, "VK_DELETE", VK_DELETE);

        PyModule_AddIntConstant(module, "DIK_ESCAPE", DIK_ESCAPE);
        PyModule_AddIntConstant(module, "DIK_ESC", DIK_ESCAPE);
        PyModule_AddIntConstant(module, "DIK_1", DIK_1);
        PyModule_AddIntConstant(module, "DIK_2", DIK_2);
        PyModule_AddIntConstant(module, "DIK_3", DIK_3);
        PyModule_AddIntConstant(module, "DIK_4", DIK_4);
        PyModule_AddIntConstant(module, "DIK_5", DIK_5);
        PyModule_AddIntConstant(module, "DIK_6", DIK_6);
        PyModule_AddIntConstant(module, "DIK_7", DIK_7);
        PyModule_AddIntConstant(module, "DIK_8", DIK_8);
        PyModule_AddIntConstant(module, "DIK_9", DIK_9);
        PyModule_AddIntConstant(module, "DIK_0", DIK_0);
        PyModule_AddIntConstant(module, "DIK_MINUS", DIK_MINUS);
        PyModule_AddIntConstant(module, "DIK_EQUALS", DIK_EQUALS);
        PyModule_AddIntConstant(module, "DIK_BACK", DIK_BACK);
        PyModule_AddIntConstant(module, "DIK_TAB", DIK_TAB);
        PyModule_AddIntConstant(module, "DIK_Q", DIK_Q);
        PyModule_AddIntConstant(module, "DIK_W", DIK_W);
        PyModule_AddIntConstant(module, "DIK_E", DIK_E);
        PyModule_AddIntConstant(module, "DIK_R", DIK_R);
        PyModule_AddIntConstant(module, "DIK_T", DIK_T);
        PyModule_AddIntConstant(module, "DIK_Y", DIK_Y);
        PyModule_AddIntConstant(module, "DIK_U", DIK_U);
        PyModule_AddIntConstant(module, "DIK_I", DIK_I);
        PyModule_AddIntConstant(module, "DIK_O", DIK_O);
        PyModule_AddIntConstant(module, "DIK_P", DIK_P);
        PyModule_AddIntConstant(module, "DIK_LBRACKET", DIK_LBRACKET);
        PyModule_AddIntConstant(module, "DIK_RBRACKET", DIK_RBRACKET);
        PyModule_AddIntConstant(module, "DIK_RETURN", DIK_RETURN);
        PyModule_AddIntConstant(module, "DIK_LCONTROL", DIK_LCONTROL);
        PyModule_AddIntConstant(module, "DIK_A", DIK_A);
        PyModule_AddIntConstant(module, "DIK_S", DIK_S);
        PyModule_AddIntConstant(module, "DIK_D", DIK_D);
        PyModule_AddIntConstant(module, "DIK_F", DIK_F);
        PyModule_AddIntConstant(module, "DIK_G", DIK_G);
        PyModule_AddIntConstant(module, "DIK_H", DIK_H);
        PyModule_AddIntConstant(module, "DIK_J", DIK_J);
        PyModule_AddIntConstant(module, "DIK_K", DIK_K);
        PyModule_AddIntConstant(module, "DIK_L", DIK_L);
        PyModule_AddIntConstant(module, "DIK_SEMICOLON", DIK_SEMICOLON);
        PyModule_AddIntConstant(module, "DIK_APOSTROPHE", DIK_APOSTROPHE);
        PyModule_AddIntConstant(module, "DIK_GRAVE", DIK_GRAVE);
        PyModule_AddIntConstant(module, "DIK_LSHIFT", DIK_LSHIFT);
        PyModule_AddIntConstant(module, "DIK_BACKSLASH", DIK_BACKSLASH);
        PyModule_AddIntConstant(module, "DIK_Z", DIK_Z);
        PyModule_AddIntConstant(module, "DIK_X", DIK_X);
        PyModule_AddIntConstant(module, "DIK_C", DIK_C);
        PyModule_AddIntConstant(module, "DIK_V", DIK_V);
        PyModule_AddIntConstant(module, "DIK_B", DIK_B);
        PyModule_AddIntConstant(module, "DIK_N", DIK_N);
        PyModule_AddIntConstant(module, "DIK_M", DIK_M);
        PyModule_AddIntConstant(module, "DIK_COMMA", DIK_COMMA);
        PyModule_AddIntConstant(module, "DIK_PERIOD", DIK_PERIOD);
        PyModule_AddIntConstant(module, "DIK_SLASH", DIK_SLASH);
        PyModule_AddIntConstant(module, "DIK_RSHIFT", DIK_RSHIFT);
        PyModule_AddIntConstant(module, "DIK_MULTIPLY", DIK_MULTIPLY);
        PyModule_AddIntConstant(module, "DIK_LALT", DIK_LMENU);
        PyModule_AddIntConstant(module, "DIK_SPACE", DIK_SPACE);
        PyModule_AddIntConstant(module, "DIK_CAPITAL", DIK_CAPITAL);
        PyModule_AddIntConstant(module, "DIK_F1", DIK_F1);
        PyModule_AddIntConstant(module, "DIK_F2", DIK_F2);
        PyModule_AddIntConstant(module, "DIK_F3", DIK_F3);
        PyModule_AddIntConstant(module, "DIK_F4", DIK_F4);
        PyModule_AddIntConstant(module, "DIK_F5", DIK_F5);
        PyModule_AddIntConstant(module, "DIK_F6", DIK_F6);
        PyModule_AddIntConstant(module, "DIK_F7", DIK_F7);
        PyModule_AddIntConstant(module, "DIK_F8", DIK_F8);
        PyModule_AddIntConstant(module, "DIK_F9", DIK_F9);
        PyModule_AddIntConstant(module, "DIK_F10", DIK_F10);
        PyModule_AddIntConstant(module, "DIK_NUMLOCK", DIK_NUMLOCK);
        PyModule_AddIntConstant(module, "DIK_SCROLL", DIK_SCROLL);
        PyModule_AddIntConstant(module, "DIK_NUMPAD7", DIK_NUMPAD7);
        PyModule_AddIntConstant(module, "DIK_NUMPAD8", DIK_NUMPAD8);
        PyModule_AddIntConstant(module, "DIK_NUMPAD9", DIK_NUMPAD9);
        PyModule_AddIntConstant(module, "DIK_SUBTRACT", DIK_SUBTRACT);
        PyModule_AddIntConstant(module, "DIK_NUMPAD4", DIK_NUMPAD4);
        PyModule_AddIntConstant(module, "DIK_NUMPAD5", DIK_NUMPAD5);
        PyModule_AddIntConstant(module, "DIK_NUMPAD6", DIK_NUMPAD6);
        PyModule_AddIntConstant(module, "DIK_ADD", DIK_ADD);
        PyModule_AddIntConstant(module, "DIK_NUMPAD1", DIK_NUMPAD1);
        PyModule_AddIntConstant(module, "DIK_NUMPAD2", DIK_NUMPAD2);
        PyModule_AddIntConstant(module, "DIK_NUMPAD3", DIK_NUMPAD3);
        PyModule_AddIntConstant(module, "DIK_NUMPAD0", DIK_NUMPAD0);
        PyModule_AddIntConstant(module, "DIK_DECIMAL", DIK_DECIMAL);
        PyModule_AddIntConstant(module, "DIK_F11", DIK_F11);
        PyModule_AddIntConstant(module, "DIK_F12", DIK_F12);
        PyModule_AddIntConstant(module, "DIK_NEXTTRACK", DIK_NEXTTRACK);
        PyModule_AddIntConstant(module, "DIK_NUMPADENTER", DIK_NUMPADENTER);
        PyModule_AddIntConstant(module, "DIK_RCONTROL", DIK_RCONTROL);
        PyModule_AddIntConstant(module, "DIK_MUTE", DIK_MUTE);
        PyModule_AddIntConstant(module, "DIK_CALCULATOR", DIK_CALCULATOR);
        PyModule_AddIntConstant(module, "DIK_PLAYPAUSE", DIK_PLAYPAUSE);
        PyModule_AddIntConstant(module, "DIK_MEDIASTOP", DIK_MEDIASTOP);
        PyModule_AddIntConstant(module, "DIK_VOLUMEDOWN", DIK_VOLUMEDOWN);
        PyModule_AddIntConstant(module, "DIK_VOLUMEUP", DIK_VOLUMEUP);
        PyModule_AddIntConstant(module, "DIK_WEBHOME", DIK_WEBHOME);
        PyModule_AddIntConstant(module, "DIK_NUMPADCOMMA", DIK_NUMPADCOMMA);
        PyModule_AddIntConstant(module, "DIK_DIVIDE", DIK_DIVIDE);
        PyModule_AddIntConstant(module, "DIK_SYSRQ", DIK_SYSRQ);
        PyModule_AddIntConstant(module, "DIK_RALT", DIK_RMENU);
        PyModule_AddIntConstant(module, "DIK_PAUSE", DIK_PAUSE);
        PyModule_AddIntConstant(module, "DIK_HOME", DIK_HOME);
        PyModule_AddIntConstant(module, "DIK_UP", DIK_UP);
        PyModule_AddIntConstant(module, "DIK_PGUP", DIK_PRIOR);
        PyModule_AddIntConstant(module, "DIK_LEFT", DIK_LEFT);
        PyModule_AddIntConstant(module, "DIK_RIGHT", DIK_RIGHT);
        PyModule_AddIntConstant(module, "DIK_END", DIK_END);
        PyModule_AddIntConstant(module, "DIK_DOWN", DIK_DOWN);
        PyModule_AddIntConstant(module, "DIK_PGDN", DIK_NEXT);
        PyModule_AddIntConstant(module, "DIK_INSERT", DIK_INSERT);
        PyModule_AddIntConstant(module, "DIK_DELETE", DIK_DELETE);
        PyModule_AddIntConstant(module, "DIK_LWIN", DIK_LWIN);
        PyModule_AddIntConstant(module, "DIK_RWIN", DIK_RWIN);
        PyModule_AddIntConstant(module, "DIK_APPS", DIK_APPS);

        // Cursor shapes
        PyModule_AddIntConstant(module, "NORMAL", python_application::CURSOR_SHAPE_NORMAL);
        PyModule_AddIntConstant(module, "ATTACK", python_application::CURSOR_SHAPE_ATTACK);
        PyModule_AddIntConstant(module, "TARGET", python_application::CURSOR_SHAPE_TARGET);
        PyModule_AddIntConstant(module, "TALK", python_application::CURSOR_SHAPE_TALK);
        PyModule_AddIntConstant(module, "CANT_GO", python_application::CURSOR_SHAPE_CANT_GO);
        PyModule_AddIntConstant(module, "PICK", python_application::CURSOR_SHAPE_PICK);

        PyModule_AddIntConstant(module, "DOOR", python_application::CURSOR_SHAPE_DOOR);
        PyModule_AddIntConstant(module, "CHAIR", python_application::CURSOR_SHAPE_CHAIR);
        PyModule_AddIntConstant(module, "MAGIC", python_application::CURSOR_SHAPE_MAGIC);
        PyModule_AddIntConstant(module, "BUY", python_application::CURSOR_SHAPE_BUY);
        PyModule_AddIntConstant(module, "SELL", python_application::CURSOR_SHAPE_SELL);

        PyModule_AddIntConstant(module, "CAMERA_ROTATE", python_application::CURSOR_SHAPE_CAMERA_ROTATE);
        PyModule_AddIntConstant(module, "HSIZE", python_application::CURSOR_SHAPE_HSIZE);
        PyModule_AddIntConstant(module, "VSIZE", python_application::CURSOR_SHAPE_VSIZE);
        PyModule_AddIntConstant(module, "HVSIZE", python_application::CURSOR_SHAPE_HVSIZE);

        PyModule_AddIntConstant(module, "CAMERA_TO_POSITIVE", python_application::CAMERA_TO_POSITIVE);
        PyModule_AddIntConstant(module, "CAMERA_TO_NEGATIVE", python_application::CAMERA_TO_NEGITIVE);
        PyModule_AddIntConstant(module, "CAMERA_STOP", python_application::CAMERA_STOP);
    }
}
