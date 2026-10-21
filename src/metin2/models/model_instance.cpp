#include "model_instance.h"
#include <metin2/graphics/resource_manager.h>
#include "model.h"

namespace metin2::inline models {

    void model_instance::set_material_image_pointer(const char *image_name, graphic_image *image) {
        mtrl_pal_.set_material_image_pointer(image_name, image);
    }

    void model_instance::set_material_data(const char *image_name, const material_data &material_data) {
        mtrl_pal_.set_material_data(image_name, material_data);
    }

    void model_instance::set_specular_info(const char *mtrl_name, bool enable, float power) {
        mtrl_pal_.set_specular_info(mtrl_name, enable, power);
    }

    void model_instance::set_local_time(float local_time) {
        local_time_ = local_time;
    }

    auto model_instance::reset_local_time() -> int {
        local_time_ = 0.0f;
        return 0;
    }

    auto model_instance::local_time() -> float {
        return local_time_;
    }

    void model_instance::set_parent_model_instance(const model_instance *parent_model_instance, const char *bone_name) {
        int bone_index;
        if (!parent_model_instance->bone_index_by_name(bone_name, &bone_index))
            return;

        set_parent_model_instance(parent_model_instance, bone_index);
    }

    void model_instance::set_parent_model_instance(const model_instance *parent_model_instance, int bone) {
        mc_parent_instance_ = parent_model_instance;
        parent_bone_index_ = bone;
    }

    auto model_instance::is_empty() -> bool {
        if (model_value_) {
            if (!mesh_matrices_)
                return true;
            return false;
        }

        return true;
    }

    auto model_instance::create_device_objects() -> bool {
        create_dynamic_vertex_buffer();

        return true;
    }

    void model_instance::destroy_device_objects() {
        destroy_dynamic_vertex_buffer();
    }

    void model_instance::initialize_impl() {
        if (model_value_) {
            model_value_->release();
        }
        model_value_ = nullptr;
        mc_parent_instance_ = nullptr;
        parent_bone_index_ = 0;

        pgrn_model_instance_ = nullptr;

        pgrn_world_pose_real_ = nullptr;
        skeleton_inst_ = nullptr;

        mesh_matrices_.reset();
        pgrn_ctrl_ = nullptr;
        pgrn_ani_ = nullptr;
        shared_deformable_vertex_buffer_ = nullptr;
        local_time_ = 0.0f;
        old_update_frame_ = 0;
    }

    model_instance::model_instance() {
        model_value_ = nullptr;
        initialize_impl();
    }

    model_instance::~model_instance() {
        clear();
    }

    dynamic_pool<model_instance> model_instance::pool_value;

    auto model_instance::new_value() -> model_instance * {
        return pool_value.alloc();
    }

    void model_instance::delete_value(model_instance *inst) {
        inst->clear();
        pool_value.free(inst);
    }

    void model_instance::destroy_system() {
        pool_value.destroy();
    }
}
