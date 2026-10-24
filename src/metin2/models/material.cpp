#include "material.h"
#include <metin2/core/file_name.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/grp_screen.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>
#include "mesh.h"

#include <cmath>

namespace metin2::inline models {

    graphic_image_instance material::sphere_map_instance_[SPHEREMAP_NUM];

    math::vector3 material::specular_trans_(0.0f, 0.0f, 0.0f);
    math::matrix material::specular_value_;

    math::color specular_color = math::color(0.0f, 0.0f, 0.0f, 0.0f);

    void material::translate_specular_matrix(float add_x, float add_y, float add_z) {
        // The sphere map repeats every 1.0, so the offset wraps there; letting it grow lost float
        // precision and made the shine step after a few hours.
        specular_trans_.x = std::fmod(specular_trans_.x + add_x, 1.0f);
        specular_trans_.y = std::fmod(specular_trans_.y + add_y, 1.0f);
        specular_trans_.z = std::fmod(specular_trans_.z + add_z, 1.0f);

        specular_value_ = math::matrix_translation(specular_trans_.x, specular_trans_.y, specular_trans_.z);
    }

    void material::apply_render_state() {
        assert(apply_render_state_ != nullptr && "CGrannyMaterial::SaveRenderState");
        (this->*apply_render_state_)();
    }

    void material::restore_render_state() {
        assert(restore_render_state_ != nullptr && "CGrannyMaterial::RestoreRenderState");
        (this->*restore_render_state_)();
    }

    void material::copy(material &mtrl) {
        pgrn_material_ = mtrl.pgrn_material_;
        ro_image_[0] = mtrl.ro_image_[0];
        ro_image_[1] = mtrl.ro_image_[1];
        type_value_ = mtrl.type_value_;
        two_side_render_ = mtrl.two_side_render_;
        set_specular_info(mtrl.specular_enable_, mtrl.specular_power_, mtrl.sphere_map_index_);
    }

    material::material() {
        two_side_render_ = false;
        last_cull_render_state_for_two_side_rendering_ = gpu::cull::CW;

        initialize();
    }

    material::~material() = default;

    auto material::get_type() const -> material::type {
        return type_value_;
    }

    void material::set_image_pointer(int stage, graphic_image *image) {
        assert(stage < 2 && "CGrannyMaterial::SetImagePointer");
        ro_image_[stage] = image;
    }

    auto material::is_in(const char *c_sz_image_name, int *stage) -> bool {
        std::string str_image_name = c_sz_image_name;
        file_name_helper::string_path(str_image_name);

        granny_texture *pgrn_diffuse_texture = GrannyGetMaterialTextureByType(pgrn_material_, GrannyDiffuseColorTexture);
        if (pgrn_diffuse_texture) {
            std::string diffuse_file_name = pgrn_diffuse_texture->FromFileName;
            file_name_helper::string_path(diffuse_file_name);
            if (diffuse_file_name == str_image_name) {
                *stage = 0;
                return true;
            }
        }

        granny_texture *pgrn_opacity_texture = GrannyGetMaterialTextureByType(pgrn_material_, GrannyOpacityTexture);
        if (pgrn_opacity_texture) {
            std::string opacity_file_name = pgrn_opacity_texture->FromFileName;
            file_name_helper::string_path(opacity_file_name);
            if (opacity_file_name == str_image_name) {
                *stage = 1;
                return true;
            }
        }

        return false;
    }

    void material::set_specular_info(bool flag, float power, uint8_t sphere_map_index) {
        specular_power_ = power;
        sphere_map_index_ = sphere_map_index;
        specular_enable_ = flag;

        if (flag) {
            apply_render_state_ = &material::apply_specular_render_state;
            restore_render_state_ = &material::restore_specular_render_state;
        } else {
            apply_render_state_ = &material::apply_diffuse_render_state;
            restore_render_state_ = &material::restore_diffuse_render_state;
        }
    }

    auto material::is_equal(granny_material *pgrn_material) const -> bool {
        if (pgrn_material_ == pgrn_material) {
            return true;
        }

        return false;
    }

    auto material::gpu_texture(int stage) const -> gpu::texture * {
        const graphic_image::ref &rat_image = ro_image_[stage];

        if (rat_image.is_null()) {
            return nullptr;
        }

        graphic_image *image = rat_image.pointer();
        const graphic_texture *texture = image->texture_pointer();
        return texture->gpu_texture();
    }

    auto material::specular_power() const -> float {
        return specular_power_;
    }

    extern auto get_model_local_path() -> const std::string &;

    auto material::image_pointer(const char *file_name) -> graphic_image * {
        assert(*file_name != '\0');

        resource_manager &res_mgr = get_resource_manager();

        size_t file_name_len = strlen(file_name);
        if (file_name_len > 2 && file_name[1] != ':') {
            char local_file_name[256];
            const std::string &model_local_path_value = get_model_local_path();

            size_t local_file_name_len = model_local_path_value.length() + 1 + file_name_len;
            if (local_file_name_len < sizeof(local_file_name) - 1) {
                snprintf(local_file_name, sizeof(local_file_name), "%s%s", model_local_path_value.c_str(), file_name);
                resource *resource = res_mgr.resource_pointer(local_file_name);
                return static_cast<graphic_image *>(resource);
            }
        }

        resource *resource = res_mgr.resource_pointer(file_name);
        return static_cast<graphic_image *>(resource);
    }

    auto material::create_from_granny_material_pointer(granny_material *pgrn_material) -> bool {
        pgrn_material_ = pgrn_material;

        granny_texture *pgrn_diffuse_texture = nullptr;
        granny_texture *pgrn_opacity_texture = nullptr;

        if (pgrn_material) {
            if (pgrn_material->MapCount > 1 && !strncasecmp(pgrn_material->Name, "Blend", 5)) {
                pgrn_diffuse_texture = GrannyGetMaterialTextureByType(pgrn_material->Maps[0].Material, GrannyDiffuseColorTexture);
                pgrn_opacity_texture = GrannyGetMaterialTextureByType(pgrn_material->Maps[1].Material, GrannyDiffuseColorTexture);
            } else {
                pgrn_diffuse_texture = GrannyGetMaterialTextureByType(pgrn_material_, GrannyDiffuseColorTexture);
                pgrn_opacity_texture = GrannyGetMaterialTextureByType(pgrn_material_, GrannyOpacityTexture);
            }

            {
                granny_int32 two_sided = 0;
                granny_data_type_definition two_sided_field_type[] =
                    {
                        {GrannyInt32Member, "Two-sided"},
                        {GrannyEndMember},
                };

                granny_variant two_side_result;

                if (GrannyFindMatchingMember(pgrn_material->ExtendedData.Type, pgrn_material->ExtendedData.Object, "Two-sided", &two_side_result) && nullptr != two_side_result.Type) {
                    GrannyConvertSingleObject(two_side_result.Type, two_side_result.Object, two_sided_field_type, &two_sided, nullptr);
                }

                two_side_render_ = 1 == two_sided;
            }
        }

        if (pgrn_diffuse_texture) {
            ro_image_[0].set_pointer(image_pointer(pgrn_diffuse_texture->FromFileName));
        }

        if (pgrn_opacity_texture) {
            ro_image_[1].set_pointer(image_pointer(pgrn_opacity_texture->FromFileName));
        }

        // A mesh with an opacity map is a blending mesh.
        if (!ro_image_[1].is_null()) {
            type_value_ = TYPE_BLEND_PNT;
        } else {
            type_value_ = TYPE_DIFFUSE_PNT;
        }

        return true;
    }

    void material::initialize() {
        ro_image_[0] = nullptr;
        ro_image_[1] = nullptr;

        set_specular_info(false, 0.0f, 0);
    }

    void material::apply_diffuse_render_state() {
        STATEMANAGER.set_texture(0, gpu_texture(0));

        if (two_side_render_) {
            // Saving and restoring the cull mode tangles with the render order, so remember the previous value here instead.
            last_cull_render_state_for_two_side_rendering_ = STATEMANAGER.render_state(gpu::state::CULL_MODE);
            STATEMANAGER.set_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);
        }
    }

    void material::restore_diffuse_render_state() {
        if (two_side_render_) {
            STATEMANAGER.set_render_state(gpu::state::CULL_MODE, last_cull_render_state_for_two_side_rendering_);
        }
    }

    void material::apply_specular_render_state() {
        if (STATEMANAGER.render_state(gpu::state::ALPHA_BLEND_ENABLE)) {
            apply_diffuse_render_state();
            return;
        }

        graphic_texture *texture = sphere_map_index_ < SPHEREMAP_NUM ? sphere_map_instance_[sphere_map_index_].texture_pointer() : nullptr;

        STATEMANAGER.set_texture(0, gpu_texture(0));

        if (texture) {
            STATEMANAGER.set_texture(1, texture->gpu_texture());
        } else {
            STATEMANAGER.set_texture(1, nullptr);
        }

        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, math::color(specular_color.r, specular_color.g, specular_color.b, specular_power()));
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX, gpu::texcoord::CAMERA_SPACE_REFLECTION_VECTOR);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::TFACTOR);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE_ALPHA_ADD_COLOR);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);

        STATEMANAGER.set_transform(gpu::transform::TEXTURE1, &specular_value_);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::COUNT2);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);
    }

    void material::restore_specular_render_state() {
        if (STATEMANAGER.render_state(gpu::state::ALPHA_BLEND_ENABLE)) {
            restore_diffuse_render_state();
            return;
        }

        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_U);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_V);

        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG1);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG2);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_OP);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_OP);
    }

    void material::create_sphere_map(uint32_t map_index, const char *sphere_map_image_file_name) {
        if (map_index >= SPHEREMAP_NUM) {
            metin2::log::error("Sphere map index {} out of range", map_index);
            return;
        }

        resource_manager &res_mgr = get_resource_manager();
        auto *image = (graphic_image *)res_mgr.resource_pointer(sphere_map_image_file_name);
        sphere_map_instance_[map_index].set_image_pointer(image);
    }

    void material::destroy_sphere_map() {
        for (uint32_t map_index = 0; map_index < SPHEREMAP_NUM; ++map_index) {
            sphere_map_instance_[map_index].destroy();
        }
    }

    material_palette::material_palette() = default;

    material_palette::~material_palette() {
        clear();
    }

    void material_palette::copy(const material_palette &mtrl_pal_src) {
        mtrl_vector_ = mtrl_pal_src.mtrl_vector_;
    }

    void material_palette::clear() {
        mtrl_vector_.clear();
    }

    auto material_palette::material_ref(uint32_t mtrl_index) -> material & {
        assert(mtrl_index < mtrl_vector_.size());
        return *mtrl_vector_[mtrl_index].pointer();
    }

    void material_palette::set_material_image_pointer(const char *image_name, graphic_image *image) {
        uint32_t size = mtrl_vector_.size();
        uint32_t i;
        for (i = 0; i < size; ++i) {
            material::ref &ro_mtrl = mtrl_vector_[i];

            int stage;
            if (ro_mtrl->is_in(image_name, &stage)) {
                auto *new_mtrl = new material;
                new_mtrl->copy(*ro_mtrl.pointer());
                new_mtrl->set_image_pointer(stage, image);
                ro_mtrl = new_mtrl;

                return;
            }
        }
    }

    void material_palette::set_material_data(const char *mtrl_name, const material_data &material_data) {
        if (mtrl_name) {
            std::vector<material::ref>::iterator i;
            for (i = mtrl_vector_.begin(); i != mtrl_vector_.end(); ++i) {
                material::ref &ro_mtrl = *i;

                int stage;
                if (ro_mtrl->is_in(mtrl_name, &stage)) {
                    auto *new_mtrl = new material;
                    new_mtrl->copy(*ro_mtrl.pointer());
                    new_mtrl->set_image_pointer(stage, material_data.image);
                    new_mtrl->set_specular_info(material_data.is_specular_enable, material_data.specular_power, material_data.sphere_map_index);
                    ro_mtrl = new_mtrl;

                    return;
                }
            }
        } else {
            std::vector<material::ref>::iterator i;
            for (i = mtrl_vector_.begin(); i != mtrl_vector_.end(); ++i) {
                material::ref &ro_mtrl = *i;
                ro_mtrl->set_specular_info(material_data.is_specular_enable, material_data.specular_power, material_data.sphere_map_index);
            }
        }
    }

    void material_palette::set_specular_info(const char *mtrl_name, bool enable, float power) {
        uint32_t size = mtrl_vector_.size();
        uint32_t i;
        if (mtrl_name) {
            for (i = 0; i < size; ++i) {
                material::ref &ro_mtrl = mtrl_vector_[i];

                int stage;
                if (ro_mtrl->is_in(mtrl_name, &stage)) {
                    ro_mtrl->set_specular_info(enable, power, 0);
                    return;
                }
            }
        } else {
            for (i = 0; i < size; ++i) {
                material::ref &ro_mtrl = mtrl_vector_[i];
                ro_mtrl->set_specular_info(enable, power, 0);
            }
        }
    }

    auto material_palette::register_material(granny_material *pgrn_material) -> uint32_t {
        uint32_t size = mtrl_vector_.size();
        uint32_t i;
        for (i = 0; i < size; ++i) {
            material::ref &ro_mtrl = mtrl_vector_[i];
            if (ro_mtrl->is_equal(pgrn_material)) {
                return i;
            }
        }

        auto *new_mtrl = new material;
        new_mtrl->create_from_granny_material_pointer(pgrn_material);
        mtrl_vector_.push_back(new_mtrl);

        return size;
    }
}
