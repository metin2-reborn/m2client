#pragma once

#include "dxtc_image.h"
#include "grp_texture.h"

namespace metin2::inline graphics {

    class graphic_image_texture : public graphic_texture {
      public:
        graphic_image_texture();
        ~graphic_image_texture() override;

        void destroy();

        auto create(uint32_t width_value, uint32_t height, gpu::format fmt) -> bool;
        auto create_device_objects() -> bool;

        void create_from_texture_pointer(const graphic_texture *src_texture);
        // A DDS, TGA, PNG, BMP or JPEG file with a full mip chain. Magenta (0xff00ff) is transparent in
        // files without alpha. UNKNOWN picks a format from the file.
        auto create_from_memory_file(uint32_t buf_size, const void *buf, gpu::format fmt) -> bool;
        auto create_dds_texture(dxtc_image &image) -> bool;

        void set_file_name(const char *file_name);

        auto lock(int *ret_pitch, void **ret_pixels, int level = 0) -> bool;
        void unlock(int level = 0);

      protected:
        void initialize();

        gpu::format format_;

        std::string file_name_;
    };
}
