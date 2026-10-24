#include "sky_box.h"
#include "camera.h"
#include "resource_manager.h"
#include "state_manager.h"

#include <metin2/core/timer.h>
#include <metin2/gpu/types.h>

namespace metin2::inline graphics {

    sky_object_quad::sky_object_quad() {
        indices_[0] = 0;
        indices_[1] = 2;
        indices_[2] = 1;
        indices_[3] = 3;

        for (unsigned char uci = 0; uci < 4; ++uci) {
            memset(static_cast<void *>(&vertex_value_[uci]), 0, sizeof(pdt_vertex));
        }
    }

    sky_object_quad::~sky_object_quad() = default;

    void sky_object_quad::clear(const unsigned char &ruc_num_vertex,
        const float &red,
        const float &green,
        const float &blue,
        const float &alpha) {
        if (ruc_num_vertex > 3)
            return;
        helper_[ruc_num_vertex].clear(red, green, blue, alpha);
    }

    void sky_object_quad::set_src_color(const unsigned char &ruc_num_vertex,
        const float &red,
        const float &green,
        const float &blue,
        const float &alpha) {
        if (ruc_num_vertex > 3)
            return;
        helper_[ruc_num_vertex].set_src_color(red, green, blue, alpha);
    }

    void sky_object_quad::set_transition(const unsigned char &ruc_num_vertex,
        const float &red,
        const float &green,
        const float &blue,
        const float &alpha,
        uint32_t duration) {
        if (ruc_num_vertex > 3)
            return;
        helper_[ruc_num_vertex].set_transition(red, green, blue, alpha, duration);
    }

    void sky_object_quad::set_vertex(const unsigned char &ruc_num_vertex, const pdt_vertex &pdt_vertex) {
        if (ruc_num_vertex > 3)
            return;
        memcpy(&vertex_value_[indices_[ruc_num_vertex]], &pdt_vertex, sizeof(pdt_vertex));
    }

    void sky_object_quad::start_transition() {
        for (unsigned char uci = 0; uci < 4; ++uci) {
            helper_[uci].start_transition();
        }
    }

    auto sky_object_quad::update() -> bool {
        bool result = false;
        for (unsigned char uci = 0; uci < 4; ++uci) {
            result = helper_[uci].update() || result;
            vertex_value_[indices_[uci]].diffuse_value = helper_[uci].cur_color();
        }
        return result;
    }

    void sky_object_quad::render() {
        if (graphic_base::set_pdt_stream(vertex_value_, 4))
            STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_STRIP, 0, 2);
    }

    sky_object::sky_object() : position_value_(0.0f, 0.0f, 0.0f),
                               scale_x_(1.0f),
                               scale_y_(1.0f),
                               scale_z_(1.0f) {
        world_ = math::matrix_identity();
        translation_ = math::matrix_identity();
        texture_cloud_ = math::matrix_identity();

        dwlast_time_ = get_timer().current_millisecond();

        cloud_position_u_ = 0.0f;
        cloud_position_v_ = 0.0f;

        transition_started_ = false;
        sky_matrix_updated_ = false;
    }

    sky_object::~sky_object() {
        destroy();
    }

    void sky_object::destroy() {
    }

    void sky_object::update() {
        math::vector3 eye = get_camera_manager().current_camera()->eye();

        if (position_value_ == eye)
            if (sky_matrix_updated_ == false)
                return;

        position_value_ = eye;

        world_._41 = position_value_.x;
        world_._42 = position_value_.y;
        world_._43 = position_value_.z;

        world_cloud_._41 = position_value_.x;
        world_cloud_._42 = position_value_.y;
        world_cloud_._43 = position_value_.z + cloud_height_;

        if (sky_matrix_updated_)
            sky_matrix_updated_ = false;
    }

    void sky_object::render() {
    }

    auto sky_object::generate_texture(const char *szfilename) -> graphic_image_instance * {
        assert(szfilename != nullptr);

        if (strlen(szfilename) <= 0)
            assert(false);

        resource *resource = get_resource_manager().resource_pointer(szfilename);

        if (!resource->is_type(graphic_image::type())) {
            assert(false);
            return nullptr;
        }

        graphic_image_instance *image_instance = graphic_image_instance::new_value();
        image_instance->set_image_pointer(static_cast<graphic_image *>(resource));
        return (image_instance);
    }

    void sky_object::delete_texture(graphic_image_instance *image_instance) {
        if (image_instance)
            graphic_image_instance::delete_value(image_instance);
    }

    void sky_object::start_transition() {
    }

    void sky_object::sky_object_face::start_transition() {
        for (unsigned char uci = 0; uci < sky_object_quad_vector_value.size(); ++uci) {
            sky_object_quad_vector_value[uci].start_transition();
        }
    }

    auto sky_object::sky_object_face::update() -> bool {
        bool result = false;
        for (uint32_t dwi = 0; dwi < sky_object_quad_vector_value.size(); ++dwi)
            result = sky_object_quad_vector_value[dwi].update() || result;
        return result;
    }

    void sky_object::sky_object_face::render() {
        for (unsigned char uci = 0; uci < sky_object_quad_vector_value.size(); ++uci) {
            sky_object_quad_vector_value[uci].render();
        }
    }

    sky_box::sky_box() {
        virtical_gradient_level_upper_ = 0;
        virtical_gradient_level_lower_ = 0;
    }

    sky_box::~sky_box() {
        destroy();
    }

    void sky_box::destroy() {
        unload();
    }

    void sky_box::set_sky_box_scale(const math::vector3 &scale) {
        scale_x_ = scale.x;
        scale_y_ = scale.y;
        scale_z_ = scale.z;

        sky_matrix_updated_ = true;
        world_ = math::matrix_scaling(scale_x_, scale_y_, scale_z_);
    }

    void sky_box::set_gradient_level(uint8_t upper, uint8_t lower) {
        virtical_gradient_level_upper_ = upper;
        virtical_gradient_level_lower_ = lower;
    }

    void sky_box::set_face_texture(const char *file_name, int face_index) {
        if (face_index < 0 || face_index > 5)
            return;

        auto itor = graphic_image_instance_map_value_.find(file_name);
        if (graphic_image_instance_map_value_.end() != itor)
            return;

        faces_[face_index].face_texture_file_name = file_name;

        graphic_image_instance *graphic_image_instance = generate_texture(file_name);
        graphic_image_instance_map_value_.insert(graphic_image_instance_map::value_type(file_name, graphic_image_instance));
    }

    void sky_box::set_cloud_texture(const char *file_name) {
        auto itor = graphic_image_instance_map_value_.find(file_name);
        if (graphic_image_instance_map_value_.end() != itor)
            return;

        face_cloud_.strfacename = file_name;
        graphic_image_instance *graphic_image_instance = generate_texture(file_name);
        graphic_image_instance_map_value_.insert(graphic_image_instance_map::value_type(face_cloud_.strfacename, graphic_image_instance));
    }

    void sky_box::set_cloud_scale(const math::vector2 &cloud_scale) {
        cloud_scale_x_ = cloud_scale.x;
        cloud_scale_y_ = cloud_scale.y;

        world_cloud_ = math::matrix_scaling(cloud_scale_x_, cloud_scale_y_, 1.0f);
    }

    void sky_box::set_cloud_height(float height) {
        cloud_height_ = height;
    }

    void sky_box::set_cloud_texture_scale(const math::vector2 &cloud_texture_scale) {
        cloud_texture_scale_x_ = cloud_texture_scale.x;
        cloud_texture_scale_y_ = cloud_texture_scale.y;

        texture_cloud_._11 = cloud_texture_scale_x_;
        texture_cloud_._22 = cloud_texture_scale_y_;
    }

    void sky_box::set_cloud_scroll_speed(const math::vector2 &cloud_scroll_speed) {
        cloud_scroll_speed_u_ = cloud_scroll_speed.x;
        cloud_scroll_speed_v_ = cloud_scroll_speed.y;
    }

    void sky_box::unload() {
        auto itor = graphic_image_instance_map_value_.begin();

        while (itor != graphic_image_instance_map_value_.end()) {
            delete_texture(itor->second);
            ++itor;
        }

        graphic_image_instance_map_value_.clear();
    }

    void sky_box::set_sky_object_quad_vertical(sky_object_quad_vector *sky_object_quad_vector, const math::vector2 *quad_points) {
        pdt_vertex pdt_vertex;

        uint32_t index = 0;

        sky_object_quad_vector->clear();
        sky_object_quad_vector->resize(virtical_gradient_level_upper_ + virtical_gradient_level_lower_);

        unsigned char y;
        for (y = 0; y < virtical_gradient_level_upper_; ++y) {
            sky_object_quad &sky_object_quad = sky_object_quad_vector->at(index++);

            pdt_vertex.position_value.x = quad_points[0].x;
            pdt_vertex.position_value.y = quad_points[0].y;
            pdt_vertex.position_value.z = 1.0f - (float)(y + 1) / (float)(virtical_gradient_level_upper_);
            pdt_vertex.tex_coord.x = 0.0f;
            pdt_vertex.tex_coord.y = (float)(y + 1) / (float)(virtical_gradient_level_upper_) * 0.5f;
            sky_object_quad.set_vertex(0, pdt_vertex);
            pdt_vertex.position_value.x = quad_points[0].x;
            pdt_vertex.position_value.y = quad_points[0].y;
            pdt_vertex.position_value.z = 1.0f - (float)(y) / (float)(virtical_gradient_level_upper_);
            pdt_vertex.tex_coord.x = 0.0f;
            pdt_vertex.tex_coord.y = (float)(y) / (float)(virtical_gradient_level_upper_) * 0.5f;
            sky_object_quad.set_vertex(1, pdt_vertex);
            pdt_vertex.position_value.x = quad_points[1].x;
            pdt_vertex.position_value.y = quad_points[1].y;
            pdt_vertex.position_value.z = 1.0f - (float)(y + 1) / (float)(virtical_gradient_level_upper_);
            pdt_vertex.tex_coord.x = 1.0f;
            pdt_vertex.tex_coord.y = (float)(y + 1) / (float)(virtical_gradient_level_upper_) * 0.5f;
            sky_object_quad.set_vertex(2, pdt_vertex);
            pdt_vertex.position_value.x = quad_points[1].x;
            pdt_vertex.position_value.y = quad_points[1].y;
            pdt_vertex.position_value.z = 1.0f - (float)(y) / (float)(virtical_gradient_level_upper_);
            pdt_vertex.tex_coord.x = 1.0f;
            pdt_vertex.tex_coord.y = (float)(y) / (float)(virtical_gradient_level_upper_) * 0.5f;
            sky_object_quad.set_vertex(3, pdt_vertex);
        }
        for (y = 0; y < virtical_gradient_level_lower_; ++y) {
            sky_object_quad &sky_object_quad = sky_object_quad_vector->at(index++);

            pdt_vertex.position_value.x = quad_points[0].x;
            pdt_vertex.position_value.y = quad_points[0].y;
            pdt_vertex.position_value.z = -(float)(y + 1) / (float)(virtical_gradient_level_lower_);
            pdt_vertex.tex_coord.x = 0.0f;
            pdt_vertex.tex_coord.y = 0.5f + (float)(y + 1) / (float)(virtical_gradient_level_upper_) * 0.5f;
            sky_object_quad.set_vertex(0, pdt_vertex);
            pdt_vertex.position_value.x = quad_points[0].x;
            pdt_vertex.position_value.y = quad_points[0].y;
            pdt_vertex.position_value.z = -(float)(y) / (float)(virtical_gradient_level_lower_);
            pdt_vertex.tex_coord.x = 0.0f;
            pdt_vertex.tex_coord.y = 0.5f + (float)(y) / (float)(virtical_gradient_level_upper_) * 0.5f;
            sky_object_quad.set_vertex(1, pdt_vertex);
            pdt_vertex.position_value.x = quad_points[1].x;
            pdt_vertex.position_value.y = quad_points[1].y;
            pdt_vertex.position_value.z = -(float)(y + 1) / (float)(virtical_gradient_level_lower_);
            pdt_vertex.tex_coord.x = 1.0f;
            pdt_vertex.tex_coord.y = 0.5f + (float)(y + 1) / (float)(virtical_gradient_level_upper_) * 0.5f;
            sky_object_quad.set_vertex(2, pdt_vertex);
            pdt_vertex.position_value.x = quad_points[1].x;
            pdt_vertex.position_value.y = quad_points[1].y;
            pdt_vertex.position_value.z = -(float)(y) / (float)(virtical_gradient_level_lower_);
            pdt_vertex.tex_coord.x = 1.0f;
            pdt_vertex.tex_coord.y = 0.5f + (float)(y) / (float)(virtical_gradient_level_upper_) * 0.5f;
            sky_object_quad.set_vertex(3, pdt_vertex);
        }
    }

    void sky_box::set_sky_object_quad_horizon(sky_object_quad_vector *sky_object_quad_vector, const math::vector3 *quad_points) {
        sky_object_quad_vector->clear();
        sky_object_quad_vector->resize(1);
        sky_object_quad &sky_object_quad = sky_object_quad_vector->at(0);

        pdt_vertex pdt_vertex;
        pdt_vertex.position_value = quad_points[0];
        pdt_vertex.tex_coord.x = 0.0f;
        pdt_vertex.tex_coord.y = 1.0f;
        sky_object_quad.set_vertex(0, pdt_vertex);

        pdt_vertex.position_value = quad_points[1];
        pdt_vertex.tex_coord.x = 0.0f;
        pdt_vertex.tex_coord.y = 0.0f;
        sky_object_quad.set_vertex(1, pdt_vertex);

        pdt_vertex.position_value = quad_points[2];
        pdt_vertex.tex_coord.x = 1.0f;
        pdt_vertex.tex_coord.y = 1.0f;
        sky_object_quad.set_vertex(2, pdt_vertex);

        pdt_vertex.position_value = quad_points[3];
        pdt_vertex.tex_coord.x = 1.0f;
        pdt_vertex.tex_coord.y = 0.0f;
        sky_object_quad.set_vertex(3, pdt_vertex);
    }

    void sky_box::refresh() {
        math::vector3 v3_quad_points[4];

        if (render_mode_value_ == sky_object::SKY_RENDER_MODE_DEFAULT || render_mode_value_ == sky_object::SKY_RENDER_MODE_DIFFUSE) {
            if (virtical_gradient_level_upper_ + virtical_gradient_level_lower_ <= 0)
                return;

            math::vector2 v2_quad_points[2];

            v2_quad_points[0] = math::vector2(1.0f, -1.0f);
            v2_quad_points[1] = math::vector2(-1.0f, -1.0f);
            set_sky_object_quad_vertical(&faces_[0].sky_object_quad_vector_value, v2_quad_points);
            faces_[0].strfacename = "front";

            v2_quad_points[0] = math::vector2(-1.0f, 1.0f);
            v2_quad_points[1] = math::vector2(1.0f, 1.0f);
            set_sky_object_quad_vertical(&faces_[1].sky_object_quad_vector_value, v2_quad_points);
            faces_[1].strfacename = "back";

            v2_quad_points[0] = math::vector2(-1.0f, -1.0f);
            v2_quad_points[1] = math::vector2(-1.0f, 1.0f);
            set_sky_object_quad_vertical(&faces_[2].sky_object_quad_vector_value, v2_quad_points);
            faces_[2].strfacename = "left";

            v2_quad_points[0] = math::vector2(1.0f, 1.0f);
            v2_quad_points[1] = math::vector2(1.0f, -1.0f);
            set_sky_object_quad_vertical(&faces_[3].sky_object_quad_vector_value, v2_quad_points);
            faces_[3].strfacename = "right";

            v3_quad_points[0] = math::vector3(1.0f, 1.0f, 1.0f);
            v3_quad_points[1] = math::vector3(-1.0f, 1.0f, 1.0f);
            v3_quad_points[2] = math::vector3(1.0f, -1.0f, 1.0f);
            v3_quad_points[3] = math::vector3(-1.0f, -1.0f, 1.0f);
            set_sky_object_quad_horizon(&faces_[4].sky_object_quad_vector_value, v3_quad_points);
            faces_[4].strfacename = "top";

            v3_quad_points[0] = math::vector3(-1.0f, 1.0f, -1.0f);
            v3_quad_points[1] = math::vector3(1.0f, 1.0f, -1.0f);
            v3_quad_points[2] = math::vector3(-1.0f, -1.0f, -1.0f);
            v3_quad_points[3] = math::vector3(1.0f, -1.0f, -1.0f);
            set_sky_object_quad_horizon(&faces_[5].sky_object_quad_vector_value, v3_quad_points);
            faces_[5].strfacename = "bottom";

        } else if (render_mode_value_ == sky_object::SKY_RENDER_MODE_TEXTURE) {
            v3_quad_points[0] = math::vector3(1.0f, -1.0f, -1.0f);
            v3_quad_points[1] = math::vector3(1.0f, -1.0f, 1.0f);
            v3_quad_points[2] = math::vector3(-1.0f, -1.0f, -1.0f);
            v3_quad_points[3] = math::vector3(-1.0f, -1.0f, 1.0f);

            set_sky_object_quad_horizon(&faces_[0].sky_object_quad_vector_value, v3_quad_points);
            faces_[0].strfacename = "front";

            v3_quad_points[0] = math::vector3(-1.0f, 1.0f, -1.0f);
            v3_quad_points[1] = math::vector3(-1.0f, 1.0f, 1.0f);
            v3_quad_points[2] = math::vector3(1.0f, 1.0f, -1.0f);
            v3_quad_points[3] = math::vector3(1.0f, 1.0f, 1.0f);

            set_sky_object_quad_horizon(&faces_[1].sky_object_quad_vector_value, v3_quad_points);
            faces_[1].strfacename = "back";

            v3_quad_points[0] = math::vector3(1.0f, 1.0f, -1.0f);
            v3_quad_points[1] = math::vector3(1.0f, 1.0f, 1.0f);
            v3_quad_points[2] = math::vector3(1.0f, -1.0f, -1.0f);
            v3_quad_points[3] = math::vector3(1.0f, -1.0f, 1.0f);

            set_sky_object_quad_horizon(&faces_[2].sky_object_quad_vector_value, v3_quad_points);
            faces_[2].strfacename = "left";

            v3_quad_points[0] = math::vector3(-1.0f, -1.0f, -1.0f);
            v3_quad_points[1] = math::vector3(-1.0f, -1.0f, 1.0f);
            v3_quad_points[2] = math::vector3(-1.0f, 1.0f, -1.0f);
            v3_quad_points[3] = math::vector3(-1.0f, 1.0f, 1.0f);

            set_sky_object_quad_horizon(&faces_[3].sky_object_quad_vector_value, v3_quad_points);
            faces_[3].strfacename = "right";

            v3_quad_points[0] = math::vector3(1.0f, -1.0f, 1.0f);
            v3_quad_points[1] = math::vector3(1.0f, 1.0f, 1.0f);
            v3_quad_points[2] = math::vector3(-1.0f, -1.0f, 1.0f);
            v3_quad_points[3] = math::vector3(-1.0f, 1.0f, 1.0f);

            set_sky_object_quad_horizon(&faces_[4].sky_object_quad_vector_value, v3_quad_points);
            faces_[4].strfacename = "top";

            v3_quad_points[0] = math::vector3(1.0f, -1.0f, -1.0f);
            v3_quad_points[1] = math::vector3(1.0f, 1.0f, -1.0f);
            v3_quad_points[2] = math::vector3(-1.0f, -1.0f, -1.0f);
            v3_quad_points[3] = math::vector3(-1.0f, 1.0f, -1.0f);

            set_sky_object_quad_horizon(&faces_[5].sky_object_quad_vector_value, v3_quad_points);
            faces_[5].strfacename = "bottom";
        }

        v3_quad_points[0] = math::vector3(1.0f, 1.0f, 0.0f);
        v3_quad_points[1] = math::vector3(-1.0f, 1.0f, 0.0f);
        v3_quad_points[2] = math::vector3(1.0f, -1.0f, 0.0f);
        v3_quad_points[3] = math::vector3(-1.0f, -1.0f, 0.0f);
        set_sky_object_quad_horizon(&face_cloud_.sky_object_quad_vector_value, v3_quad_points);
    }

    void sky_box::set_cloud_color(const gradient_color &color, const gradient_color &next_color, const uint32_t &transition_time) {
        sky_object_face &face_cloud = face_cloud_;
        for (uint32_t dwk = 0; dwk < face_cloud.sky_object_quad_vector_value.size(); ++dwk) {
            sky_object_quad &sky_object_quad = face_cloud.sky_object_quad_vector_value[dwk];

            sky_object_quad.set_src_color(0,
                color.first_color.r,
                color.first_color.g,
                color.first_color.b,
                color.first_color.a);
            sky_object_quad.set_transition(0,
                next_color.first_color.r,
                next_color.first_color.g,
                next_color.first_color.b,
                next_color.first_color.a,
                transition_time);
            sky_object_quad.set_src_color(1,
                color.first_color.r,
                color.first_color.g,
                color.first_color.b,
                color.first_color.a);
            sky_object_quad.set_transition(1,
                next_color.first_color.r,
                next_color.first_color.g,
                next_color.first_color.b,
                next_color.first_color.a,
                transition_time);
            sky_object_quad.set_src_color(2,
                color.first_color.r,
                color.first_color.g,
                color.first_color.b,
                color.first_color.a);
            sky_object_quad.set_transition(2,
                next_color.first_color.r,
                next_color.first_color.g,
                next_color.first_color.b,
                next_color.first_color.a,
                transition_time);
            sky_object_quad.set_src_color(3,
                color.first_color.r,
                color.first_color.g,
                color.first_color.b,
                color.first_color.a);
            sky_object_quad.set_transition(3,
                next_color.first_color.r,
                next_color.first_color.g,
                next_color.first_color.b,
                next_color.first_color.a,
                transition_time);
        }
    }

    void sky_box::set_sky_color(const vector_gradient_color &color_vector, const vector_gradient_color &next_color_vector, int32_t transition_time) {
        uint32_t vector_gradient_colornum = 0;
        uint32_t uck;
        for (unsigned char ucj = 0; ucj < 4; ++ucj) {
            sky_object_face &face = faces_[ucj];
            vector_gradient_colornum = 0;
            for (uck = 0; uck < face.sky_object_quad_vector_value.size(); ++uck) {
                sky_object_quad &sky_object_quad = face.sky_object_quad_vector_value[uck];

                sky_object_quad.set_src_color(0,
                    color_vector[vector_gradient_colornum].second_color.r,
                    color_vector[vector_gradient_colornum].second_color.g,
                    color_vector[vector_gradient_colornum].second_color.b,
                    color_vector[vector_gradient_colornum].second_color.a);
                sky_object_quad.set_transition(0,
                    next_color_vector[vector_gradient_colornum].second_color.r,
                    next_color_vector[vector_gradient_colornum].second_color.g,
                    next_color_vector[vector_gradient_colornum].second_color.b,
                    next_color_vector[vector_gradient_colornum].second_color.a,
                    transition_time);
                sky_object_quad.set_src_color(1,
                    color_vector[vector_gradient_colornum].first_color.r,
                    color_vector[vector_gradient_colornum].first_color.g,
                    color_vector[vector_gradient_colornum].first_color.b,
                    color_vector[vector_gradient_colornum].first_color.a);
                sky_object_quad.set_transition(1,
                    next_color_vector[vector_gradient_colornum].first_color.r,
                    next_color_vector[vector_gradient_colornum].first_color.g,
                    next_color_vector[vector_gradient_colornum].first_color.b,
                    next_color_vector[vector_gradient_colornum].first_color.a,
                    transition_time);
                sky_object_quad.set_src_color(2,
                    color_vector[vector_gradient_colornum].second_color.r,
                    color_vector[vector_gradient_colornum].second_color.g,
                    color_vector[vector_gradient_colornum].second_color.b,
                    color_vector[vector_gradient_colornum].second_color.a);
                sky_object_quad.set_transition(2,
                    next_color_vector[vector_gradient_colornum].second_color.r,
                    next_color_vector[vector_gradient_colornum].second_color.g,
                    next_color_vector[vector_gradient_colornum].second_color.b,
                    next_color_vector[vector_gradient_colornum].second_color.a,
                    transition_time);
                sky_object_quad.set_src_color(3,
                    color_vector[vector_gradient_colornum].first_color.r,
                    color_vector[vector_gradient_colornum].first_color.g,
                    color_vector[vector_gradient_colornum].first_color.b,
                    color_vector[vector_gradient_colornum].first_color.a);
                sky_object_quad.set_transition(3,
                    next_color_vector[vector_gradient_colornum].first_color.r,
                    next_color_vector[vector_gradient_colornum].first_color.g,
                    next_color_vector[vector_gradient_colornum].first_color.b,
                    next_color_vector[vector_gradient_colornum].first_color.a,
                    transition_time);

                vector_gradient_colornum++;
            }
        }

        sky_object_face &face_top = faces_[4];
        vector_gradient_colornum = 0;
        for (uck = 0; uck < face_top.sky_object_quad_vector_value.size(); ++uck) {
            sky_object_quad &sky_object_quad = face_top.sky_object_quad_vector_value[uck];

            sky_object_quad.set_src_color(0,
                color_vector[vector_gradient_colornum].first_color.r,
                color_vector[vector_gradient_colornum].first_color.g,
                color_vector[vector_gradient_colornum].first_color.b,
                color_vector[vector_gradient_colornum].first_color.a);
            sky_object_quad.set_transition(0,
                next_color_vector[vector_gradient_colornum].first_color.r,
                next_color_vector[vector_gradient_colornum].first_color.g,
                next_color_vector[vector_gradient_colornum].first_color.b,
                next_color_vector[vector_gradient_colornum].first_color.a,
                transition_time);
            sky_object_quad.set_src_color(1,
                color_vector[vector_gradient_colornum].first_color.r,
                color_vector[vector_gradient_colornum].first_color.g,
                color_vector[vector_gradient_colornum].first_color.b,
                color_vector[vector_gradient_colornum].first_color.a);
            sky_object_quad.set_transition(1,
                next_color_vector[vector_gradient_colornum].first_color.r,
                next_color_vector[vector_gradient_colornum].first_color.g,
                next_color_vector[vector_gradient_colornum].first_color.b,
                next_color_vector[vector_gradient_colornum].first_color.a,
                transition_time);
            sky_object_quad.set_src_color(2,
                color_vector[vector_gradient_colornum].first_color.r,
                color_vector[vector_gradient_colornum].first_color.g,
                color_vector[vector_gradient_colornum].first_color.b,
                color_vector[vector_gradient_colornum].first_color.a);
            sky_object_quad.set_transition(2,
                next_color_vector[vector_gradient_colornum].first_color.r,
                next_color_vector[vector_gradient_colornum].first_color.g,
                next_color_vector[vector_gradient_colornum].first_color.b,
                next_color_vector[vector_gradient_colornum].first_color.a,
                transition_time);
            sky_object_quad.set_src_color(3,
                color_vector[vector_gradient_colornum].first_color.r,
                color_vector[vector_gradient_colornum].first_color.g,
                color_vector[vector_gradient_colornum].first_color.b,
                color_vector[vector_gradient_colornum].first_color.a);
            sky_object_quad.set_transition(3,
                next_color_vector[vector_gradient_colornum].first_color.r,
                next_color_vector[vector_gradient_colornum].first_color.g,
                next_color_vector[vector_gradient_colornum].first_color.b,
                next_color_vector[vector_gradient_colornum].first_color.a,
                transition_time);
        }
        sky_object_face &face_bottom = faces_[5];
        vector_gradient_colornum = color_vector.size() - 1;
        for (uck = 0; uck < face_bottom.sky_object_quad_vector_value.size(); ++uck) {
            sky_object_quad &sky_object_quad = face_bottom.sky_object_quad_vector_value[uck];

            sky_object_quad.set_src_color(0,
                color_vector[vector_gradient_colornum].second_color.r,
                color_vector[vector_gradient_colornum].second_color.g,
                color_vector[vector_gradient_colornum].second_color.b,
                color_vector[vector_gradient_colornum].second_color.a);
            sky_object_quad.set_transition(0,
                next_color_vector[vector_gradient_colornum].second_color.r,
                next_color_vector[vector_gradient_colornum].second_color.g,
                next_color_vector[vector_gradient_colornum].second_color.b,
                next_color_vector[vector_gradient_colornum].second_color.a,
                transition_time);
            sky_object_quad.set_src_color(1,
                color_vector[vector_gradient_colornum].second_color.r,
                color_vector[vector_gradient_colornum].second_color.g,
                color_vector[vector_gradient_colornum].second_color.b,
                color_vector[vector_gradient_colornum].second_color.a);
            sky_object_quad.set_transition(1,
                next_color_vector[vector_gradient_colornum].second_color.r,
                next_color_vector[vector_gradient_colornum].second_color.g,
                next_color_vector[vector_gradient_colornum].second_color.b,
                next_color_vector[vector_gradient_colornum].second_color.a,
                transition_time);
            sky_object_quad.set_src_color(2,
                color_vector[vector_gradient_colornum].second_color.r,
                color_vector[vector_gradient_colornum].second_color.g,
                color_vector[vector_gradient_colornum].second_color.b,
                color_vector[vector_gradient_colornum].second_color.a);
            sky_object_quad.set_transition(2,
                next_color_vector[vector_gradient_colornum].second_color.r,
                next_color_vector[vector_gradient_colornum].second_color.g,
                next_color_vector[vector_gradient_colornum].second_color.b,
                next_color_vector[vector_gradient_colornum].second_color.a,
                transition_time);
            sky_object_quad.set_src_color(3,
                color_vector[vector_gradient_colornum].second_color.r,
                color_vector[vector_gradient_colornum].second_color.g,
                color_vector[vector_gradient_colornum].second_color.b,
                color_vector[vector_gradient_colornum].second_color.a);
            sky_object_quad.set_transition(3,
                next_color_vector[vector_gradient_colornum].second_color.r,
                next_color_vector[vector_gradient_colornum].second_color.g,
                next_color_vector[vector_gradient_colornum].second_color.b,
                next_color_vector[vector_gradient_colornum].second_color.a,
                transition_time);
        }
    }

    void sky_box::start_transition() {
        transition_started_ = true;
        for (unsigned char ucj = 0; ucj < 6; ++ucj)
            faces_[ucj].start_transition();
        face_cloud_.start_transition();
    }

    void sky_box::update() {
        sky_object::update();

        if (!transition_started_)
            return;

        bool result = false;
        for (unsigned char uci = 0; uci < 6; ++uci)
            result = faces_[uci].update() || result;
        result = face_cloud_.update() || result;

        transition_started_ = result;
    }

    void sky_box::render() {
        // Drawn last in the frame, depth tested but without depth writes.
        STATEMANAGER.save_render_state(gpu::state::Z_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.save_render_state(gpu::state::FOG_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);

        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG2);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);

        STATEMANAGER.set_transform(gpu::transform::WORLD, &world_);

        if (render_mode_value_ == sky_object::SKY_RENDER_MODE_TEXTURE) {
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);

            for (unsigned int i = 0; i < 6; ++i) {
                graphic_image_instance *face_image_instance = graphic_image_instance_map_value_[faces_[i].face_texture_file_name];
                if (!face_image_instance)
                    break;

                STATEMANAGER.set_texture(0, face_image_instance->texture_reference().gpu_texture());

                faces_[i].render();
            }

            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ADDRESS_U);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ADDRESS_V);
        } else {
            for (unsigned int i = 0; i < 6; ++i) {
                faces_[i].render();
            }
        }

        STATEMANAGER.restore_render_state(gpu::state::LIGHTING);
        STATEMANAGER.restore_render_state(gpu::state::Z_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::FOG_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_OP);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG1);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG2);
    }

    void sky_box::render_cloud() {
        graphic_image_instance *cloud_graphic_image_instance = graphic_image_instance_map_value_[face_cloud_.strfacename];
        if (!cloud_graphic_image_instance)
            return;

        // Drawn last in the frame, depth tested but without depth writes.
        STATEMANAGER.save_render_state(gpu::state::Z_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.save_render_state(gpu::state::FOG_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::ONE);
        STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_COLOR);

        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::COUNT2);

        texture_cloud_._31 = cloud_position_u_;
        texture_cloud_._32 = cloud_position_v_;

        uint32_t cur_time = get_timer().current_millisecond();

        cloud_position_u_ += cloud_scroll_speed_u_ * (float)(cur_time - dwlast_time_) * 0.001f;
        if (cloud_position_u_ >= 1.0f)
            cloud_position_u_ = 0.0f;

        cloud_position_v_ += cloud_scroll_speed_v_ * (float)(cur_time - dwlast_time_) * 0.001f;
        if (cloud_position_v_ >= 1.0f)
            cloud_position_v_ = 0.0f;

        dwlast_time_ = cur_time;

        STATEMANAGER.save_transform(gpu::transform::TEXTURE0, &texture_cloud_);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE_INV_ALPHA_ADD_COLOR);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);

        math::matrix proj_cloud;
        // The cloud layer was authored for a 45° vertical field of view; the real aspect keeps it from
        // stretching on wide screens.
        proj_cloud = math::matrix_perspective_fov_rh(math::PI * 0.25f, aspect_, 50.0f, 999999.0f);
        STATEMANAGER.set_transform(gpu::transform::WORLD, &world_cloud_);
        STATEMANAGER.save_transform(gpu::transform::PROJECTION, &proj_cloud);
        STATEMANAGER.set_texture(0, cloud_graphic_image_instance->texture_pointer()->gpu_texture());
        face_cloud_.render();
        STATEMANAGER.restore_transform(gpu::transform::PROJECTION);

        STATEMANAGER.restore_transform(gpu::transform::TEXTURE0);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);

        STATEMANAGER.restore_render_state(gpu::state::LIGHTING);
        STATEMANAGER.restore_render_state(gpu::state::Z_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::FOG_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
    }
}
