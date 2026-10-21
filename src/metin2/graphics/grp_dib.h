#pragma once

#include <cstdint>
#include <vector>

namespace metin2::inline graphics {

    // A 32-bit pixel buffer that text is drawn into before it goes to a texture.
    class graphic_dib {
      public:
        void destroy();
        auto create(int width_value, int height) -> bool;

        auto width() -> int { return width_; }
        auto height() -> int { return height_; }

        auto pointer() -> void * { return pixels_.data(); }
        auto pixels() -> uint32_t * { return pixels_.data(); }

      protected:
        std::vector<uint32_t> pixels_;
        int width_ = 0;
        int height_ = 0;
    };
}
