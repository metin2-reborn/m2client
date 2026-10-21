#include "effect_mesh.h"
#include <metin2/core/utils.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/graphics/state_manager.h>

namespace metin2::inline effects {

    dynamic_pool<effect_mesh::effect_mesh_data> effect_mesh::effect_mesh_data::pool_value;

    auto effect_mesh::effect_mesh_data::new_value() -> effect_mesh::effect_mesh_data * {
        return pool_value.alloc();
    }

    void effect_mesh::effect_mesh_data::delete_value(effect_mesh_data *data) {
        data->effect_frame_data_vector.clear();
        data->image_vector.clear();

        pool_value.free(data);
    }

    void effect_mesh::effect_mesh_data::destroy_system() {
        pool_value.destroy();
    }

    auto effect_mesh::frame_count() -> uint32_t {
        return frame_count_;
    }

    auto effect_mesh::mesh_count() -> uint32_t {
        return effect_mesh_data_vector_.size();
    }

    auto effect_mesh::mesh_data_pointer(uint32_t mesh_index) -> effect_mesh::effect_mesh_data * {
        assert(mesh_index < effect_mesh_data_vector_.size());
        return effect_mesh_data_vector_[mesh_index];
    }

    auto effect_mesh::texture_vector_pointer(uint32_t mesh_index) -> std::vector<graphic_image *> * {
        if (mesh_index >= effect_mesh_data_vector_.size())
            return nullptr;

        return &effect_mesh_data_vector_[mesh_index]->image_vector;
    }

    auto effect_mesh::type() -> effect_mesh::type_type {
        static type_type s_type = string_to_type("CEffectMesh");
        return s_type;
    }

    auto effect_mesh::on_is_type(type_type type) -> bool {
        if (effect_mesh::type() == type)
            return true;

        return resource::on_is_type(type);
    }

    auto effect_mesh::on_load(int size, const void *c_pv_buf) -> bool {
        if (!c_pv_buf)
            return false;

        const auto *c_pb_buf = static_cast<const uint8_t *>(c_pv_buf);

        char header[10 + 1];
        memcpy(header, c_pb_buf, 10 + 1);
        c_pb_buf += 10 + 1;

        if (0 == strcmp("EffectData", header)) {
            if (!load_data_ver001(size, c_pb_buf))
                return false;
        } else if (0 == strcmp("MDEData002", header)) {
            if (!load_data_ver002(size, c_pb_buf))
                return false;
        } else {
            return false;
        }

        is_data_ = true;
        return true;
    }

    auto effect_mesh::load_data_ver002(int size, const uint8_t *buf) -> bool {
        std::vector<math::vector3> vertex_vector;
        std::vector<int> index_vector;
        std::vector<math::vector2> texture_vertex_vector;
        std::vector<int> texture_index_vector;

        geom_count_ = *(int *)buf;
        buf += 4;
        frame_count_ = *(int *)buf;
        buf += 4;

        effect_mesh_data_vector_.clear();
        effect_mesh_data_vector_.resize(geom_count_);

        for (int n = 0; n < geom_count_; ++n) {
            effect_mesh_data *mesh_data = effect_mesh_data::new_value();

            memcpy(mesh_data->object_name, buf, 32);
            buf += 32;
            memcpy(mesh_data->diffuse_map_file_name, buf, 128);
            buf += 128;

            mesh_data->effect_frame_data_vector.clear();
            mesh_data->effect_frame_data_vector.resize(frame_count_);

            for (int i = 0; i < frame_count_; ++i) {
                effect_frame_data &frame_data = mesh_data->effect_frame_data_vector[i];

                memcpy(&frame_data.changed_frame, buf, sizeof(uint8_t));
                buf += sizeof(uint8_t);

                memcpy(&frame_data.visibility, buf, sizeof(float));
                buf += sizeof(float);

                memcpy(&frame_data.vertex_count, buf, sizeof(uint32_t));
                buf += sizeof(uint32_t);

                memcpy(&frame_data.index_count, buf, sizeof(uint32_t));
                buf += sizeof(uint32_t);

                memcpy(&frame_data.texture_vertex_count, buf, sizeof(uint32_t));
                buf += sizeof(uint32_t);

                vertex_vector.clear();
                vertex_vector.resize(frame_data.vertex_count);
                index_vector.clear();
                index_vector.resize(frame_data.index_count);
                texture_vertex_vector.clear();
                texture_vertex_vector.resize(frame_data.texture_vertex_count);
                texture_index_vector.clear();
                texture_index_vector.resize(frame_data.index_count);

                if (frame_data.vertex_count) memcpy(vertex_vector.data(), buf, frame_data.vertex_count * sizeof(math::vector3));
                buf += frame_data.vertex_count * sizeof(math::vector3);
                if (frame_data.index_count) memcpy(index_vector.data(), buf, frame_data.index_count * sizeof(int));
                buf += frame_data.index_count * sizeof(int);
                if (frame_data.texture_vertex_count) memcpy(texture_vertex_vector.data(), buf, frame_data.texture_vertex_count * sizeof(math::vector2));
                buf += frame_data.texture_vertex_count * sizeof(math::vector2);
                if (frame_data.index_count) memcpy(texture_index_vector.data(), buf, frame_data.index_count * sizeof(int));
                buf += frame_data.index_count * sizeof(int);

                frame_data.pdt_vertex_vector.clear();
                frame_data.pdt_vertex_vector.resize(frame_data.index_count);
                for (uint32_t j = 0; j < frame_data.index_count; ++j) {
                    pt_vertex &vertex = frame_data.pdt_vertex_vector[j];

                    uint32_t index = index_vector[j];
                    uint32_t texture_index = texture_index_vector[j];

                    assert(index < vertex_vector.size());
                    assert(texture_index < texture_vertex_vector.size());

                    vertex.position_value = vertex_vector[index];
                    vertex.tex_coord = texture_vertex_vector[texture_index];
                    vertex.tex_coord.y *= -1;
                }
            }

            mesh_data->image_vector.clear();

            std::string extension;
            get_file_extension(mesh_data->diffuse_map_file_name, strlen(mesh_data->diffuse_map_file_name), &extension);
            stl_lowers(extension);

            if (0 == extension.compare("ifl")) {
                const void *motion_data;
                mapped_file file;

                if (get_file_manager().get(file, mesh_data->diffuse_map_file_name, &motion_data)) {
                    memory_text_file_loader text_file_loader;
                    std::vector<std::string> token_vector;

                    text_file_loader.bind(file.size(), motion_data);

                    std::string path_name;
                    get_only_path_name(mesh_data->diffuse_map_file_name, path_name);

                    std::string texture_file_name;
                    for (uint32_t i = 0; i < text_file_loader.line_count(); ++i) {
                        const std::string &file_name = text_file_loader.line_string(i);

                        if (file_name.empty())
                            continue;

                        texture_file_name = path_name;
                        texture_file_name += file_name;

                        auto *image = (graphic_image *)get_resource_manager().resource_pointer(texture_file_name.c_str());

                        mesh_data->image_vector.push_back(image);
                    }
                }
            } else {
                auto *image = (graphic_image *)get_resource_manager().resource_pointer(mesh_data->diffuse_map_file_name);

                mesh_data->image_vector.push_back(image);
            }

            effect_mesh_data_vector_[n] = mesh_data;
        }

        return true;
    }

    auto effect_mesh::load_data_ver001(int size, const uint8_t *buf) -> bool {
        std::vector<math::vector3> vertex_vector;
        std::vector<int> index_vector;
        std::vector<math::vector2> texture_vertex_vector;
        std::vector<int> texture_index_vector;

        geom_count_ = *(int *)buf;
        buf += 4;
        frame_count_ = *(int *)buf;
        buf += 4;

        effect_mesh_data_vector_.clear();
        effect_mesh_data_vector_.resize(geom_count_);

        for (int n = 0; n < geom_count_; ++n) {
            effect_mesh_data *mesh_data = effect_mesh_data::new_value();

            memcpy(mesh_data->object_name, buf, 32);
            buf += 32;
            memcpy(mesh_data->diffuse_map_file_name, buf, 128);
            buf += 128;

            uint32_t vertex_count;
            uint32_t index_count;
            uint32_t texture_vertex_count;

            memcpy(&vertex_count, buf, sizeof(uint32_t));
            buf += sizeof(uint32_t);

            memcpy(&index_count, buf, sizeof(uint32_t));
            buf += sizeof(uint32_t);

            memcpy(&texture_vertex_count, buf, sizeof(uint32_t));
            buf += sizeof(uint32_t);

            mesh_data->effect_frame_data_vector.clear();
            mesh_data->effect_frame_data_vector.resize(frame_count_);

            for (int i = 0; i < frame_count_; ++i) {
                effect_frame_data &frame_data = mesh_data->effect_frame_data_vector[i];

                frame_data.vertex_count = vertex_count;
                frame_data.index_count = index_count;
                frame_data.texture_vertex_count = texture_vertex_count;

                vertex_vector.clear();
                vertex_vector.resize(frame_data.vertex_count);
                index_vector.clear();
                index_vector.resize(frame_data.index_count);
                texture_vertex_vector.clear();
                texture_vertex_vector.resize(frame_data.texture_vertex_count);
                texture_index_vector.clear();
                texture_index_vector.resize(frame_data.index_count);

                memcpy(&frame_data.visibility, buf, sizeof(float));
                buf += sizeof(float);
                if (frame_data.vertex_count) memcpy(vertex_vector.data(), buf, frame_data.vertex_count * sizeof(math::vector3));
                buf += frame_data.vertex_count * sizeof(math::vector3);
                if (frame_data.index_count) memcpy(index_vector.data(), buf, frame_data.index_count * sizeof(int));
                buf += frame_data.index_count * sizeof(int);
                if (frame_data.texture_vertex_count) memcpy(texture_vertex_vector.data(), buf, frame_data.texture_vertex_count * sizeof(math::vector2));
                buf += frame_data.texture_vertex_count * sizeof(math::vector2);
                if (frame_data.index_count) memcpy(texture_index_vector.data(), buf, frame_data.index_count * sizeof(int));
                buf += frame_data.index_count * sizeof(int);

                frame_data.pdt_vertex_vector.clear();
                frame_data.pdt_vertex_vector.resize(frame_data.index_count);
                for (uint32_t j = 0; j < frame_data.index_count; ++j) {
                    pt_vertex &vertex = frame_data.pdt_vertex_vector[j];

                    uint32_t index = index_vector[j];
                    uint32_t texture_index = texture_index_vector[j];

                    assert(index < vertex_vector.size());
                    assert(texture_index < texture_vertex_vector.size());

                    vertex.position_value = vertex_vector[index];
                    vertex.tex_coord = texture_vertex_vector[texture_index];
                    vertex.tex_coord.y *= -1;
                }
            }

            mesh_data->image_vector.clear();

            std::string extension;
            get_file_extension(mesh_data->diffuse_map_file_name, strlen(mesh_data->diffuse_map_file_name), &extension);
            stl_lowers(extension);

            if (0 == extension.compare("ifl")) {
                const void *motion_data;
                mapped_file file;

                if (get_file_manager().get(file, mesh_data->diffuse_map_file_name, &motion_data)) {
                    memory_text_file_loader text_file_loader;
                    std::vector<std::string> token_vector;

                    text_file_loader.bind(file.size(), motion_data);

                    std::string path_name;
                    get_only_path_name(mesh_data->diffuse_map_file_name, path_name);

                    std::string texture_file_name;
                    for (uint32_t i = 0; i < text_file_loader.line_count(); ++i) {
                        const std::string &file_name = text_file_loader.line_string(i);

                        if (file_name.empty())
                            continue;

                        texture_file_name = path_name;
                        texture_file_name += file_name;

                        auto *image = (graphic_image *)get_resource_manager().resource_pointer(texture_file_name.c_str());

                        mesh_data->image_vector.push_back(image);
                    }
                }
            } else {
                auto *image = (graphic_image *)get_resource_manager().resource_pointer(mesh_data->diffuse_map_file_name);

                mesh_data->image_vector.push_back(image);
            }

            effect_mesh_data_vector_[n] = mesh_data;
        }

        return true;
    }

    void effect_mesh::on_clear() {
        if (!is_data_)
            return;

        for (uint32_t i = 0; i < effect_mesh_data_vector_.size(); ++i) {
            effect_mesh_data_vector_[i]->image_vector.clear();
            effect_mesh_data_vector_[i]->effect_frame_data_vector.clear();

            effect_mesh_data::delete_value(effect_mesh_data_vector_[i]);
        }
        effect_mesh_data_vector_.clear();

        is_data_ = false;
    }

    auto effect_mesh::on_is_empty() const -> bool {
        return !is_data_;
    }

    effect_mesh::effect_mesh(const char *file_name) : resource(file_name) {
        geom_count_ = 0;
        frame_count_ = 0;
        is_data_ = false;
    }

    effect_mesh::~effect_mesh() = default;

    dynamic_pool<effect_mesh_script> effect_mesh_script::pool_value;

    void effect_mesh_script::destroy_system() {
        pool_value.destroy();
    }

    auto effect_mesh_script::new_value() -> effect_mesh_script * {
        return pool_value.alloc();
    }

    void effect_mesh_script::delete_value(effect_mesh_script *data) {
        data->clear();
        pool_value.free(data);
    }

    auto effect_mesh_script::mesh_file_name() -> const char * {
        return mesh_file_name_.c_str();
    }

    auto effect_mesh_script::check_mesh_index(uint32_t mesh_index) -> bool {
        if (mesh_index >= mesh_data_vector_value_.size())
            return false;

        return true;
    }

    auto effect_mesh_script::mesh_data_pointer(uint32_t mesh_index, mesh_data **mesh_data_value) -> bool {
        if (!check_mesh_index(mesh_index))
            return false;

        *mesh_data_value = &mesh_data_vector_value_[mesh_index];

        return true;
    }

    auto effect_mesh_script::billboard_type(uint32_t mesh_index) -> int {
        if (!check_mesh_index(mesh_index))
            return 0;

        return mesh_data_vector_value_[mesh_index].billboard_type;
    }
    auto effect_mesh_script::is_blending_enable(uint32_t mesh_index) -> bool {
        if (!check_mesh_index(mesh_index))
            return false;

        return mesh_data_vector_value_[mesh_index].blending_enable;
    }
    auto effect_mesh_script::blending_src_type(uint32_t mesh_index) -> uint8_t {
        if (!check_mesh_index(mesh_index))
            return 0;

        return mesh_data_vector_value_[mesh_index].blending_src_type;
    }
    auto effect_mesh_script::blending_dest_type(uint32_t mesh_index) -> uint8_t {
        if (!check_mesh_index(mesh_index))
            return 0;

        return mesh_data_vector_value_[mesh_index].blending_dest_type;
    }
    auto effect_mesh_script::color_operation_type(uint32_t mesh_index, uint8_t *type) -> bool {
        if (!check_mesh_index(mesh_index))
            return false;

        *type = mesh_data_vector_value_[mesh_index].color_operation_type;

        return true;
    }
    auto effect_mesh_script::color_factor(uint32_t mesh_index, math::color *color) -> bool {
        if (!check_mesh_index(mesh_index))
            return false;

        *color = mesh_data_vector_value_[mesh_index].color_factor;

        return true;
    }

    auto effect_mesh_script::time_table_alpha_pointer(uint32_t mesh_index, time_event_table_float **time_event_alpha) -> bool {
        if (!check_mesh_index(mesh_index))
            return false;

        *time_event_alpha = &mesh_data_vector_value_[mesh_index].time_event_alpha;

        return true;
    }

    auto effect_mesh_script::is_mesh_animation_loop() -> bool {
        return is_mesh_animation_loop_;
    }
    auto effect_mesh_script::mesh_animation_loop_count() -> int {
        return mesh_animation_loop_count_;
    }
    auto effect_mesh_script::mesh_animation_frame_delay() -> float {
        return mesh_animation_frame_delay_;
    }

    auto effect_mesh_script::on_load_script(text_file_loader &text_file_loader) -> bool {
        if (text_file_loader.token_string("meshfilename", &mesh_file_name_)) {
            if (!is_global_file_name(mesh_file_name_.c_str())) {
                mesh_file_name_ = get_only_path_name(text_file_loader.file_name()) + mesh_file_name_;
            }
        } else {
            return false;
        }

        int mesh_animation_loop;
        if (!text_file_loader.token_integer("meshanimationloopenable", &mesh_animation_loop))
            return false;
        is_mesh_animation_loop_ = mesh_animation_loop != 0;
        if (!text_file_loader.token_integer("meshanimationloopcount", &mesh_animation_loop_count_)) {
            mesh_animation_loop_count_ = 0;
        }
        if (!text_file_loader.token_float("meshanimationframedelay", &mesh_animation_frame_delay_))
            return false;

        uint32_t mesh_element_count;
        if (!text_file_loader.token_double_word("meshelementcount", &mesh_element_count))
            return false;

        mesh_data_vector_value_.clear();
        mesh_data_vector_value_.resize(mesh_element_count);
        for (uint32_t i = 0; i < mesh_data_vector_value_.size(); ++i) {
            text_file_loader::goto_child goto_child(&text_file_loader, i);

            mesh_data &mesh_data_value = mesh_data_vector_value_[i];

            if (!text_file_loader.token_byte("billboardtype", &mesh_data_value.billboard_type))
                return false;
            if (!text_file_loader.token_boolean("blendingenable", &mesh_data_value.blending_enable))
                return false;
            if (!text_file_loader.token_byte("blendingsrctype", &mesh_data_value.blending_src_type))
                return false;
            if (!text_file_loader.token_byte("blendingdesttype", &mesh_data_value.blending_dest_type))
                return false;

            if (!text_file_loader.token_boolean("textureanimationloopenable", &mesh_data_value.texture_animation_loop_enable))
                return false;
            if (!text_file_loader.token_float("textureanimationframedelay", &mesh_data_value.texture_animation_frame_delay))
                return false;
            if (!text_file_loader.token_double_word("textureanimationstartframe", &mesh_data_value.texture_animation_start_frame)) {
                mesh_data_value.texture_animation_start_frame = 0;
            }

            if (!text_file_loader.token_byte("coloroperationtype", &mesh_data_value.color_operation_type)) {
                mesh_data_value.color_operation_type = gpu::texture_op::MODULATE;
            }
            if (!text_file_loader.token_color("colorfactor", &mesh_data_value.color_factor)) {
                mesh_data_value.color_factor = math::color(1.0f, 1.0f, 1.0f, 1.0f);
            }

            if (!get_token_time_event_float(text_file_loader, "timeeventalpha", &mesh_data_value.time_event_alpha)) {
                mesh_data_value.time_event_alpha.clear();
            }
        }

        return true;
    }

    auto effect_mesh_script::on_is_data() -> bool {
        if (0 == mesh_file_name_.length())
            return false;

        return true;
    }

    void effect_mesh_script::on_clear() {
        is_mesh_animation_loop_ = false;
        mesh_animation_loop_count_ = 0;
        mesh_animation_frame_delay_ = 0.02f;

        mesh_data_vector_value_.clear();
        mesh_file_name_ = "";
    }

    effect_mesh_script::effect_mesh_script() = default;
    effect_mesh_script::~effect_mesh_script() = default;
}
