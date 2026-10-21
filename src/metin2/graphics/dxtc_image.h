#pragma once

#include <cstddef>
#include <cstdint>

namespace metin2::inline graphics {

    enum pix_format {
        PF_DXT1,
        PF_DXT3,
        PF_DXT5,
        PF_UNKNOWN,
    };

    // Reads the header of a DXT1-5 .dds file held in memory and copies its mip levels out. DXT2 and
    // DXT4 (premultiplied alpha) are read as DXT3 and DXT5.
    // The buffer must stay alive between LoadHeaderFromMemory and the Copy calls.
    class dxtc_image {
      public:
        static constexpr int MAX_MIPLEVELS = 12;

        dxtc_image();

        // Returns true if the buffer is a DXT1-5 DDS file with at least its first level complete.
        auto load_header_from_memory(const uint8_t *map, size_t size) -> bool;

        // Copies one mip level into a locked texture surface with the given row pitch.
        auto copy(int miplevel, uint8_t *dest, int32_t dest_pitch) const -> bool;
        // Decodes one mip level to R, G, B, A bytes (level width x level height pixels).
        auto decompress(int miplevel, uint8_t *rgba) const -> bool;

        auto level_width(int miplevel) const -> int { return width >> miplevel ? width >> miplevel : 1; }
        auto level_height(int miplevel) const -> int { return height >> miplevel ? height >> miplevel : 1; }

        pix_format comp_format;
        uint32_t mip_map_count;
        int width; // in pixels of the uncompressed image
        int height;

      private:
        void initialize();

        const uint8_t *comp_buffer_by_levels_[MAX_MIPLEVELS];
    };
}
