#include "hit_mask.h"

#include <algorithm>
#include <string>
#include <string_view>

#include <metin2/core/mapped_file.h>
#include <metin2/core/string_case.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/graphics/grp_sub_image.h>
#include <metin2/log/log.h>

namespace metin2::inline ui {

    namespace {
        auto read_file(const std::string &name, std::string &bytes) -> bool {
            mapped_file file;
            const void *data = nullptr;
            if (!get_file_manager().get(file, name.c_str(), &data) || !data)
                return false;
            bytes.assign(static_cast<const char *>(data), file.size());
            return true;
        }
    }

    auto hit_mask::load(const char *name) -> bool {
        std::string image_name = name;
        int rect[4] = {0, 0, 0, 0};
        bool sub_image = false;

        const std::string_view view(image_name);
        if (view.size() > 5 && iequals(view.substr(view.size() - 5), ".toml")) {
            std::string text;
            if (!read_file(image_name, text) || !graphic_sub_image::load(text, image_name, image_name, rect)) {
                metin2::log::error("Cannot load the hit mask {}", name);
                return false;
            }
            sub_image = true;
        }

        std::string bytes;
        rgba_image image;
        if (!read_file(image_name, bytes) || !decode_image(bytes.data(), bytes.size(), image)) {
            metin2::log::error("Cannot load the hit mask image {}", image_name);
            return false;
        }
        if (sub_image)
            assign(image, rect[0], rect[1], rect[2], rect[3]);
        else
            assign(image, 0, 0, image.width, image.height);
        return true;
    }

    void hit_mask::assign(const rgba_image &image, int left, int top, int right, int bottom) {
        left = std::clamp(left, 0, image.width);
        right = std::clamp(right, left, image.width);
        top = std::clamp(top, 0, image.height);
        bottom = std::clamp(bottom, top, image.height);

        width_ = right - left;
        height_ = bottom - top;
        opaque_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), true);
        if (!image.has_alpha)
            return;
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                const std::size_t pixel = static_cast<std::size_t>(top + y) * static_cast<std::size_t>(image.width) + static_cast<std::size_t>(left + x);
                opaque_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)] = image.pixels[pixel * 4 + 3] >= 128;
            }
        }
    }

    auto hit_mask::hit(int x, int y, int window_width, int window_height) const -> bool {
        if (opaque_.empty() || window_width <= 0 || window_height <= 0)
            return true;
        if (x < 0 || y < 0 || x >= window_width || y >= window_height)
            return false;
        const int mx = static_cast<int>(static_cast<int64_t>(x) * width_ / window_width);
        const int my = static_cast<int>(static_cast<int64_t>(y) * height_ / window_height);
        return opaque_[static_cast<std::size_t>(my) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(mx)];
    }
}
