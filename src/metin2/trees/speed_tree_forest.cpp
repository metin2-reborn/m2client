#include "speed_tree_forest.h"

#include <cstring>

#include <metin2/core/mapped_file.h>
#include <metin2/files/eter_file_manager.h>

namespace metin2::inline trees {

    speed_tree_forest::speed_tree_forest() {
        memset(lighting_, 0, sizeof(lighting_));
        memset(fog_, 0, sizeof(fog_));
    }

    speed_tree_forest::~speed_tree_forest() {
        clear();
    }

    void speed_tree_forest::clear() {
        auto itor = main_tree_map_.begin();
        unsigned int count;

        while (itor != main_tree_map_.end()) {
            speed_tree_wrapper *main_tree = (itor++)->second;
            speed_tree_wrapper **instances = main_tree->instances(count);

            for (unsigned int i = 0; i < count; ++i)
                delete instances[i];

            delete main_tree;
        }

        main_tree_map_.clear();
    }

    auto speed_tree_forest::main_tree(uint32_t crc, speed_tree_wrapper **main_tree, const char *file_name) -> bool {
        auto itor = main_tree_map_.find(crc);

        speed_tree_wrapper *tree;

        if (itor != main_tree_map_.end())
            tree = itor->second;
        else {
            mapped_file file;
            const void *data;

            if (!get_file_manager().get(file, file_name, &data))
                return false;

            tree = new speed_tree_wrapper;

            if (!tree->load_tree(file_name, std::span(static_cast<const uint8_t *>(data), file.size()))) {
                delete tree;
                return false;
            }

            main_tree_map_.insert(tree_map::value_type(crc, tree));
        }

        *main_tree = tree;
        return true;
    }

    auto speed_tree_forest::create_instance(float x, float y, float z, uint32_t tree_crc, const char *tree_name) -> speed_tree_wrapper * {
        speed_tree_wrapper *main_tree_value;
        if (!main_tree(tree_crc, &main_tree_value, tree_name))
            return nullptr;

        speed_tree_wrapper *tree_inst = main_tree_value->make_instance();
        if (!tree_inst)
            return nullptr;

        tree_inst->set_position(x, y, z);
        tree_inst->register_bounding_sphere();
        return tree_inst;
    }

    void speed_tree_forest::delete_instance(speed_tree_wrapper *instance) {
        if (!instance)
            return;

        speed_tree_wrapper *parent_tree = instance->instance_of();

        if (!parent_tree)
            return;

        parent_tree->delete_instance(instance);
    }

    void speed_tree_forest::set_light(const float *direction, const float *ambient, const float *diffuse) {
        lighting_[0] = direction[0];
        lighting_[1] = direction[1];
        lighting_[2] = direction[2];
        lighting_[3] = 1.0f;

        lighting_[4] = ambient[0];
        lighting_[5] = ambient[1];
        lighting_[6] = ambient[2];
        lighting_[7] = ambient[3];

        lighting_[8] = diffuse[0];
        lighting_[9] = diffuse[1];
        lighting_[10] = diffuse[2];
        lighting_[11] = diffuse[3];
    }

    void speed_tree_forest::set_fog(float fog_near, float fog_far) {
        const float fog_linear_scale = (1.0f / (fog_far - fog_near));

        fog_[0] = fog_near;
        fog_[1] = fog_far;
        fog_[2] = fog_linear_scale;
        fog_[3] = 0.0f;
    }
}
