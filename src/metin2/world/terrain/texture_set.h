#pragma once

#include <metin2/gpu/types.h>
#include <metin2/graphics/grp_image_instance.h>
#include <metin2/math/math.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace metin2::inline world::inline terrain {
    struct terrain_texture {
        terrain_texture() : gpu_texture(nullptr), u_scale(4.0f), v_scale(4.0f), u_offset(0.0f), v_offset(0.0f) {
            transform = math::matrix_identity();
        }

        std::string filename;
        gpu::texture *gpu_texture;
        graphic_image_instance image_instance;
        float u_scale;
        float v_scale;
        float u_offset;
        float v_offset;
        math::matrix transform;
        // Grass grows on tiles whose file name says grass or field; it is tinted with the tile's
        // average colour (0xAARRGGBB).
        bool grass = false;
        uint32_t average_color = 0xff708040;
    };

    // One [[texture]] of a texture set file.
    struct texture_set_entry {
        uint32_t index = 0; // the number the map's tile maps use
        std::string path;
        float u_scale = 4.0f;
        float v_scale = 4.0f;
        float u_offset = 0.0f;
        float v_offset = 0.0f;
    };

    // Reads a texture set (TOML, Textures:sets/*.toml). A bad entry is reported with its line and left
    // out; nullopt when the text is not TOML at all.
    auto parse_texture_set(std::string_view text, std::string_view name) -> std::optional<std::vector<texture_set_entry>>;

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

        auto set_texture(uint32_t index, const char *file_name, float fu_scale, float fv_scale, float fu_offset, float fv_offset, float terrain_tex_coord_base) -> bool;

      protected:
        void add_empty_texture();

      protected:
        texture_vector textures_;
        terrain_texture error_texture_;
        std::string file_name_;
    };
}
