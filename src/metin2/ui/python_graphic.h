#pragma once

#include <metin2/graphics/grp_expanded_image_instance.h>
#include <metin2/graphics/grp_image_instance.h>
#include <metin2/graphics/grp_mark_instance.h>
#include <metin2/graphics/grp_text_instance.h>
#include <metin2/models/thing_instance.h>

#include <stack>

namespace metin2::inline ui {
    class python_graphic : public screen {
      public:
        python_graphic();

        ~python_graphic() override;

        void destroy();

        void push_state();
        void pop_state();

        auto ortho_depth() -> float;
        void set_interface_render_state();
        void set_game_render_state();

        // The interface is laid out in logical pixels and drawn magnified by an integer factor, so it
        // stays legible on large displays. Window pixels = interface pixels * scale.
        void set_interface_scale(int scale) { interface_scale_ = scale < 1 ? 1 : scale; }
        auto interface_scale() const -> int { return interface_scale_; }
        // A world position in interface pixels (project_position gives window pixels).
        void project_position_to_interface(float x, float y, float z, float *px, float *py, float *pz = nullptr);

        // In interface pixels.
        void set_cursor_position(int x, int y);

        void set_omni_light();

        void set_viewport(float fx, float fy, float width_value, float height);
        void restore_viewport();

        auto generate_color(float r, float g, float b, float a) -> int32_t;

        void render_cool_time_box(float fx_center, float fy_center, float radius, float time);

        auto save_screen_shot(const char *file_name) -> bool;

      protected:
        struct state {
            math::matrix view;
            math::matrix proj;
        };

        uint32_t light_color_;
        uint32_t dark_color_;

      protected:
        std::stack<state> state_stack_;

        math::matrix save_world_matrix_;

        gpu::viewport backup_viewport_;

        float ortho_depth_;
        int interface_scale_ = 1;
    };

    inline auto get_python_graphic() -> python_graphic & {
        static python_graphic instance;
        return instance;
    }
}
