#include "grp_dib.h"

namespace metin2::inline graphics {

    void graphic_dib::destroy() {
        pixels_.clear();
        width_ = 0;
        height_ = 0;
    }

    auto graphic_dib::create(int width_value, int height) -> bool {
        if (width_value <= 0 || height <= 0)
            return false;
        width_ = width_value;
        height_ = height;
        pixels_.assign(static_cast<size_t>(width_) * height_, 0);
        return true;
    }
}
