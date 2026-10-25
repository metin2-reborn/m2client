#include "python_application.h"

#include <metin2/client/app/python_system.h>
#include <metin2/client/background/python_background.h>
#include <metin2/client/characters/python_character_manager.h>
#include <metin2/core/error.h>
#include <metin2/core/text_file_loader.h>
#include <metin2/core/utils.h>
#include <metin2/dev/automation.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/attribute_instance.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/font_face.h>
#include <metin2/graphics/grp_device.h>
#include <metin2/log/log.h>
#include <metin2/models/lod_controller.h>
#include <metin2/models/material.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/terrain/area_terrain.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <thread>

namespace metin2::inline graphics {
}

namespace metin2::inline client::inline app {
    float min_fog_distance = 2400.0f;
    double specular_spd = 0.007f;

    const float default_camera_rotate_speed = 1.5f;
    const float default_camera_pitch_speed = 1.5f;
    const float default_camera_zoom_speed = 0.05f;

    // The creation order matters: the managers are destroyed at exit in the reverse order.
    static void create_managers() {
        get_timer();
        get_light_manager();
        get_sound_manager();
        get_flying_manager();
        get_race_manager();
        get_game_event_manager();
        get_item_manager();
        widgets::get_window_manager();
        get_effect_manager();
        get_python_character_manager();
        get_server_state_checker();
        get_culling_manager();
        get_python_graphic();
        get_python_network_stream();
        get_python_player();
        get_python_item();
        get_python_shop();
        get_python_exchange();
        get_python_chat();
        get_python_text_tail();
        get_python_non_player();
        get_python_mini_map();
        get_python_event_manager();
        get_property_manager();
        get_speed_tree_forest_direct_x8();
        get_python_background();
        get_python_skill();
        get_resource_manager();
        get_python_resource();
        get_python_quest();
        get_python_messenger();
        get_python_safe_box();
        get_python_guild();
        get_guild_mark_manager();
        get_guild_mark_downloader();
        get_guild_mark_uploader();
        get_account_connector();
        get_python_system();
    }

    python_application::python_application() : mouse_handler_(nullptr), ave_render_time_(0.0f), last_idle_time_(0), global_time_(0.0f), global_elapsed_time_(0.0f), update_fps_(0), render_fps_(0), face_count_(0), l_button_down_time_(0), mouse_capture_count_(0), cursor_visible_(true), liar_cursor_on_(false), cursor_mode_value_(CURSOR_MODE_HARDWARE), is_windowed_(false) {
        create_managers();
        set_eter_exception_handler();

        width_ = 800;
        height_ = 600;

        center_position_ = math::vector3(0.0f, 0.0f, 0.0f);
        start_local_time_ = el_timer_get_m_sec();
        server_time_ = 0;
        local_start_time_ = 0;

        port_ = 0;

        is_activate_wnd_ = false;
        is_minimized_wnd_ = true;

        rotation_speed_ = 0.0f;
        pitch_speed_ = 0.0f;
        zoom_speed_ = 0.0f;

        face_spd_ = 0.0f;

        face_acc_count_ = 0;
        face_acc_time_ = 0;

        face_spd_sum_ = 0;
        face_spd_count_ = 0;

        get_flying_manager().set_map_manager_ptr(&get_python_background());

        cursor_num_ = CURSOR_SHAPE_NORMAL;
        continuous_cursor_num_ = CURSOR_SHAPE_NORMAL;

        is_special_camera_mode_ = false;
        camera_rotate_speed_ = default_camera_rotate_speed;
        camera_pitch_speed_ = default_camera_pitch_speed;
        camera_zoom_speed_ = default_camera_zoom_speed;

        camera_mode_ = CAMERA_MODE_NORMAL;
        blend_camera_start_time_ = 0.0f;
        blend_camera_blend_time_ = 0.0f;

        force_sight_range_ = -1;

        get_camera_manager().add_camera(EVENT_CAMERA_NUMBER);
    }

    python_application::~python_application() = default;

    void python_application::set_min_fog(float min_fog) {
        min_fog_distance = min_fog;
    }

    void python_application::notify_hack(const char *format, ...) {
        char buf[1024];

        va_list args;
        va_start(args, format);
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        get_python_network_stream().notify_hack(buf);
    }

    void python_application::get_info(uint32_t e_info, std::string *pst_info) {
        switch (e_info) {
        case INFO_ACTOR:
            get_python_character_manager().get_info(pst_info);
            break;
        case INFO_EFFECT:
            get_effect_manager().get_info(pst_info);
            break;
        case INFO_ITEM:
            get_python_item().get_info(pst_info);
            break;
        case INFO_TEXTTAIL:
            get_python_text_tail().get_info(pst_info);
            break;
        }
    }

    void python_application::abort() {
        metin2::log::error("Aborting");
        metin2::log::error("Quitting the application");

        request_quit();
    }

    void python_application::exit() {
        request_quit();
    }

    bool perf_checker_render_game = false;

    void python_application::begin_scene() {
        gpu::device &device = get_python_graphic().device();
        const uint32_t width = device.back_buffer_width();
        const uint32_t height = device.back_buffer_height();
        if (width == 0 || height == 0)
            return;

        if (!scene_target_.is_created() || scene_target_.texture()->width() != width || scene_target_.texture()->height() != height) {
            if (!scene_target_.create(width, height, gpu::format::X8R8G8B8, gpu::format::D24X8))
                return;
        }

        if (!scene_target_.begin())
            return;
        device.clear({}, gpu::clear::TARGET | gpu::clear::Z_BUFFER, 0xff000000, 1.0f, 0);
        scene_active_ = true;
    }

    void python_application::end_scene() {
        if (!scene_active_)
            return;
        scene_active_ = false;
        scene_target_.end();

        const python_system &system = get_python_system();
        gpu::device::blit_params params;
        // The 1..5 gamma setting is a brightness step around the neutral 3.
        const int gamma = std::clamp(system.gamma(), 1, 5);
        params.gamma = 1.0f / (1.0f + static_cast<float>(gamma - 3) * 0.2f);
        params.antialias = system.is_antialias();
        get_python_graphic().device().blit(scene_target_.texture(), params);
    }

    void python_application::render_game() {
        if (!perf_checker_render_game) {
            float aspect = widgets::get_window_manager().aspect();
            float far_clip = get_python_background().far_clip();

            get_python_graphic().set_perspective(30.0f, aspect, 100.0, far_clip);

            get_culling_manager().process();

            get_python_character_manager().deform();
            get_effect_manager().update();

            get_python_background().render_character_shadow_to_texture();

            get_python_graphic().set_game_render_state();
            get_python_graphic().push_state();

            {
                int32_t lx, ly;
                widgets::get_window_manager().mouse_position(lx, ly);
                get_python_graphic().set_cursor_position(lx, ly);
            }

            begin_scene();
            get_python_background().render_sky();

            get_python_background().render_before_lens_flare();

            get_python_background().render_cloud();

            get_python_background().begin_environment();
            get_python_background().render();

            get_python_background().set_character_dir_light();
            get_python_character_manager().render();

            get_python_background().set_background_dir_light();
            get_python_background().render_water();
            get_python_background().render_snow();
            get_python_background().render_effect();

            get_python_background().end_environment();

            get_effect_manager().render();
            get_python_item().render();
            get_flying_manager().render();

            get_python_background().begin_environment();
            get_python_background().render_pc_blocker();
            get_python_background().end_environment();

            get_python_background().render_after_lens_flare();
            end_scene();

            return;
        }

        uint32_t t1 = el_timer_get_m_sec();
        get_python_character_manager().deform();
        uint32_t t2 = el_timer_get_m_sec();
        get_effect_manager().update();
        uint32_t t3 = el_timer_get_m_sec();
        get_python_background().render_character_shadow_to_texture();
        uint32_t t4 = el_timer_get_m_sec();

        get_python_graphic().set_game_render_state();
        get_python_graphic().push_state();

        float aspect = widgets::get_window_manager().aspect();
        float far_clip = get_python_background().far_clip();

        get_python_graphic().set_perspective(30.0f, aspect, 100.0, far_clip);

        uint32_t t5 = el_timer_get_m_sec();

        get_culling_manager().process();

        uint32_t t6 = el_timer_get_m_sec();

        {
            int32_t lx, ly;
            widgets::get_window_manager().mouse_position(lx, ly);
            get_python_graphic().set_cursor_position(lx, ly);
        }

        begin_scene();
        get_python_background().render_sky();
        uint32_t t7 = el_timer_get_m_sec();
        get_python_background().render_before_lens_flare();
        uint32_t t8 = el_timer_get_m_sec();
        get_python_background().render_cloud();
        uint32_t t9 = el_timer_get_m_sec();
        get_python_background().begin_environment();
        get_python_background().render();

        get_python_background().set_character_dir_light();
        uint32_t t10 = el_timer_get_m_sec();
        get_python_character_manager().render();
        uint32_t t11 = el_timer_get_m_sec();

        get_python_background().set_background_dir_light();
        get_python_background().render_water();
        uint32_t t12 = el_timer_get_m_sec();
        get_python_background().render_effect();
        uint32_t t13 = el_timer_get_m_sec();
        get_python_background().end_environment();
        get_effect_manager().render();
        uint32_t t14 = el_timer_get_m_sec();
        get_python_item().render();
        uint32_t t15 = el_timer_get_m_sec();
        get_flying_manager().render();
        uint32_t t16 = el_timer_get_m_sec();
        get_python_background().begin_environment();
        get_python_background().render_pc_blocker();
        get_python_background().end_environment();
        uint32_t t17 = el_timer_get_m_sec();
        get_python_background().render_after_lens_flare();
        end_scene();
        uint32_t t18 = el_timer_get_m_sec();
        uint32_t end = el_timer_get_m_sec();

        if (is_key_down(DIK_Z))
            STATEMANAGER.set_render_state(gpu::state::FILL_MODE, gpu::fill::SOLID);

        if (end - t1 < 3)
            return;

        metin2::log::debug("Render timing (ms): gr_total {} gr_dfm {} gr_eft_up {} gr_shw {} gr_stt {} gr_cll {} gr_bg_sky {} gr_bg_len {} gr_bg_cld {} gr_bg_main {} gr_chr {} gr_bg_wtr {} gr_bg_eft {} gr_eft {} gr_itm {} gr_fly {} gr_bg_blk {} gr_bg_len {}", end - t1, t2 - t1, t3 - t2, t4 - t3, t5 - t4, t6 - t5, t7 - t6, t8 - t7, t9 - t8, t10 - t9, t11 - t10, t12 - t11, t13 - t12, t14 - t13, t15 - t14, t16 - t15, t17 - t16, t18 - t17);
    }

    void python_application::update_game() {
        uint32_t t1 = el_timer_get_m_sec();
        const point mouse = window::mouse_position();
        const int scale = get_python_graphic().interface_scale();

        graphic_text_instance::hyperlink_update_mouse_pos(mouse.x / scale, mouse.y / scale);

        uint32_t t2 = el_timer_get_m_sec();

        // Workaround for a crash in SetTransform during Alt+Tab.
        {
            screen s;
            float aspect = widgets::get_window_manager().aspect();
            float far_clip = get_python_background().far_clip();

            s.set_perspective(30.0f, aspect, 100.0f, far_clip);
            s.build_view_frustum();
        }

        uint32_t t3 = el_timer_get_m_sec();
        pixel_position p_pos_main_actor;
        get_python_player().new_get_main_actor_position(&p_pos_main_actor);

        uint32_t t4 = el_timer_get_m_sec();
        get_python_background().update(p_pos_main_actor.x, p_pos_main_actor.y, p_pos_main_actor.z);

        uint32_t t5 = el_timer_get_m_sec();
        get_game_event_manager().set_center_position(p_pos_main_actor.x, p_pos_main_actor.y, p_pos_main_actor.z);
        get_game_event_manager().update();

        uint32_t t6 = el_timer_get_m_sec();
        get_python_character_manager().update();
        uint32_t t7 = el_timer_get_m_sec();
        get_effect_manager().update_sound();
        uint32_t t8 = el_timer_get_m_sec();
        get_flying_manager().update();
        uint32_t t9 = el_timer_get_m_sec();
        get_python_item().update(mouse.x, mouse.y);
        uint32_t t10 = el_timer_get_m_sec();
        get_python_player().update();
        uint32_t t11 = el_timer_get_m_sec();

        // The position changes during update, so fetch it again; otherwise the main character's
        // sounds play at the old position.
        get_python_player().new_get_main_actor_position(&p_pos_main_actor);
        set_center_position(p_pos_main_actor.x, p_pos_main_actor.y, p_pos_main_actor.z);
        uint32_t t12 = el_timer_get_m_sec();

        if (perf_checker_render_game) {
            if (t12 - t1 > 5) {
                metin2::log::debug("Update timing (ms): gu_total {} gu_gmp {} gu_scr {} gu_mps {} gu_bg {} gu_gem {} gu_chr {} gu_eft {} gu_fly {} gu_itm {} gu_plr {} gu_pos {}", t12 - t1, t2 - t1, t3 - t2, t4 - t3, t5 - t4, t6 - t5, t7 - t6, t8 - t7, t9 - t8, t10 - t9, t11 - t10, t12 - t11);
            }
        }
    }

    void python_application::skip_render_buffering(uint32_t sleep_m_sec) {
        buf_sleep_skip_time_ = el_timer_get_m_sec() + sleep_m_sec;
    }

    auto python_application::process() -> bool {
        el_timer_set_frame_m_sec();

        uint32_t start = el_timer_get_m_sec();

        static uint32_t update_frame_count = 0;
        static uint32_t render_frame_count = 0;
        static uint32_t face_count = 0;
        static uint32_t load = 0;
        static uint32_t check_time = el_timer_get_m_sec();

        if (el_timer_get_m_sec() - check_time > 1000) {
            update_fps_ = update_frame_count;
            render_fps_ = render_frame_count;
            load_ = load;

            face_count_ = face_count / std::max<uint32_t>(1, render_frame_count);

            check_time = el_timer_get_m_sec();

            load = face_count = update_frame_count = render_frame_count = 0;
        }

        timer &timer = get_timer();
        timer.advance();

        global_time_ = timer.current_second();
        global_elapsed_time_ = timer.elapsed_second();

        uint32_t updatestart = el_timer_get_m_sec();
        get_python_network_stream().process();

        get_guild_mark_uploader().process();

        get_guild_mark_downloader().process();
        get_account_connector().process();

        update_keyboard();
        const point mouse = window::mouse_position();
        on_mouse_move(mouse.x, mouse.y);
        // Workaround for a crash in SetTransform during Alt+Tab.
        update_camera();
        get_resource_manager().update();
        on_camera_update();
        on_mouse_update();
        on_ui_update();

        cur_update_time_ = el_timer_get_m_sec() - updatestart;

        // Every frame is rendered: the simulation above took the real elapsed time, so a slow frame
        // simply covers more of it instead of being followed by catch-up updates without a render.
        {
            // specular_spd is the scroll per frame at 60 fps.
            const double specular_step = specular_spd * global_elapsed_time_ * 60.0;
            material::translate_specular_matrix(static_cast<float>(specular_step), static_cast<float>(specular_step), 0.0f);

            uint32_t render_start_time = el_timer_get_m_sec();

            bool can_render = true;

            if (is_minimized_wnd_)
                can_render = false;

            if (!is_active()) {
                skip_render_buffering(3000);
            }

            if (!can_render) {
                skip_render_buffering(3000);
            } else {
                get_culling_manager().update();
                if (get_python_graphic().begin()) {

                    get_python_graphic().clear_depth_buffer();

                    get_python_graphic().set_interface_render_state();

                    on_ui_render();
                    on_mouse_render();

                    get_python_graphic().end();

                    get_python_graphic().show();

                    uint32_t render_end_time = el_timer_get_m_sec();

                    static uint32_t render_check_time = render_end_time;
                    static uint32_t render_range_time = 0;
                    static uint32_t render_range_frame = 0;

                    cur_render_time_ = render_end_time - render_start_time;
                    render_range_time += cur_render_time_;
                    ++render_range_frame;

                    if (render_end_time - render_check_time > 1000) {
                        ave_render_time_ = float(double(render_range_time) / double(render_range_frame));

                        render_check_time = el_timer_get_m_sec();
                        render_range_time = 0;
                        render_range_frame = 0;
                    }

                    uint32_t cur_face_count = get_python_graphic().face_count();
                    get_python_graphic().reset_face_count();
                    face_count += cur_face_count;

                    if (cur_face_count > 5000) {
                        if (render_end_time > buf_sleep_skip_time_) {
                            static float s_f_buf_render_time = 0.0f;

                            float cur_render_time = cur_render_time_;

                            if (cur_render_time > s_f_buf_render_time) {
                                float ratio = f_max(0.5f, (cur_render_time - s_f_buf_render_time) / 30.0f);
                                s_f_buf_render_time = (s_f_buf_render_time * (100.0f - ratio) + (cur_render_time + 5) * ratio) / 100.0f;
                            } else {
                                float ratio = 0.5f;
                                s_f_buf_render_time = (s_f_buf_render_time * (100.0f - ratio) + cur_render_time * ratio) / 100.0f;
                            }

                            if (s_f_buf_render_time > 100.0f)
                                s_f_buf_render_time = 100.0f;

                            uint32_t dw_buf_render_time = s_f_buf_render_time;

                            if (is_windowed_) {
                                if (dw_buf_render_time > 58)
                                    dw_buf_render_time = 64;
                                else if (dw_buf_render_time > 42)
                                    dw_buf_render_time = 48;
                                else if (dw_buf_render_time > 26)
                                    dw_buf_render_time = 32;
                                else if (dw_buf_render_time > 10)
                                    dw_buf_render_time = 16;
                                else
                                    dw_buf_render_time = 8;
                            }

                            ave_render_time_ = s_f_buf_render_time;
                        }

                        face_acc_count_ += cur_face_count;
                        face_acc_time_ += cur_render_time_;

                        // Render times are whole milliseconds, so fast frames add nothing to the time.
                        if (face_acc_time_ > 0) {
                            face_spd_ = static_cast<float>(face_acc_count_ / face_acc_time_);
                        }

                        if (-1 == force_sight_range_) {
                            // A moving average that moved 0.3% per frame at 60 fps, now per time.
                            static float ave_render_time = 16.0f;
                            const float weight = 1.0f - std::pow(1.0f - 0.003f, static_cast<float>(global_elapsed_time_) * 60.0f);
                            ave_render_time += (std::max(16.0f, float(cur_render_time_)) - ave_render_time) * weight;

                            float far_value = 25600.0f;
                            float near_value = min_fog_distance;
                            auto db_ave_pow = double(1000.0f / ave_render_time);
                            double db_max_pow = 60.0;
                            float distance = std::max<double>(near_value + (far_value - near_value) * (db_ave_pow) / db_max_pow, near_value);
                            get_python_background().set_view_distance_set(0, distance);
                        } else {
                            get_python_background().set_view_distance_set(0, float(force_sight_range_));
                        }
                    } else {
                        // Below 5000 faces, use the furthest view distance.
                        get_python_background().set_view_distance_set(0, 25600.0f);
                    }

                    ++render_frame_count;
                }
            }
        }

        ++update_frame_count;

        load += el_timer_get_m_sec() - start;
        const uint32_t before_limit = el_timer_get_m_sec();
        limit_frame_rate();
        load -= el_timer_get_m_sec() - before_limit;
        return true;
    }

    void python_application::limit_frame_rate() {
        using clock = std::chrono::steady_clock;

        // A minimized window has nothing to show, so it idles at a low rate whatever the setting.
        const int limit = is_minimized_wnd_ ? 30 : get_python_system().fps_limit();
        if (limit <= 0) {
            next_frame_deadline_ = {};
            return;
        }

        const auto period = std::chrono::nanoseconds(1'000'000'000 / limit);
        const clock::time_point now = clock::now();
        // After a stall, or when the cap was just enabled, the schedule restarts from now.
        if (next_frame_deadline_ == clock::time_point{} || next_frame_deadline_ + period < now)
            next_frame_deadline_ = now;
        next_frame_deadline_ += period;

        // sleep_until overshoots by a scheduler tick, so the last millisecond is spun.
        if (next_frame_deadline_ - now > std::chrono::milliseconds(2))
            std::this_thread::sleep_until(next_frame_deadline_ - std::chrono::milliseconds(1));
        while (clock::now() < next_frame_deadline_)
            std::this_thread::yield();
    }

    void python_application::set_mouse_handler(PyObject *mouse_handler) {
        mouse_handler_ = mouse_handler;
    }

    auto python_application::create_device(int width_value, int height) -> bool {
        if (!grp_device_.create(width_value, height, get_python_system().renderer())) {
            PyErr_SetString(PyExc_RuntimeError, "Cannot create the graphics device");
            metin2::log::error("Cannot create the graphics device");
            return false;
        }
        return true;
    }

    auto load_locale_data() -> bool {
        python_non_player &npc_mgr = get_python_non_player();
        item_manager &item_mgr = get_item_manager();
        python_skill &skill_mgr = get_python_skill();
        python_network_stream &net_stream = get_python_network_stream();

        const auto locale_file = [](const char *name) { return std::format("{}{}", LOCALE_DIR, name); };
        const std::string item_list = locale_file("item_list.txt");
        const std::string item_proto = locale_file("item_proto");
        const std::string item_desc = locale_file("itemdesc.txt");
        const std::string mob_proto = locale_file("mob_proto");
        const std::string skill_desc_file_name = locale_file("SkillDesc.txt");
        const std::string skill_table_file_name = locale_file("SkillTable.txt");
        const std::string insult_list = locale_file("insult.txt");

        npc_mgr.destroy();
        item_mgr.destroy();
        skill_mgr.destroy();

        if (!item_mgr.load_item_list(item_list.c_str())) {
            metin2::log::error("Cannot load the item list {}", item_list);
        }

        if (!item_mgr.load_item_table(item_proto.c_str())) {
            metin2::log::error("Cannot load the item proto {}", item_proto);
            return false;
        }

        if (!item_mgr.load_item_desc(item_desc.c_str())) {
            metin2::log::debug("Cannot load the item descriptions {}", item_desc);
        }

        if (!npc_mgr.load_non_player_data(mob_proto.c_str())) {
            metin2::log::error("Cannot load the mob proto {}", mob_proto);
            return false;
        }

        if (!skill_mgr.register_skill_desc(skill_desc_file_name.c_str())) {
            metin2::log::error("Cannot load the skill descriptions {}", skill_desc_file_name);
            return false;
        }

        if (!skill_mgr.register_skill_table(skill_table_file_name.c_str())) {
            metin2::log::error("Cannot load the skill table {}", skill_table_file_name);
            return false;
        }

        if (!net_stream.load_insult_list(insult_list.c_str())) {
            metin2::log::debug("Cannot load the insult list {}", insult_list);
        }

        return true;
    }

    auto python_application::create(PyObject *self, const char *name, int width_value, int height, int windowed) -> bool {
        windowed = get_python_system().is_windowed() ? 1 : 0;

        width_ = width_value;
        height_ = height;

        font_face::configure(get_python_system().is_font_antialias(), get_python_system().font_scale());

        is_windowed_ = windowed != 0;
        if (!main_window::create(name, get_python_system().width(), get_python_system().height(), get_python_system().is_vsync(), !is_windowed_)) {
            metin2::log::error("Cannot create the main window");
            PyErr_SetString(PyExc_RuntimeError, "Cannot create the main window");
            return false;
        }

        // Full screen is a borderless window covering the monitor; the display mode is not changed
        // and the configured size applies to the windowed mode only.
        int window_width = get_python_system().width();
        int window_height = get_python_system().height();
        if (!is_windowed_) {
            const point monitor = monitor_size();
            window_width = monitor.x;
            window_height = monitor.y;
        }
        width_ = window_width;
        height_ = window_height;
        set_size(window_width, window_height);
        set_position(0, 0);

        if (!create_cursors()) {
            metin2::log::error("Cannot create the cursors");
            PyErr_SetString(PyExc_RuntimeError, "Cannot create the cursors");
            return false;
        }

        if (!get_python_system().is_no_sound_card()) {
            if (!get_sound_manager().create()) {
                // Run without sound when the sound device cannot be created.
            }
        }

        if (!create_device(window_width, window_height))
            return false;

        // Window events are also collected after every frame, so the harness and the window state
        // keep up while a long load renders progress without returning to the main loop.
        graphic_base::device().set_present_hooks([] { dev::on_present(); }, [] { window::on_frame_presented(); });

        granny_create_shared_deform_buffer();

        show();

        get_python_system().display_settings();

        if (get_python_system().is_software_cursor())
            set_cursor_mode(CURSOR_MODE_SOFTWARE);
        else
            set_cursor_mode(CURSOR_MODE_HARDWARE);

        get_python_item().create();

        default_font_startup();

        get_python_text_tail().initialize();

        get_light_manager().initialize();

        graphic_image_instance::create_system(32);

        material::create_sphere_map(0, "special:spheremap.jpg");
        material::create_sphere_map(1, "special:spheremap01.jpg");
        return true;
    }

    void python_application::set_global_center_position(int32_t x, int32_t y) {
        python_background &bg = get_python_background();
        bg.global_position_to_local_position(x, y);

        float z = get_python_background().height(x, y);

        get_python_application().set_center_position(x, y, z);
    }

    void python_application::set_center_position(float fx, float fy, float fz) {
        center_position_.x = +fx;
        center_position_.y = -fy;
        center_position_.z = +fz;
    }

    void python_application::center_position(pixel_position *pixel_position) {
        pixel_position->x = +center_position_.x;
        pixel_position->y = -center_position_.y;
        pixel_position->z = +center_position_.z;
    }

    void python_application::set_server_time(time_t time_value) {
        start_local_time_ = el_timer_get_m_sec();
        server_time_ = time_value;
        local_start_time_ = time(nullptr);
    }

    auto python_application::server_time() -> time_t {
        return (el_timer_get_m_sec() - start_local_time_) + server_time_;
    }

    // Mall item times are created by the server with time(0), so this uses the same unit.
    auto python_application::server_time_stamp() -> time_t {
        return (time(nullptr) - local_start_time_) + server_time_;
    }

    auto python_application::global_time() -> float {
        return global_time_;
    }

    auto python_application::global_elapsed_time() -> float {
        return global_elapsed_time_;
    }

    auto python_application::width() -> int {
        return width_;
    }

    auto python_application::height() -> int {
        return height_;
    }

    void python_application::enable_special_camera_mode() {
        is_special_camera_mode_ = true;
    }

    void python_application::set_camera_speed(int percentage) {
        camera_rotate_speed_ = default_camera_rotate_speed * float(percentage) / 100.0f;
        camera_pitch_speed_ = default_camera_pitch_speed * float(percentage) / 100.0f;
        camera_zoom_speed_ = default_camera_zoom_speed * float(percentage) / 100.0f;
    }

    void python_application::set_force_sight_range(int range) {
        force_sight_range_ = range;
    }

    void python_application::clear() {
        get_python_system().clear();
    }

    void python_application::destroy() {

        material::destroy_sphere_map();

        widgets::get_window_manager().destroy();

        get_python_system().save_config();

        destroy_collision_instance_system();

        get_python_system().save_interface_status();

        get_python_event_manager().destroy();
        get_flying_manager().destroy();

        get_python_mini_map().destroy();

        get_python_text_tail().destroy();
        get_python_chat().destroy();
        get_python_character_manager().destroy();
        get_race_manager().destroy();

        get_python_item().destroy();
        get_item_manager().destroy();

        get_python_background().destroy();

        get_effect_manager().destroy();
        get_light_manager().destroy();

        default_font_cleanup();

        granny_destroy_shared_deform_buffer();

        get_python_graphic().destroy();

        get_python_resource().destroy();

        get_guild_mark_downloader().disconnect();

        model_instance::destroy_system();
        graphic_image_instance::destroy_system();

        get_sound_manager().destroy();
        grp_device_.destroy();

        attribute_instance::destroy_system();
        text_file_loader::destroy_system();
        destroy_cursors();

        main_window::destroy();
    }
}

namespace metin2::inline client::inline app {

    auto get_abstract_application() -> abstract_application & {
        return get_python_application();
    }
}
