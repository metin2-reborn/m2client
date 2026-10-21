#include "effect_data.h"
#include <metin2/core/file_name.h>

namespace metin2::inline effects {

    dynamic_pool<effect_data> effect_data::pool_value;

    auto effect_data::new_value() -> effect_data * {
        return pool_value.alloc();
    }

    void effect_data::delete_value(effect_data *data) {
        data->clear();
        pool_value.free(data);
    }

    void effect_data::destroy_system() {
        pool_value.destroy();

        particle_system_data::destroy_system();
        effect_mesh_script::destroy_system();
        light_data::destroy_system();
    }

    auto effect_data::load_script(const char *file_name) -> bool {
        file_name_ = file_name;
        file_name_helper::string_path(file_name_);

        text_file_loader text_file_loader;
        if (!text_file_loader.load(file_name))
            return false;

        text_file_loader.set_top();

        if (!text_file_loader.token_float("boundingsphereradius", &bounding_sphere_radius_)) {
            bounding_sphere_radius_ = 0.0f;
        }

        if (!text_file_loader.token_vector3("boundingsphereposition", &bounding_sphere_position_)) {
            bounding_sphere_position_.x = bounding_sphere_position_.y = bounding_sphere_position_.z = 0.0f;
        }

        for (uint32_t i = 0; i < text_file_loader.child_node_count(); ++i) {
            if (!text_file_loader.set_child_node(i))
                continue;

            std::string name;

            if (!text_file_loader.current_node_name(&name))
                continue;

            if (0 == name.compare("mesh")) {
                effect_mesh_script *mesh = alloc_mesh();
                mesh->clear();
                mesh->load_script(text_file_loader);
            } else if (0 == name.compare("particle")) {
                particle_system_data *particle_system_data = alloc_particle();
                particle_system_data->clear();
                particle_system_data->load_script(text_file_loader);
            } else if (0 == name.compare("light")) {
                light_data *light_data = alloc_light();
                light_data->clear();
                light_data->load_script(text_file_loader);
            }

            text_file_loader.set_parent_node();
        }

        std::string path_header = "d:/ymir work/";
        std::string no_extension_name = file_name_helper::no_extension(file_name_);
        if (no_extension_name.compare(0, path_header.size(), path_header) == 0 && no_extension_name.size() > path_header.size()) {
            std::string sound_file_name;
            sound_file_name = "sound/";
            sound_file_name += &no_extension_name[path_header.size()];
            sound_file_name += ".mss";

            load_sound_script_data(sound_file_name.c_str());
        }

        return true;
    }

    auto effect_data::load_sound_script_data(const char *file_name) -> bool {
        sound_types::sound_data_vector sound_data_vector;

        if (!sound_types::load_sound_information_piece(file_name, sound_data_vector))
            return false;

        sound_types::data_to_instance(sound_data_vector, &sound_instance_vector_value_);
        return true;
    }

    auto effect_data::alloc_particle() -> particle_system_data * {
        particle_system_data *particle = particle_system_data::new_value();
        particle_vector_value_.push_back(particle);
        return particle;
    }

    auto effect_data::alloc_mesh() -> effect_mesh_script * {
        effect_mesh_script *mesh = effect_mesh_script::new_value();
        mesh_vector_value_.push_back(mesh);
        return mesh;
    }

    auto effect_data::alloc_light() -> light_data * {
        light_data *light = light_data::new_value();
        light_vector_value_.push_back(light);
        return light;
    }

    auto effect_data::light_count() -> uint32_t {
        return light_vector_value_.size();
    }

    auto effect_data::light_pointer(uint32_t position) -> light_data * {
        assert(position < light_vector_value_.size());
        return light_vector_value_[position];
    }

    auto effect_data::particle_count() -> uint32_t {
        return particle_vector_value_.size();
    }
    auto effect_data::particle_pointer(uint32_t position) -> particle_system_data * {
        if (position < particle_vector_value_.size())
            return particle_vector_value_[position];
        else {
            assert(false);
            return nullptr;
        }
    }

    auto effect_data::mesh_count() -> uint32_t {
        return mesh_vector_value_.size();
    }
    auto effect_data::mesh_pointer(uint32_t position) -> effect_mesh_script * {
        assert(position < mesh_vector_value_.size());
        return mesh_vector_value_[position];
    }

    auto effect_data::get_sound_instance_vector() -> sound_types::sound_instance_vector * {
        return &sound_instance_vector_value_;
    }

    auto effect_data::bounding_sphere_radius() -> float {
        return bounding_sphere_radius_;
    }

    auto effect_data::bounding_sphere_position() -> math::vector3 {
        return bounding_sphere_position_;
    }

    auto effect_data::file_name() const -> const char * {
        return file_name_.c_str();
    }

    void effect_data::clear_particle_data_vector() {
        std::for_each(particle_vector_value_.begin(), particle_vector_value_.end(), particle_system_data::delete_value);
        particle_vector_value_.clear();
    }

    void effect_data::clear_light_data_vector() {
        std::for_each(light_vector_value_.begin(), light_vector_value_.end(), light_data::delete_value);
        light_vector_value_.clear();
    }

    void effect_data::clear_mesh_data_vector() {
        std::for_each(mesh_vector_value_.begin(), mesh_vector_value_.end(), effect_mesh_script::delete_value);
        mesh_vector_value_.clear();
    }

    void effect_data::clear() {
        bounding_sphere_radius_ = 0.0f;
        bounding_sphere_position_.x = bounding_sphere_position_.y = bounding_sphere_position_.z = 0.0f;
        clear_particle_data_vector();
        clear_light_data_vector();
        clear_mesh_data_vector();
        sound_instance_vector_value_.clear();
        file_name_.clear();
    }

    effect_data::effect_data() {
        bounding_sphere_radius_ = 0.0f;
        bounding_sphere_position_.x = bounding_sphere_position_.y = bounding_sphere_position_.z = 0.0f;
    }
    effect_data::~effect_data() = default;
}
