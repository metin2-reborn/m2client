#pragma once

#include <vector>

#include <metin2/graphics/image_file.h>

namespace metin2::inline ui {

    // Where a window is hit by the mouse: the opaque pixels (alpha of half or more) of an image,
    // stretched over the window. Lets a window that is not a rectangle, such as an empire's area
    // on the map, be picked only where it is painted.
    class hit_mask {
      public:
        // An image or a SubImage asset (only its rectangle of the image); false when it does not
        // load.
        auto load(const char *name) -> bool;
        // The rectangle left, top, right, bottom (exclusive) of a decoded image, clamped to it.
        void assign(const rgba_image &image, int left, int top, int right, int bottom);

        // Whether the point (x, y) of a window of the given size is on an opaque pixel.
        auto hit(int x, int y, int window_width, int window_height) const -> bool;

        auto empty() const -> bool { return opaque_.empty(); }

      private:
        int width_ = 0;
        int height_ = 0;
        std::vector<bool> opaque_; // row by row, top-down
    };
}
