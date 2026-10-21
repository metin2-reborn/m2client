#include "model.h"
#include "model_instance.h"

namespace metin2::inline models {

    void model_instance::copy_motion(model_instance *model_instance_value, bool is_free_source_control) {
        if (!model_instance_value->is_motion_playing())
            return;

        if (pgrn_ctrl_)
            GrannyFreeControl(pgrn_ctrl_);

        float local_time_value = local_time();
        pgrn_ani_ = model_instance_value->pgrn_ani_;
        pgrn_ctrl_ = GrannyPlayControlledAnimation(local_time_value, pgrn_ani_, pgrn_model_instance_);

        if (!pgrn_ctrl_)
            return;

        GrannySetControlSpeed(pgrn_ctrl_, GrannyGetControlSpeed(model_instance_value->pgrn_ctrl_));
        GrannySetControlLoopCount(pgrn_ctrl_, GrannyGetControlLoopCount(model_instance_value->pgrn_ctrl_));

        GrannySetControlEaseIn(pgrn_ctrl_, true);
        GrannySetControlEaseOut(pgrn_ctrl_, false);

        GrannySetControlRawLocalClock(pgrn_ctrl_, GrannyGetControlRawLocalClock(model_instance_value->pgrn_ctrl_));

        GrannyFreeControlOnceUnused(pgrn_ctrl_);

        if (is_free_source_control) {
            GrannyFreeControl(model_instance_value->pgrn_ctrl_);
            model_instance_value->pgrn_ctrl_ = nullptr;
        }
    }

    auto model_instance::is_motion_playing() -> bool {
        if (!pgrn_ctrl_)
            return false;

        if (GrannyControlIsComplete(pgrn_ctrl_))
            return false;

        return true;
    }

    void model_instance::set_motion_pointer(const motion *motion_value, float blend_time, int loop_count_value, float speed_ratio) {
        if (!pgrn_world_pose_real_)
            return;

        granny_model_instance *pgrn_model_instance = pgrn_model_instance_;
        if (!pgrn_model_instance)
            return;

        float local_time_value = local_time();

        bool is_first = false;
        if (pgrn_ctrl_) {
            GrannySetControlEaseOutCurve(pgrn_ctrl_, local_time_value, local_time_value + blend_time, 1.0f, 1.0f, 0.0f, 0.0f);

            GrannySetControlEaseIn(pgrn_ctrl_, false);
            GrannySetControlEaseOut(pgrn_ctrl_, true);

            GrannyCompleteControlAt(pgrn_ctrl_, local_time_value + blend_time);
            GrannyFreeControlIfComplete(pgrn_ctrl_);
        } else {
            is_first = true;
        }

        pgrn_ani_ = motion_value->granny_animation_pointer();
        pgrn_ctrl_ = GrannyPlayControlledAnimation(local_time_value, pgrn_ani_, pgrn_model_instance);
        if (!pgrn_ctrl_)
            return;

        GrannySetControlSpeed(pgrn_ctrl_, speed_ratio);
        GrannySetControlLoopCount(pgrn_ctrl_, loop_count_value);

        if (is_first) {
            GrannySetControlEaseIn(pgrn_ctrl_, false);
            GrannySetControlEaseOut(pgrn_ctrl_, false);
        } else {
            GrannySetControlEaseIn(pgrn_ctrl_, true);
            GrannySetControlEaseOut(pgrn_ctrl_, false);
            if (blend_time > 0.0f)
                GrannySetControlEaseInCurve(pgrn_ctrl_, local_time_value, local_time_value + blend_time, 0.0f, 0.0f, 1.0f, 1.0f);
        }

        GrannyFreeControlOnceUnused(pgrn_ctrl_);
    }

    void model_instance::change_motion_pointer(const motion *motion_value, int loop_count_value, float speed_ratio) {
        granny_model_instance *pgrn_model_instance = pgrn_model_instance_;
        if (!pgrn_model_instance)
            return;

        // Ignore part of the local time to skip the blended-in start.
        float skip_time = 0.3f;
        float local_time_value = local_time() - skip_time;

        if (pgrn_ctrl_) {
            GrannySetControlEaseIn(pgrn_ctrl_, false);
            GrannySetControlEaseOut(pgrn_ctrl_, false);
            GrannyCompleteControlAt(pgrn_ctrl_, local_time_value);
            GrannyFreeControlIfComplete(pgrn_ctrl_);
        }

        pgrn_ani_ = motion_value->granny_animation_pointer();
        pgrn_ctrl_ = GrannyPlayControlledAnimation(local_time_value, pgrn_ani_, pgrn_model_instance);
        if (!pgrn_ctrl_)
            return;

        GrannySetControlSpeed(pgrn_ctrl_, speed_ratio);
        GrannySetControlLoopCount(pgrn_ctrl_, loop_count_value);
        GrannySetControlEaseIn(pgrn_ctrl_, false);
        GrannySetControlEaseOut(pgrn_ctrl_, false);

        GrannyFreeControlOnceUnused(pgrn_ctrl_);
    }

    void model_instance::set_motion_at_end() {
        if (!pgrn_ctrl_)
            return;

        float ending_time = GrannyGetControlLocalDuration(pgrn_ctrl_);
        GrannySetControlRawLocalClock(pgrn_ctrl_, ending_time);
    }
}
