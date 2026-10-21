#pragma once

#include "color_transition_helper.h"
#include "grp_base.h"
#include "grp_image_instance.h"
#include "grp_screen.h"

#include <map>
#include <string>

namespace metin2::inline graphics {

    struct color {
        color(float r_value = 0.0f, float g_value = 0.0f, float b_value = 0.0f, float a_value = 0.0f) : r(r_value), g(g_value), b(b_value), a(a_value) {}
        float r, g, b, a;
    };

    struct gradient_color {
        color first_color;
        color second_color;
    };

    using vector_gradient_color = std::vector<gradient_color>;
    using vector_gradient_iterator = vector_gradient_color::iterator;

    class sky_object_quad {
      public:
        sky_object_quad();
        virtual ~sky_object_quad();

        void clear(const unsigned char &ruc_num_vertex,
            const float &red,
            const float &green,
            const float &blue,
            const float &alpha);
        void set_src_color(const unsigned char &ruc_num_vertex,
            const float &red,
            const float &green,
            const float &blue,
            const float &alpha);
        void set_transition(const unsigned char &ruc_num_vertex,
            const float &red,
            const float &green,
            const float &blue,
            const float &alpha,
            uint32_t duration);

        void set_vertex(const unsigned char &ruc_num_vertex, const pdt_vertex &pdt_vertex);

        void start_transition();
        auto update() -> bool;
        void render();

      private:
        pdt_vertex vertex_value_[4];
        index indices_[4];
        color_transition_helper helper_[4];
    };

    class sky_object : public screen {
      public:
        enum {
            SKY_RENDER_MODE_DEFAULT,
            SKY_RENDER_MODE_DIFFUSE,
            SKY_RENDER_MODE_TEXTURE,
            SKY_RENDER_MODE_MODULATE,
            SKY_RENDER_MODE_MODULATE2X,
            SKY_RENDER_MODE_MODULATE4X,
        };

        sky_object();
        ~sky_object() override;

        virtual void destroy();

        virtual void render() = 0;
        virtual void update() = 0;
        virtual void start_transition();

        void set_render_mode(unsigned char render_mode) { render_mode_value_ = render_mode; }

        auto is_transition_started() -> const bool & { return transition_started_; }

      protected:
        auto generate_texture(const char *szfilename) -> graphic_image_instance *;
        void delete_texture(graphic_image_instance *graphic_image_instance);

      protected:
        using sky_object_quad_vector = std::vector<sky_object_quad>;
        using sky_object_quad_iterator = sky_object_quad_vector::iterator;

        struct sky_box {
            void start_transition();
            auto update() -> bool;
            void render();
            std::string strfacename;
            std::string face_texture_file_name;
            sky_object_quad_vector sky_object_quad_vector_value;
        };
        using sky_object_face = sky_box;

        using graphic_image_instance_map = std::map<std::string, graphic_image_instance *>;

        sky_object_face face_cloud_;
        math::matrix world_cloud_, translation_cloud_, texture_cloud_;
        math::vector3 position_cloud_;
        float cloud_scale_x_, cloud_scale_y_, cloud_height_;
        float cloud_texture_scale_x_, cloud_texture_scale_y_;
        float cloud_scroll_speed_u_, cloud_scroll_speed_v_;
        float cloud_position_u_, cloud_position_v_;

        uint32_t dwlast_time_;

        graphic_image_instance_map graphic_image_instance_map_value_;

        math::matrix world_, translation_;
        math::vector3 position_value_;
        float scale_x_, scale_y_, scale_z_;

        unsigned char render_mode_value_;

        std::string cur_time_;
        bool transition_started_;
        bool sky_matrix_updated_;

        graphic_image_instance cloud_alpha_image_instance_;
    };

    class sky_box : public sky_object {
      public:
        sky_box();
        ~sky_box() override;

        void update() override;
        void render() override;
        void render_cloud();

        void destroy() override;
        void unload();

        void set_sky_box_scale(const math::vector3 &scale);
        void set_gradient_level(uint8_t upper, uint8_t lower);
        void set_face_texture(const char *file_name, int face_index);
        void set_cloud_texture(const char *file_name);
        void set_cloud_scale(const math::vector2 &cloud_scale);
        void set_cloud_height(float height);
        void set_cloud_texture_scale(const math::vector2 &cloud_texture_scale);
        void set_cloud_scroll_speed(const math::vector2 &cloud_scroll_speed);
        void set_cloud_color(const gradient_color &color, const gradient_color &next_color, const uint32_t &transition_time);
        void refresh();
        void set_sky_color(const vector_gradient_color &color_vector, const vector_gradient_color &next_color_vector, int32_t transition_time);
        void start_transition() override;

      protected:
        void set_sky_object_quad_vertical(sky_object_quad_vector *sky_object_quad_vector, const math::vector2 *quad_points);
        void set_sky_object_quad_horizon(sky_object_quad_vector *sky_object_quad_vector, const math::vector3 *quad_points);

      protected:
        unsigned char virtical_gradient_level_upper_;
        unsigned char virtical_gradient_level_lower_;
        sky_object_face faces_[6];
    };
}
