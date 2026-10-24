#include "thing_instance.h"
#include <metin2/core/timer.h>
#include <metin2/graphics/camera.h>
#include <metin2/log/log.h>
#include "model_instance.h"
#include "thing.h"

namespace metin2::inline models {

    dynamic_pool<graphic_thing_instance> graphic_thing_instance::pool_value;

    auto graphic_thing_instance::base_thing_ptr() -> graphic_thing * {
        if (model_thing_set_vector_.empty()) {
            return nullptr;
        }

        model_thing_set &model_thing_set_value = model_thing_set_vector_[0];
        if (model_thing_set_value.lod_thing_ref_vector.empty()) {
            return nullptr;
        }

        graphic_thing::ref *pro_thing = model_thing_set_value.lod_thing_ref_vector[0];
        if (!pro_thing) {
            return nullptr;
        }

        graphic_thing::ref ro_thing = *pro_thing;
        return ro_thing.pointer();
    }

    auto graphic_thing_instance::less_render_order(graphic_thing_instance *thing_instance) -> bool {
        return (base_thing_ptr() < thing_instance->base_thing_ptr());
    }

    void graphic_thing_instance::create_system(uint32_t capacity) {
        pool_value.create(capacity);
    }

    void graphic_thing_instance::destroy_system() {
        pool_value.destroy();
    }

    auto graphic_thing_instance::new_value() -> graphic_thing_instance * {
        return pool_value.alloc();
    }

    void graphic_thing_instance::delete_value(graphic_thing_instance *thing_inst) {
        thing_inst->clear();
        pool_value.free(thing_inst);
    }

    void graphic_thing_instance::set_motion_at_end() {
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), std::mem_fn(&lod_controller::set_motion_at_end));
    }

    auto graphic_thing_instance::picking(const math::vector3 &v, const math::vector3 &dir, float &out_x, float &out_y) -> bool {
        if (!height_attribute_instance_) {
            return false;
        }
        return height_attribute_instance_->picking(v, dir, out_x, out_y);
    }

    void graphic_thing_instance::on_update_collision_data(const static_collision_data_vector *pscd_vector) {
        assert(pscd_vector);
        static_collision_data_vector::const_iterator it;
        for (it = pscd_vector->begin(); it != pscd_vector->end(); ++it) {
            add_collision(&(*it), &get_transform());
        }
    }

    void graphic_thing_instance::on_update_heigh_instance(attribute_instance *attribute_instance) {
        assert(attribute_instance);
        set_height_instance(attribute_instance);
    }

    auto graphic_thing_instance::on_get_object_height(float x, float y, float *height) -> bool {
        if (height_attribute_instance_ && height_attribute_instance_->height(x, y, height)) {
            return true;
        }
        return false;
    }

    void graphic_thing_instance::build_bounding_sphere() {
        math::vector3 min_value, max_value;
        get_bound_box(0, &min_value, &max_value);
        center_ = (min_value + max_value) * 0.5f;
        math::vector3 v_delta = (max_value - min_value);

        radius_ = math::length(v_delta) * 0.5f + 50.0f; // Extra length for attached objects.
    }

    auto graphic_thing_instance::bounding_sphere(math::vector3 &center, float &radius) -> bool {
        if (radius_ <= 0) {
            build_bounding_sphere();

            radius = radius_;
            center = center_;
        } else {
            radius = radius_;
            center = center_;
        }

        center = math::transform_coord(center, get_transform());
        return true;
    }

    void graphic_thing_instance::build_bounding_aabb() {
        math::vector3 min_value, max_value;
        get_bound_box(0, &min_value, &max_value);
        center_ = (min_value + max_value) * 0.5f;
        min_ = min_value;
        max_ = max_value;
    }

    auto graphic_thing_instance::bounding_aabb(math::vector3 &min_value, math::vector3 &max_value) -> bool {
        build_bounding_aabb();

        min_value = min_;
        max_value = max_;

        center_ = math::transform_coord(center_, get_transform());
        return true;
    }

    void graphic_thing_instance::calculate_b_box() {
        get_bound_box(&b_box_min_, &b_box_max_);

        tb_box_[0] = math::vector4(b_box_min_.x, b_box_min_.y, b_box_min_.z, 1.0f);
        tb_box_[1] = math::vector4(b_box_min_.x, b_box_max_.y, b_box_min_.z, 1.0f);
        tb_box_[2] = math::vector4(b_box_max_.x, b_box_min_.y, b_box_min_.z, 1.0f);
        tb_box_[3] = math::vector4(b_box_max_.x, b_box_max_.y, b_box_min_.z, 1.0f);
        tb_box_[4] = math::vector4(b_box_min_.x, b_box_min_.y, b_box_max_.z, 1.0f);
        tb_box_[5] = math::vector4(b_box_min_.x, b_box_max_.y, b_box_max_.z, 1.0f);
        tb_box_[6] = math::vector4(b_box_max_.x, b_box_min_.y, b_box_max_.z, 1.0f);
        tb_box_[7] = math::vector4(b_box_max_.x, b_box_max_.y, b_box_max_.z, 1.0f);

        const math::matrix &transform = get_transform();

        for (uint32_t i = 0; i < 8; ++i) {
            tb_box_[i] = math::transform(tb_box_[i], transform);
            if (0 == i) {
                tb_box_min_.x = tb_box_[i].x;
                tb_box_min_.y = tb_box_[i].y;
                tb_box_min_.z = tb_box_[i].z;
                tb_box_max_.x = tb_box_[i].x;
                tb_box_max_.y = tb_box_[i].y;
                tb_box_max_.z = tb_box_[i].z;
            } else {
                if (tb_box_min_.x > tb_box_[i].x) {
                    tb_box_min_.x = tb_box_[i].x;
                }
                if (tb_box_max_.x < tb_box_[i].x) {
                    tb_box_max_.x = tb_box_[i].x;
                }
                if (tb_box_min_.y > tb_box_[i].y) {
                    tb_box_min_.y = tb_box_[i].y;
                }
                if (tb_box_max_.y < tb_box_[i].y) {
                    tb_box_max_.y = tb_box_[i].y;
                }
                if (tb_box_min_.z > tb_box_[i].z) {
                    tb_box_min_.z = tb_box_[i].z;
                }
                if (tb_box_max_.z < tb_box_[i].z) {
                    tb_box_max_.z = tb_box_[i].z;
                }
            }
        }
    }

    auto graphic_thing_instance::create_device_objects() -> bool {
        lod_controller::create_device_objects_fn create_device_objects;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), create_device_objects);
        return true;
    }

    void graphic_thing_instance::destroy_device_objects() {
        lod_controller::destroy_device_objects_fn destroy_device_objects;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), destroy_device_objects);
    }

    auto graphic_thing_instance::check_model_instance_index(int model_instance) -> bool {
        if (model_instance < 0) {
            return false;
        }

        if (model_instance >= int(lod_controller_vector_.size())) {
            return false;
        }

        return true;
    }

    auto graphic_thing_instance::check_model_thing_index(int model_thing) -> bool {
        if (model_thing < 0) {
            return false;
        }

        if (model_thing >= int(model_thing_set_vector_.size())) {
            return false;
        }

        return true;
    }

    auto graphic_thing_instance::check_motion_thing_index(uint32_t motion_key) -> bool {
        auto itor = ro_motion_thing_map_.find(motion_key);

        if (ro_motion_thing_map_.end() == itor) {
            return false;
        }

        return true;
    }

    auto graphic_thing_instance::motion_thing_pointer(uint32_t key, graphic_thing **motion) -> bool {
        if (!check_motion_thing_index(key)) {
            return false;
        }

        *motion = ro_motion_thing_map_[key]->pointer();
        return true;
    }

    auto graphic_thing_instance::is_motion_thing() -> bool {
        return !ro_motion_thing_map_.empty();
    }

    void graphic_thing_instance::reserve_model_instance(int count) {
        stl_wipe(lod_controller_vector_);

        for (int i = 0; i < count; ++i) {
            auto *instance = new lod_controller;
            lod_controller_vector_.push_back(instance);
        }
    }

    void graphic_thing_instance::reserve_model_thing(int count) {
        model_thing_set_vector_.resize(count);
    }

    auto graphic_thing_instance::find_bone_index(int i_model_instance, const char *bone_name, int *ret_bone) -> bool {
        assert(check_model_instance_index(i_model_instance));

        model_instance *p_model_instance = lod_controller_vector_[i_model_instance]->get_model_instance();

        if (!p_model_instance) {
            return false;
        }

        return p_model_instance->bone_index_by_name(bone_name, ret_bone);
    }

    void graphic_thing_instance::attach_model_instance(int dst_model_instance, const char *bone_name, int src_model_instance) {
        if (!check_model_instance_index(src_model_instance)) {
            metin2::log::error("Model instance {2} out of range (attaching it to model instance {0} at bone {1})", dst_model_instance, bone_name, src_model_instance);
            return;
        }
        if (!check_model_instance_index(dst_model_instance)) {
            metin2::log::error("Model instance {0} out of range (attaching model instance {2} at bone {1})", dst_model_instance, bone_name, src_model_instance);
            return;
        }

        lod_controller *src_lod_controller = lod_controller_vector_[src_model_instance];
        lod_controller *dst_lod_controller = lod_controller_vector_[dst_model_instance];
        dst_lod_controller->attach_model_instance(src_lod_controller, bone_name);
    }

    void graphic_thing_instance::attach_model_instance(int dst_model_instance, const char *bone_name, graphic_thing_instance &src_instance, int src_model_instance) {
        if (!check_model_instance_index(dst_model_instance)) {
            metin2::log::error("Model instance {0} out of range (attaching model instance {2} at bone {1})", dst_model_instance, bone_name, src_model_instance);
            return;
        }
        if (!src_instance.check_model_instance_index(src_model_instance)) {
            metin2::log::error("Model instance {2} out of range (attaching it to model instance {0} at bone {1})", dst_model_instance, bone_name, src_model_instance);
            return;
        }

        lod_controller *dst_lod_controller = lod_controller_vector_[dst_model_instance];
        lod_controller *src_lod_controller = src_instance.lod_controller_vector_[src_model_instance];
        dst_lod_controller->attach_model_instance(src_lod_controller, bone_name);
    }

    void graphic_thing_instance::detach_model_instance(int dst_model_instance, graphic_thing_instance &src_instance, int src_model_instance) {
        if (!check_model_instance_index(dst_model_instance)) {
            metin2::log::error("Model instance {0} out of range (detaching model instance {1})", dst_model_instance, src_model_instance);
            return;
        }
        if (!src_instance.check_model_instance_index(src_model_instance)) {
            metin2::log::error("Model instance {1} out of range (detaching it from model instance {0})", dst_model_instance, src_model_instance);
            return;
        }

        lod_controller *dst_lod_controller = lod_controller_vector_[dst_model_instance];
        lod_controller *src_lod_controller = src_instance.lod_controller_vector_[src_model_instance];
        dst_lod_controller->detach_model_instance(src_lod_controller);
    }

    // Without it the model is attached to the base bone (only the position is taken).
    auto graphic_thing_instance::set_model_instance(int dst_model_instance, int src_model_thing, int src_model, int skel_instance) -> bool {
        if (!check_model_instance_index(dst_model_instance)) {
            metin2::log::error("Model instance {0} out of range (setting model {2} of model thing {1})", dst_model_instance, src_model_thing, src_model);
            return false;
        }
        if (!check_model_thing_index(src_model_thing)) {
            metin2::log::error("Model thing {1} out of range (setting its model {2} on model instance {0})", dst_model_instance, src_model_thing, src_model);
            return false;
        }

        lod_controller *controller = lod_controller_vector_[dst_model_instance];
        if (!controller) {
            return false;
        }

        lod_controller *skel_controller = nullptr;
        if (skel_instance != DONTUSEVALUE) {
            if (!check_model_instance_index(skel_instance)) {
                metin2::log::error("Skeleton model instance {0} out of range (setting model {3} of model thing {2} on model instance {1})", skel_instance, dst_model_instance, src_model_thing, src_model);
                return false;
            }
            skel_controller = lod_controller_vector_[skel_instance];
            if (!skel_controller) {
                return false;
            }
        }

        model_thing_set &model_thing_set_value = model_thing_set_vector_[src_model_thing];

        controller->clear();

        for (uint32_t i = 0; i < model_thing_set_value.lod_thing_ref_vector.size(); ++i) {
            if (model_thing_set_value.lod_thing_ref_vector[i]->is_null()) {
                return false;
            }

            controller->add_model(model_thing_set_value.lod_thing_ref_vector[i]->pointer(), src_model, skel_controller);
        }
        return true;
    }

    void graphic_thing_instance::set_material_image_pointer(uint32_t part, const char *image_name, graphic_image *image) {
        if (part >= lod_controller_vector_.size()) {
            metin2::log::error("Cannot set material image {2} ({3}): part {0} out of range ({1} parts)", part, lod_controller_vector_.size(), image_name, image ? image->file_name() : "NULL");

            return;
        }

        if (!lod_controller_vector_[part]) {
            metin2::log::error("Cannot set material image {1} ({2}): part {0} has no data", part, image_name, image ? image->file_name() : "NULL");

            return;
        }

        lod_controller_vector_[part]->set_material_image_pointer(image_name, image);
    }

    void graphic_thing_instance::set_material_data(uint32_t part, const char *image_name, material_data material_data) {
        if (part >= lod_controller_vector_.size()) {
            metin2::log::error("Cannot set material data: part {} out of range ({} parts)", part, lod_controller_vector_.size());

            return;
        }

        if (!lod_controller_vector_[part]) {
            metin2::log::error("Cannot set material data: part {} has no data", part);

            return;
        }

        lod_controller_vector_[part]->set_material_data(image_name, material_data);
    }

    void graphic_thing_instance::set_specular_info(uint32_t part, const char *mtrl_name, bool enable, float power) {
        if (part >= lod_controller_vector_.size()) {
            metin2::log::error("Cannot set specular info: part {} out of range ({} parts)", part, lod_controller_vector_.size());

            return;
        }

        if (!lod_controller_vector_[part]) {
            metin2::log::error("Cannot set specular info: part {} has no data", part);

            return;
        }

        lod_controller_vector_[part]->set_specular_info(mtrl_name, enable, power);
    }

    auto graphic_thing_instance::set_motion(uint32_t motion_key, float blend_time, int loop_count, float speed_ratio) -> bool {
        if (!check_motion_thing_index(motion_key)) {
            return false;
        }

        auto itor = ro_motion_thing_map_.find(motion_key);
        graphic_thing::ref *pro_motion_thing = itor->second;
        graphic_thing *motion_thing = pro_motion_thing->pointer();

        if (!motion_thing) {
            return false;
        }

        if (!motion_thing->check_motion_index(0)) {
            return false;
        }

        lod_controller::set_motion_pointer_fn set_motion_pointer;
        set_motion_pointer.motion_value = motion_thing->motion_pointer(0);
        set_motion_pointer.blend_time = blend_time;
        set_motion_pointer.loop_count = loop_count;
        set_motion_pointer.speed_ratio = speed_ratio;

        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), set_motion_pointer);
        return true;
    }

    auto graphic_thing_instance::change_motion(uint32_t motion_key, int loop_count, float speed_ratio) -> bool {
        if (!check_motion_thing_index(motion_key)) {
            return false;
        }

        auto itor = ro_motion_thing_map_.find(motion_key);
        graphic_thing::ref *pro_motion_thing = itor->second;
        graphic_thing *motion_thing = pro_motion_thing->pointer();

        if (!motion_thing) {
            return false;
        }

        if (!motion_thing->check_motion_index(0)) {
            return false;
        }

        lod_controller::change_motion_pointer_fn change_motion_pointer;
        change_motion_pointer.motion_value = motion_thing->motion_pointer(0);
        change_motion_pointer.loop_count = loop_count;
        change_motion_pointer.speed_ratio = speed_ratio;

        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), change_motion_pointer);
        return true;
    }

    void graphic_thing_instance::set_end_stop_motion() {
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), lod_controller::end_stop_motion_pointer_fn());
    }

    void graphic_thing_instance::register_model_thing(int i_model_thing, graphic_thing *p_model_thing) {
        if (!check_model_thing_index(i_model_thing)) {
            metin2::log::error("Cannot register model thing {1}: index {0} out of range", i_model_thing, p_model_thing ? p_model_thing->file_name() : "NULL");
            return;
        }

        model_thing_set_vector_[i_model_thing].clear();

        if (p_model_thing) {
            register_lod_thing(i_model_thing, p_model_thing);
        }
    }

    void graphic_thing_instance::register_lod_thing(int i_model_thing, graphic_thing *p_model_thing) {
        assert(check_model_thing_index(i_model_thing));
        auto *model_ref = new graphic_thing::ref;
        model_ref->set_pointer(p_model_thing);
        model_thing_set_vector_[i_model_thing].lod_thing_ref_vector.push_back(model_ref);
    }

    void graphic_thing_instance::register_motion_thing(uint32_t motion_key, graphic_thing *motion_thing) {
        graphic_thing::ref *&motion_ref = ro_motion_thing_map_[motion_key];
        if (!motion_ref) {
            motion_ref = new graphic_thing::ref;
        }
        motion_ref->set_pointer(motion_thing);
    }

    void graphic_thing_instance::reset_local_time() {
        local_time_ = 0.0f;

        lod_controller::reset_local_time_fn reset_local_time;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), reset_local_time);
    }

    void graphic_thing_instance::insert_delay(float delay) {
        delay_ = delay;
    }

    auto graphic_thing_instance::local_time() -> float {
        return local_time_;
    }

    auto graphic_thing_instance::second_elapsed() -> float {
        return second_elapsed_;
    }

    auto graphic_thing_instance::intersect(float *pu, float *pv, float *pt) -> bool {
        if (!graphic_object_instance::is_show()) {
            return false;
        }
        if (!updated_) {
            return false;
        }

        if (lod_controller_vector_.empty()) {
            return false;
        }

        return lod_controller_vector_[0]->intersect(&get_transform(), pu, pv, pt);
    }

    void graphic_thing_instance::get_bound_box(math::vector3 *vt_min, math::vector3 *vt_max) {
        vt_min->x = vt_min->y = vt_min->z = 100000.0f;
        vt_max->x = vt_max->y = vt_max->z = -100000.0f;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), lod_controller::bound_box_fn(vt_min, vt_max));
    }

    auto graphic_thing_instance::get_bound_box(uint32_t model_instance_index, math::vector3 *vt_min, math::vector3 *vt_max) -> bool {
        if (!check_model_instance_index(model_instance_index)) {
            return false;
        }

        vt_min->x = vt_min->y = vt_min->z = 100000.0f;
        vt_max->x = vt_max->y = vt_max->z = -100000.0f;

        lod_controller *controller = lod_controller_vector_[model_instance_index];
        if (!controller->is_model_instance()) {
            return false;
        }

        model_instance *model_instance = controller->get_model_instance();
        model_instance->get_bound_box(vt_min, vt_max);
        return true;
    }

    auto graphic_thing_instance::bone_matrix(uint32_t model_instance_index, uint32_t bone_index, math::matrix **matrix) -> bool {
        if (!check_model_instance_index(model_instance_index)) {
            return false;
        }

        model_instance *model_instance = lod_controller_vector_[model_instance_index]->get_model_instance();
        if (!model_instance) {
            return false;
        }

        *matrix = (math::matrix *)model_instance->bone_matrix_pointer(bone_index);
        if (!*matrix) {
            return false;
        }

        return true;
    }

    auto graphic_thing_instance::composite_bone_matrix(uint32_t model_instance_index, uint32_t bone_index, math::matrix **matrix) -> bool {
        if (!check_model_instance_index(model_instance_index)) {
            return false;
        }

        model_instance *model_instance = lod_controller_vector_[model_instance_index]->get_model_instance();
        if (!model_instance) {
            return false;
        }

        *matrix = (math::matrix *)model_instance->composite_bone_matrix_pointer(bone_index);
        if (!*matrix) {
            return false;
        }

        return true;
    }

    void graphic_thing_instance::update_transform(math::matrix *matrix, float seconds_elapsed, int model_instance_index) {
        int lod_count = int(lod_controller_vector_.size());
        if (model_instance_index < 0 || model_instance_index >= lod_count) {
            return;
        }

        lod_controller *lod_ctrl = lod_controller_vector_[model_instance_index];
        if (!lod_ctrl) {
            return;
        }

        model_instance *model_instance = lod_ctrl->get_model_instance();
        if (!model_instance) {
            return;
        }

        model_instance->update_transform(matrix, seconds_elapsed);
    }

    void graphic_thing_instance::deform_all() {
        updated_ = true;

        lod_controller::deform_all_fn deform;
        deform.mc_world_matrix = &world_matrix_;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), deform);
    }

    void graphic_thing_instance::deform_no_skin() {
        updated_ = true;

        std::vector<lod_controller *>::iterator i;
        for (i = lod_controller_vector_.begin(); i != lod_controller_vector_.end(); ++i) {
            lod_controller *lod = *i;
            if (lod->is_model_instance()) {
                lod->deform_no_skin(&world_matrix_);
            }
        }
    }

    void graphic_thing_instance::on_deform() {
        updated_ = true;

        lod_controller::deform_fn deform;
        deform.mc_world_matrix = &world_matrix_;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), deform);
    }

    void graphic_thing_instance::set_local_time_impl(float local_time) {
        local_time_ = local_time;

        lod_controller::set_local_time_fn set_local_time;
        set_local_time.local_time = local_time;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), set_local_time);
    }

    void graphic_thing_instance::update_lod_level() {
        camera *pcur_camera = get_camera_manager().current_camera();
        if (!pcur_camera) {
            metin2::log::error("Cannot update the LOD level: no current camera");
            return;
        }

        const math::vector3 &target_position = pcur_camera->target();
        const math::vector3 &camera_position = pcur_camera->eye();
        const math::vector3 &position = get_position();

        // The distance from the centre ignores the z difference.
        lod_controller::update_lod_level_fn update;
        update.distance_from_center = sqrtf((target_position.x - position.x) * (target_position.x - position.x) +
                                            (target_position.y - position.y) * (target_position.y - position.y));
        update.distance_from_camera = sqrtf((camera_position.x - position.x) * (camera_position.x - position.x) +
                                            (camera_position.y - position.y) * (camera_position.y - position.y) +
                                            (camera_position.z - position.z) * (camera_position.z - position.z));

        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), update);
    }

    void graphic_thing_instance::update_time() {
        second_elapsed_ = get_timer().elapsed_second();

        if (delay_ > second_elapsed_) {
            delay_ -= second_elapsed_;
            second_elapsed_ = 0.0f;
        } else {
            second_elapsed_ -= delay_;
            delay_ = 0.0f;
        }

        local_time_ += second_elapsed_;

        lod_controller::update_time_fn update;
        update.elapsed_time = second_elapsed_;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), update);
    }

    void graphic_thing_instance::on_update() {
        update_lod_level();
        update_time();
    }

    void graphic_thing_instance::on_render() {
        render_with_one_texture();
    }

    void graphic_thing_instance::on_blend_render() {
        blend_render_with_one_texture();
    }

    void graphic_thing_instance::render_with_one_texture() {
        if (!updated_) {
            return;
        }

        lod_controller::render_with_one_texture_fn render;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), render);
    }

    void graphic_thing_instance::blend_render_with_one_texture() {
        if (!updated_) {
            return;
        }

        lod_controller::blend_render_with_one_texture_fn blend_render;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), blend_render);
    }

    void graphic_thing_instance::on_render_to_shadow_map() {
        if (!updated_) {
            return;
        }

        lod_controller::render_to_shadow_map_fn render_to_shadow_map;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), render_to_shadow_map);
    }

    void graphic_thing_instance::on_render_shadow() {
        lod_controller::render_shadow_fn render_shadow;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), render_shadow);
    }

    void graphic_thing_instance::on_render_pc_blocker() {
        lod_controller::render_with_one_texture_fn render_pc_blocker;
        std::for_each(lod_controller_vector_.begin(), lod_controller_vector_.end(), render_pc_blocker);
    }

    auto graphic_thing_instance::lod_controller_pointer(uint32_t model_index) const -> lod_controller * {
        assert(model_index < lod_controller_vector_.size());
        return lod_controller_vector_[model_index];
    }

    auto graphic_thing_instance::lod_controller_pointer(uint32_t model_index) -> lod_controller * {
        assert(model_index < lod_controller_vector_.size());
        return lod_controller_vector_[model_index];
    }

    auto graphic_thing_instance::lod_level(uint32_t model_instance) -> uint8_t {
        assert(model_instance < lod_controller_vector_.size());
        return (lod_controller_vector_[model_instance]->lod_level());
    }

    auto graphic_thing_instance::height() -> float {
        if (lod_controller_vector_.empty()) {
            return 0.0f;
        }

        model_instance *model_instance = lod_controller_vector_[0]->get_model_instance();
        if (!model_instance) {
            return 0.0f;
        }

        math::vector3 vt_min, vt_max;
        model_instance->get_bound_box(&vt_min, &vt_max);

        return fabs(vt_min.z - vt_max.z);
    }

    auto graphic_thing_instance::have_blend_thing() -> bool {
        for (size_t i = 0; i < lod_controller_vector_.size(); i++) {
            if (lod_controller_vector_[i]->have_blend_thing()) {
                return true;
            }
        }
        return false;
    }

    void graphic_thing_instance::on_clear() {
        stl_wipe(lod_controller_vector_);
        stl_wipe_second(ro_motion_thing_map_);

        for (uint32_t d = 0; d < model_thing_set_vector_.size(); ++d) {
            model_thing_set_vector_[d].clear();
        }
    }

    void graphic_thing_instance::on_initialize() {
        updated_ = false;
        local_time_ = 0.0f;
        delay_ = 0.0;
        second_elapsed_ = 0.0f;
        radius_ = -1.0f;
        center_ = math::vector3(0.0f, 0.0f, 0.0f);

        reset_local_time();
    }

    graphic_thing_instance::graphic_thing_instance() {
        initialize();
    }

    graphic_thing_instance::~graphic_thing_instance() = default;
}
