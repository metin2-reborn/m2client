#include "thing.h"
#include <metin2/log/log.h>

namespace metin2::inline models {

    graphic_thing::graphic_thing(const char *file_name) : resource(file_name) {
        initialize();
    }

    graphic_thing::~graphic_thing() {
        clear();
    }

    void graphic_thing::initialize() {
        pgrn_file_ = nullptr;
        pgrn_file_info_ = nullptr;

        models_.reset();
        motions_.reset();
    }

    void graphic_thing::on_clear() {
        if (motions_) {
            motions_.reset();
        }

        if (models_) {
            models_.reset();
        }

        if (pgrn_file_) {
            GrannyFreeFile(pgrn_file_);
        }

        initialize();
    }

    auto graphic_thing::type() -> graphic_thing::type_type {
        static type_type s_type = string_to_type("CGraphicThing");
        return s_type;
    }

    auto graphic_thing::on_is_empty() const -> bool {
        return pgrn_file_ ? false : true;
    }

    auto graphic_thing::on_is_type(type_type type) -> bool {
        if (graphic_thing::type() == type) {
            return true;
        }

        return resource::on_is_type(type);
    }

    auto graphic_thing::create_device_objects() -> bool {
        if (!pgrn_file_info_) {
            return true;
        }

        for (int m = 0; m < pgrn_file_info_->ModelCount; ++m) {
            model &model = models_[m];
            model.create_device_objects();
        }

        return true;
    }

    void graphic_thing::destroy_device_objects() {
        if (!pgrn_file_info_) {
            return;
        }

        for (int m = 0; m < pgrn_file_info_->ModelCount; ++m) {
            model &model = models_[m];
            model.destroy_device_objects();
        }
    }

    auto graphic_thing::check_model_index(int model) const -> bool {
        if (!pgrn_file_info_) {
            metin2::log::debug("Model {} has no Granny file info", file_name());
            return false;
        }

        assert(pgrn_file_info_ != nullptr);

        if (model < 0) {
            return false;
        }

        if (model >= pgrn_file_info_->ModelCount) {
            return false;
        }

        return true;
    }

    auto graphic_thing::check_motion_index(int motion) const -> bool {
        if (!pgrn_file_info_) {
            return false;
        }

        assert(pgrn_file_info_ != nullptr);

        if (motion < 0) {
            return false;
        }

        if (motion >= pgrn_file_info_->AnimationCount) {
            return false;
        }

        return true;
    }

    auto graphic_thing::model_pointer(int model_value) -> model * {
        assert(check_model_index(model_value));
        assert(models_ != nullptr);
        return models_.get() + model_value;
    }

    auto graphic_thing::motion_pointer(int motion_value) -> motion * {
        assert(check_motion_index(motion_value));

        if (motion_value >= pgrn_file_info_->AnimationCount) {
            return nullptr;
        }

        assert(motions_ != nullptr);
        return (motions_.get() + motion_value);
    }

    auto graphic_thing::model_count() const -> int {
        if (!pgrn_file_info_) {
            return 0;
        }

        return (pgrn_file_info_->ModelCount);
    }

    auto graphic_thing::motion_count() const -> int {
        if (!pgrn_file_info_) {
            return 0;
        }

        return (pgrn_file_info_->AnimationCount);
    }

    auto graphic_thing::on_load(int size, const void *buf) -> bool {
        if (!buf) {
            return false;
        }

        pgrn_file_ = GrannyReadEntireFileFromMemory(size, (void *)buf);

        if (!pgrn_file_) {
            return false;
        }

        pgrn_file_info_ = GrannyGetFileInfo(pgrn_file_);

        if (!pgrn_file_info_) {
            return false;
        }

        load_models();
        load_motions();
        return true;
    }

    static std::string model_local_path;

    auto get_model_local_path() -> const std::string & {
        return model_local_path;
    }

    auto graphic_thing::load_models() -> bool {
        assert(pgrn_file_ != nullptr);
        assert(models_ == nullptr);

        if (pgrn_file_info_->ModelCount <= 0) {
            return false;
        }

        const std::string &file_name = file_name_string();

        // Relative model names resolve their relative texture names next to the model.
        model_local_path.clear();
        if (file_name.length() > 2 && file_name[1] != ':') {
            std::string::size_type sep_pos = file_name.rfind('\\');
            if (sep_pos != std::string::npos) {
                model_local_path.assign(file_name, 0, sep_pos + 1);
            }
        }

        int model_count = pgrn_file_info_->ModelCount;

        models_ = std::make_unique<model[]>(model_count);

        for (int m = 0; m < model_count; ++m) {
            model &model = models_[m];
            granny_model *pgrn_model = pgrn_file_info_->Models[m];

            if (!model.create_from_granny_model_pointer(pgrn_model)) {
                return false;
            }
        }

        GrannyFreeFileSection(pgrn_file_, GrannyStandardRigidVertexSection);
        GrannyFreeFileSection(pgrn_file_, GrannyStandardRigidIndexSection);
        GrannyFreeFileSection(pgrn_file_, GrannyStandardDeformableIndexSection);
        GrannyFreeFileSection(pgrn_file_, GrannyStandardTextureSection);
        return true;
    }

    auto graphic_thing::load_motions() -> bool {
        assert(pgrn_file_ != nullptr);
        assert(motions_ == nullptr);

        if (pgrn_file_info_->AnimationCount <= 0) {
            return false;
        }

        int motion_count = pgrn_file_info_->AnimationCount;

        motions_ = std::make_unique<motion[]>(motion_count);

        for (int m = 0; m < motion_count; ++m) {
            if (!motions_[m].bind_granny_animation(pgrn_file_info_->Animations[m])) {
                return false;
            }
        }

        return true;
    }
}
