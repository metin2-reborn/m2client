#pragma once

#include <cstdint>
#include <vector>

#include <granny.h>
#include <metin2/gpu/types.h>
#include <metin2/math/math.h>

#include <metin2/core/ref.h>
#include <metin2/core/reference_object.h>
#include <metin2/graphics/grp_image_instance.h>

namespace metin2::inline graphics {
    class graphic_image;
}

namespace metin2::inline models {

    struct material_data {
        graphic_image *image;
        float specular_power;
        bool is_specular_enable;
        uint8_t sphere_map_index;
    };

    class material : public reference_object {
      public:
        using ref = ref_ptr<material>;

        static void create_sphere_map(uint32_t map_index, const char *sphere_map_image_file_name);
        static void destroy_sphere_map();

      public:
        enum type {
            TYPE_DIFFUSE_PNT,
            TYPE_BLEND_PNT,
            TYPE_MAX_NUM
        };

      public:
        static void translate_specular_matrix(float add_x, float add_y, float add_z);

      private:
        static math::matrix specular_value_;
        static math::vector3 specular_trans_;

      public:
        material();

        ~material() override;

        void destroy();
        void copy(material &mtrl);
        auto is_equal(granny_material *material) const -> bool;
        auto is_in(const char *image_name, int *stage) -> bool;
        void set_specular_info(bool flag, float power, uint8_t sphere_map_index);

        void apply_render_state();
        void restore_render_state();

      protected:
        void initialize();

      public:
        auto create_from_granny_material_pointer(granny_material *material) -> bool;
        void set_image_pointer(int stage, graphic_image *image);

        auto get_type() const -> material::type;
        auto gpu_texture(int stage) const -> gpu::texture *;

      protected:
        auto image_pointer(const char *file_name) -> graphic_image *;

        auto specular_power() const -> float;

        void apply_diffuse_render_state();
        void restore_diffuse_render_state();
        void apply_specular_render_state();
        void restore_specular_render_state();

      protected:
        granny_material *pgrn_material_;
        graphic_image::ref ro_image_[2];
        type type_value_;

        float specular_power_;
        bool specular_enable_;
        bool two_side_render_;
        uint32_t last_cull_render_state_for_two_side_rendering_;
        uint8_t sphere_map_index_;

        void (material::*apply_render_state_)();
        void (material::*restore_render_state_)();

      private:
        enum {
            SPHEREMAP_NUM = 10,
        };
        static graphic_image_instance sphere_map_instance_[SPHEREMAP_NUM];
    };

    class material_palette {
      public:
        material_palette();
        virtual ~material_palette();

        void clear();
        void copy(const material_palette &mtrl_pal_src);

        auto register_material(granny_material *pgrn_material) -> uint32_t;
        void set_material_image_pointer(const char *image_name, graphic_image *image);
        void set_material_data(const char *mtrl_name, const material_data &material_data);
        void set_specular_info(const char *mtrl_name, bool enable, float power);

        auto material_ref(uint32_t mtrl_index) -> material &;

      protected:
        std::vector<material::ref> mtrl_vector_;
    };
}
