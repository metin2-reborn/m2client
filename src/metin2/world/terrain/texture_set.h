#pragma once

#include <metin2/gpu/types.h>
#include <metin2/graphics/grp_image_instance.h>
#include <metin2/math/math.h>

#include <cstdint>
#include <string>
#include <vector>

namespace metin2::inline world::inline terrain {
    struct terrain_texture {
        terrain_texture() : gpu_texture(nullptr), u_scale(4.0f), v_scale(4.0f), u_offset(0.0f), v_offset(0.0f), splat(true), begin(0), end(0) {
            transform = math::matrix_identity();
        }

        std::string filename;
        gpu::texture *gpu_texture;
        graphic_image_instance image_instance;
        float u_scale;
        float v_scale;
        float u_offset;
        float v_offset;
        bool splat;
        uint16_t begin, end; // 16-bit height field values, 0 to 65535.
        math::matrix transform;
        // Grass grows on tiles whose file name says grass or field; it is tinted with the tile's
        // average colour (0xAARRGGBB).
        bool grass = false;
        uint32_t average_color = 0xff708040;
    };

    class texture_set {
      public:
        using texture_vector = std::vector<terrain_texture>;

        texture_set();

        virtual ~texture_set();

        void clear();
        void create();

        auto load(const char *file_name, float terrain_tex_coord_base) -> bool;

        auto texture_count() -> uint32_t;
        auto texture(uint32_t index) -> terrain_texture &;

        auto set_texture(uint32_t index, const char *file_name, float fu_scale, float fv_scale, float fu_offset, float fv_offset, bool splat, uint16_t us_begin, uint16_t us_end, float terrain_tex_coord_base) -> bool;

      protected:
        void add_empty_texture();

      protected:
        texture_vector textures_;
        terrain_texture error_texture_;
        std::string file_name_;
    };
}
