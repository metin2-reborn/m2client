#include "lod_controller.h"
#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include <metin2/log/log.h>

namespace metin2::inline models {

    static float lod_height_actor = 500.0f;
    static float lod_distance_actor = 5000.0f;
    static float lod_distance_building = 25000.0f;

    static const float LOD_APPLY_MIN = 500.0f;

    static bool is_min_lod_mode_enable = false;

    enum {
        SHARED_VB_500 = 0,
        SHARED_VB_1000 = 1,
        SHARED_VB_1500 = 2,
        SHARED_VB_2000 = 3,
        SHARED_VB_2500 = 4,
        SHARED_VB_3000 = 5,
        SHARED_VB_3500 = 6,
        SHARED_VB_4000 = 7,
        SHARED_VB_NUM = 9,
    };

    static std::vector<graphic_vertex_buffer *> shared_vbs[SHARED_VB_NUM];

    static graphic_vertex_buffer empty_vb;

    static auto alloc_deform_vertex_buffer(unsigned deformable_vertex_count) -> graphic_vertex_buffer * {
        if (deformable_vertex_count == 0)
            return &empty_vb;

        unsigned capacity = (((deformable_vertex_count - 1) / 500) + 1) * 500;
        unsigned index = (deformable_vertex_count - 1) / 500;
        if (index < SHARED_VB_NUM) {
            std::vector<graphic_vertex_buffer *> &vbs = shared_vbs[index];
            if (!vbs.empty()) {
                graphic_vertex_buffer *ret_vb = vbs.back();
                vbs.pop_back();
                return ret_vb;
            }
        }

        auto *new_vb = new graphic_vertex_buffer;

        if (!new_vb->create(
                capacity,
                gpu::fvf::XYZ | gpu::fvf::NORMAL | gpu::fvf::TEX1,
                gpu::usage::WRITE_ONLY)) {
            metin2::log::error("Cannot create a deform vertex buffer of {} vertices ({} requested)", capacity, deformable_vertex_count);
        }

        return new_vb;
    }

    static void free_deform_vertex_buffer(graphic_vertex_buffer *del_vb) {
        if (del_vb) {
            if (del_vb == &empty_vb)
                return;

            unsigned index = (del_vb->vertex_count() - 1) / 500;
            if (index < SHARED_VB_NUM) {
                shared_vbs[index].push_back(del_vb);
            } else {
                del_vb->destroy();
                delete del_vb;
            }
        }
    }

    static void reserve_shared_vertex_buffers(unsigned index, unsigned count) {
        if (index >= SHARED_VB_NUM)
            return;

        unsigned capacity = (index + 1) * 500;

        for (unsigned i = 0; i != count; ++i) {
            auto *new_vb = new graphic_vertex_buffer;
            new_vb->create(
                capacity,
                gpu::fvf::XYZ | gpu::fvf::NORMAL | gpu::fvf::TEX1,
                gpu::usage::WRITE_ONLY);
            shared_vbs[index].push_back(new_vb);
        }
    }

    void granny_create_shared_deform_buffer() {
        reserve_shared_vertex_buffers(SHARED_VB_500, 40);
        reserve_shared_vertex_buffers(SHARED_VB_1000, 20);
        reserve_shared_vertex_buffers(SHARED_VB_1500, 20);
        reserve_shared_vertex_buffers(SHARED_VB_2000, 40);
        reserve_shared_vertex_buffers(SHARED_VB_3000, 20);
    }

    void granny_destroy_shared_deform_buffer() {
        for (int i = 0; i != SHARED_VB_NUM; ++i) {
            std::vector<graphic_vertex_buffer *> &vbs = shared_vbs[i];

            std::vector<graphic_vertex_buffer *>::iterator v;
            for (v = vbs.begin(); v != vbs.end(); ++v) {
                graphic_vertex_buffer *each_vb = (*v);
                each_vb->destroy();
                delete each_vb;
            }
            vbs.clear();
        }
    }

    void lod_controller::set_min_lod_mode(bool is_enable) {
        is_min_lod_mode_enable = is_enable;
    }

    void lod_controller::set_material_image_pointer(const char *image_name, graphic_image *image) {
        std::deque<model_instance *>::iterator i;
        for (i = que_model_inst_.begin(); i != que_model_inst_.end(); ++i) {
            model_instance *model_inst = (*i);
            model_inst->set_material_image_pointer(image_name, image);
        }
    }

    void lod_controller::set_material_data(const char *image_name, const material_data &material_data) {
        std::deque<model_instance *>::iterator i;
        for (i = que_model_inst_.begin(); i != que_model_inst_.end(); ++i) {
            model_instance *model_inst = (*i);
            model_inst->set_material_data(image_name, material_data);
        }
    }

    void lod_controller::set_specular_info(const char *mtrl_name, bool enable, float power) {
        std::deque<model_instance *>::iterator i;
        for (i = que_model_inst_.begin(); i != que_model_inst_.end(); ++i) {
            model_instance *model_inst = (*i);
            model_inst->set_specular_info(mtrl_name, enable, power);
        }
    }

    lod_controller::lod_controller() : lod_distance_(0.0f),
                                       lod_ani_fps_(model_instance::ANIFPS_MAX),
                                       attached_parent_model_(nullptr),
                                       lod_level_(0),
                                       current_model_instance_(nullptr),
                                       shared_deformable_vertex_buffer_(nullptr) {
    }

    lod_controller::~lod_controller() {
        free_deform_vertex_buffer(shared_deformable_vertex_buffer_);

        clear();
    }

    void lod_controller::clear() {
        if (attached_parent_model_) {
            attached_parent_model_->detach_model_instance(this);
        }

        current_model_instance_ = nullptr;
        attached_parent_model_ = nullptr;

        std::for_each(que_model_inst_.begin(), que_model_inst_.end(), model_instance::delete_value);
        que_model_inst_.clear();

        auto itor = attached_model_data_vector_.begin();
        for (; attached_model_data_vector_.end() != itor; ++itor) {
            attaching_model_data &data = *itor;
            data.lod_controller_value->attached_parent_model_ = nullptr;
        }

        attached_model_data_vector_.clear();
    }

    void lod_controller::add_model(graphic_thing *thing, int src_model, lod_controller *skel_lod_controller) {
        if (!thing)
            return;

        if (skel_lod_controller && skel_lod_controller->que_model_inst_.empty()) {
            assert(!"EMPTY SKELETON(CANNON LINK)");
            return;
        }

        assert(thing->reference_count() >= 1);

        thing->add_reference();

        if (thing->model_count() <= src_model) {
            thing->release();
            return;
        }
        model *model = thing->model_pointer(src_model);
        if (!model) {
            thing->release();
            return;
        }

        model_instance *model_instance = model_instance::new_value();

        reserve_shared_deformable_vertex_buffer(model->deform_vertex_count());

        if (skel_lod_controller) {
            model_instance->set_linked_model_pointer(model, shared_deformable_vertex_buffer_, &skel_lod_controller->current_model_instance_);
        } else {
            model_instance->set_linked_model_pointer(model, shared_deformable_vertex_buffer_, nullptr);
        }

        if (!current_model_instance_) {
            current_model_instance_ = model_instance;
            model_instance->deform_no_skin(&identity_);

            math::vector3 vt_min, vt_max;
            model_instance->get_bound_box(&vt_min, &vt_max);

            float size = 0.0f;
            size = f_max(size, fabs(vt_min.x - vt_max.x));
            size = f_max(size, fabs(vt_min.y - vt_max.y));
            size = f_max(size, fabs(vt_min.z - vt_max.z));

            if (size < lod_height_actor)
                set_lod_limits(0.0f, lod_distance_actor);
            else
                set_lod_limits(0.0f, lod_distance_building);
        } else {
            // FIXME: update() reads the model instance's world pose, which stays null until a deform;
            // the structure may need a small change.
            model_instance->deform_no_skin(&identity_);
        }

        thing->release();

        que_model_inst_.push_front(model_instance);
    }

    void lod_controller::reserve_shared_deformable_vertex_buffer(uint32_t deformable_vertex_count) {
        if (shared_deformable_vertex_buffer_ &&
            uint32_t(shared_deformable_vertex_buffer_->vertex_count()) >= deformable_vertex_count)
            return;

        free_deform_vertex_buffer(shared_deformable_vertex_buffer_);

        shared_deformable_vertex_buffer_ = alloc_deform_vertex_buffer(deformable_vertex_count);
    }

    void lod_controller::attach_model_instance(lod_controller *src_lod_controller, const char *bone_name) {
        model_instance *src_instance = src_lod_controller->get_model_instance();
        if (!src_instance)
            return;

        model_instance *dest_instance = get_model_instance();
        if (dest_instance) {
            src_instance->set_parent_model_instance(dest_instance, bone_name);
        }

        if (!src_lod_controller->get_model_instance())
            return;

        src_lod_controller->attached_parent_model_ = this;

        auto itor = attached_model_data_vector_.begin();
        for (; attached_model_data_vector_.end() != itor;) {
            attaching_model_data &data = *itor;
            if (src_lod_controller == data.lod_controller_value) {
                itor = attached_model_data_vector_.erase(itor);
            } else {
                ++itor;
            }
        }

        attaching_model_data attaching_model_data_value;
        attaching_model_data_value.lod_controller_value = src_lod_controller;
        attaching_model_data_value.bone_name = bone_name;
        attached_model_data_vector_.push_back(attaching_model_data_value);
    }

    void lod_controller::detach_model_instance(lod_controller *src_lod_controller) {
        model_instance *src_instance = src_lod_controller->get_model_instance();
        if (!src_instance)
            return;

        model_instance *dest_instance = get_model_instance();
        if (dest_instance) {
            src_instance->set_parent_model_instance(nullptr, 0);
        }

        auto itor = attached_model_data_vector_.begin();
        for (; attached_model_data_vector_.end() != itor;) {
            attaching_model_data &data = *itor;
            if (src_lod_controller == data.lod_controller_value) {
                itor = attached_model_data_vector_.erase(itor);
            } else {
                ++itor;
            }
        }

        src_lod_controller->attached_parent_model_ = nullptr;
    }

    void lod_controller::set_lod_limits(float /*fNearLOD*/, float far_lod) {
        lod_distance_ = far_lod;
    }

    void lod_controller::set_lod_level(uint8_t lod_level) {
        assert(que_model_inst_.size() > 0);

        if (que_model_inst_.size() > 0)
            lod_level_ = (uint8_t)std::min<size_t>(que_model_inst_.size() - 1, lod_level);
    }

    void lod_controller::create_device_objects() {
        std::for_each(que_model_inst_.begin(),
            que_model_inst_.end(),
            model_instance::create_device_objects_fn());
    }

    void lod_controller::destroy_device_objects() {
        std::for_each(que_model_inst_.begin(),
            que_model_inst_.end(),
            model_instance::destroy_device_objects_fn());
    }

    void lod_controller::render_with_one_texture() {
        assert(current_model_instance_ != nullptr);

        current_model_instance_->render_with_one_texture();
    }

    void lod_controller::blend_render_with_one_texture() {
        assert(current_model_instance_ != nullptr);
        current_model_instance_->blend_render_with_one_texture();
    }

    void lod_controller::update_lod_level(float distance_from_center, float distance_from_camera) {
        if (que_model_inst_.size() <= 1)
            return;

        assert(current_model_instance_ != nullptr);

        if (distance_from_center > LOD_APPLY_MIN) // No LOD near the centre.
        {
            // The animation rate drops from ANIFPS_MAX towards ANIFPS_MIN as the distance from the camera grows.
            float lod_factor = f_minmax(0.0f, (lod_distance_ - distance_from_camera), lod_distance_);

            if (lod_distance_ > 0.0f)
                lod_ani_fps_ = (uint32_t)((model_instance::ANIFPS_MAX - model_instance::ANIFPS_MIN) * lod_factor / lod_distance_ + static_cast<float>(model_instance::ANIFPS_MIN));
            else
                lod_ani_fps_ = model_instance::ANIFPS_MIN;

            assert(lod_ani_fps_ > 0);
            lod_ani_fps_ /= 10;
            lod_ani_fps_ *= 10;

            float lod_step = lod_distance_ / que_model_inst_.size();
            auto lod_level = uint8_t(lod_factor / lod_step);

            if (lod_distance_ <= 5000.0f) {
                if (distance_from_camera < 500.0f) {
                    lod_level = 0;
                } else if (distance_from_camera < 1500.0f) {
                    lod_level = 1;
                } else if (distance_from_camera < 2500.0f) {
                    lod_level = 2;
                } else {
                    lod_level = 3;
                }

                lod_level = (uint8_t)(que_model_inst_.size() - std::min<size_t>(lod_level, que_model_inst_.size()) - 1);
            }

            if (is_min_lod_mode_enable)
                lod_level = 0;

            set_lod_level(lod_level);

            if (current_model_instance_ != que_model_inst_[lod_level_]) {
                set_current_model_instance(que_model_inst_[lod_level_]);
            }
        } else {
            lod_ani_fps_ = model_instance::ANIFPS_MAX;

            if (!que_model_inst_.empty()) {
                if (current_model_instance_ != que_model_inst_.back()) {
                    set_current_model_instance(que_model_inst_.back());
                }
            }
        }
    }

    void lod_controller::update_time(float elapsed_time) {
        assert(current_model_instance_ != nullptr);

        current_model_instance_->update(lod_ani_fps_);
        current_model_instance_->update_local_time(elapsed_time);
    }

    void lod_controller::set_current_model_instance(model_instance *pgrn_model_instance) {
        pgrn_model_instance->copy_motion(current_model_instance_, true);
        current_model_instance_ = pgrn_model_instance;

        refresh_attached_model_instance();

        if (attached_parent_model_) {
            attached_parent_model_->refresh_attached_model_instance();
        }
    }

    void lod_controller::refresh_attached_model_instance() {
        if (!current_model_instance_)
            return;

        for (size_t i = 0; i < attached_model_data_vector_.size(); ++i) {
            attaching_model_data &model_data = attached_model_data_vector_[i];

            model_instance *src_instance = model_data.lod_controller_value->get_model_instance();
            if (!src_instance) {
                metin2::log::debug("Attached model {} has no model instance", i);
                continue;
            }

            src_instance->set_parent_model_instance(current_model_instance_, model_data.bone_name.c_str());
        }
    }

    void lod_controller::deform_all(const math::matrix *world_matrix) {
        std::deque<model_instance *>::iterator i;
        for (i = que_model_inst_.begin(); i != que_model_inst_.end(); ++i) {
            model_instance *model_inst = (*i);
            model_inst->deform(world_matrix);
        }
    }

    void lod_controller::deform_no_skin(const math::matrix *world_matrix) {
        if (current_model_instance_)
            current_model_instance_->deform_no_skin(world_matrix);
    }

    void lod_controller::deform(const math::matrix *world_matrix) {
        if (current_model_instance_)
            current_model_instance_->deform(world_matrix);
    }

    void lod_controller::render_to_shadow_map() {
        if (current_model_instance_)
            current_model_instance_->render_without_texture();
    }

    void lod_controller::render_shadow() {
        if (current_model_instance_)
            current_model_instance_->render_with_one_texture();
    }

    void lod_controller::get_bound_box(math::vector3 *vt_min, math::vector3 *vt_max) {
        if (current_model_instance_)
            current_model_instance_->get_bound_box(vt_min, vt_max);
    }

    auto lod_controller::intersect(const math::matrix *matrix, float *u, float *v, float *t) -> bool {
        if (!current_model_instance_)
            return false;
        return current_model_instance_->intersect(matrix, u, v, t);
    }

    void lod_controller::set_local_time(float local_time) {
        if (current_model_instance_)
            current_model_instance_->set_local_time(local_time);
    }

    void lod_controller::reset_local_time() {
        assert(current_model_instance_ != nullptr);
        current_model_instance_->reset_local_time();
    }

    void lod_controller::set_motion_pointer(const motion *motion, float blend_time, int loop_count, float speed_ratio) {
        assert(current_model_instance_ != nullptr);
        current_model_instance_->set_motion_pointer(motion, blend_time, loop_count, speed_ratio);
    }

    void lod_controller::change_motion_pointer(const motion *motion, int loop_count, float speed_ratio) {
        assert(current_model_instance_ != nullptr);
        current_model_instance_->change_motion_pointer(motion, loop_count, speed_ratio);
    }

    void lod_controller::set_motion_at_end() {
        if (current_model_instance_)
            current_model_instance_->set_motion_at_end();
    }

    auto lod_controller::is_model_instance() -> bool {
        if (!current_model_instance_)
            return false;

        return true;
    }

    auto lod_controller::get_model_instance() -> model_instance * {
        return current_model_instance_;
    }
}
