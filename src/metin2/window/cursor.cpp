#include "cursor.h"

#include <metin2/core/mapped_file.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>

#include <GLFW/glfw3.h>
#include <raylib.h>

#include <cstdlib>
#include <span>
#include <string>
#include <vector>

namespace metin2::inline window {

    namespace {
        struct cursor_image {
            int width = 0;
            int height = 0;
            int hot_x = 0;
            int hot_y = 0;
            std::vector<unsigned char> rgba;
        };

        // A .cur file is an ICO container whose entries hold a BITMAPINFOHEADER image (twice the height:
        // colour rows, then the 1-bit transparency mask), stored bottom-up.
        auto load_cur_file(std::span<const unsigned char> data, cursor_image &image) -> bool {
            if (data.size() < 22)
                return false;

            auto read16 = [&](size_t offset) { return static_cast<uint32_t>(data[offset]) | (static_cast<uint32_t>(data[offset + 1]) << 8); };
            auto read32 = [&](size_t offset) { return read16(offset) | (read16(offset + 2) << 16); };

            const uint32_t count = read16(4);
            if (count == 0)
                return false;

            // The first entry, or the first 32x32 one.
            size_t entry = 6;
            for (uint32_t i = 0; i < count; ++i) {
                const size_t e = 6 + i * 16;
                if (e + 16 <= data.size() && data[e] == 32 && data[e + 1] == 32) {
                    entry = e;
                    break;
                }
            }
            image.hot_x = static_cast<int>(read16(entry + 4));
            image.hot_y = static_cast<int>(read16(entry + 6));
            const uint32_t size = read32(entry + 8);
            const uint32_t offset = read32(entry + 12);
            if (offset + size > data.size() || size < 40)
                return false;

            const uint32_t header_size = read32(offset);
            const int width = static_cast<int>(read32(offset + 4));
            const int height = static_cast<int>(read32(offset + 8)) / 2;
            const int bpp = static_cast<int>(read16(offset + 14));
            const uint32_t colors_used = read32(offset + 32);
            if (width <= 0 || height <= 0)
                return false;

            const unsigned char *p = &data[offset + header_size];
            std::vector<uint32_t> palette;
            if (bpp <= 8) {
                const uint32_t colors = colors_used ? colors_used : (1u << bpp);
                for (uint32_t i = 0; i < colors; ++i, p += 4)
                    palette.push_back(static_cast<uint32_t>(p[0]) | (p[1] << 8) | (p[2] << 16));
            }

            const size_t row_bytes = (static_cast<size_t>(width) * bpp + 31) / 32 * 4;
            const size_t mask_row_bytes = (static_cast<size_t>(width) + 31) / 32 * 4;
            const unsigned char *colors = p;
            const unsigned char *mask = colors + row_bytes * height;
            if (mask + mask_row_bytes * height > data.data() + data.size())
                return false;

            image.width = width;
            image.height = height;
            image.rgba.assign(static_cast<size_t>(width) * height * 4, 0);
            for (int y = 0; y < height; ++y) {
                const unsigned char *row = colors + row_bytes * (height - 1 - y);
                const unsigned char *mask_row = mask + mask_row_bytes * (height - 1 - y);
                for (int x = 0; x < width; ++x) {
                    uint32_t bgr = 0;
                    unsigned char alpha = 255;
                    switch (bpp) {
                    case 32:
                        bgr = row[x * 4] | (row[x * 4 + 1] << 8) | (row[x * 4 + 2] << 16);
                        alpha = row[x * 4 + 3];
                        break;
                    case 24:
                        bgr = row[x * 3] | (row[x * 3 + 1] << 8) | (row[x * 3 + 2] << 16);
                        break;
                    case 8:
                        bgr = palette[row[x]];
                        break;
                    case 4:
                        bgr = palette[(x & 1) ? (row[x / 2] & 0xf) : (row[x / 2] >> 4)];
                        break;
                    case 1:
                        bgr = palette[(row[x / 8] >> (7 - (x & 7))) & 1];
                        break;
                    default:
                        break;
                    }
                    const bool masked = ((mask_row[x / 8] >> (7 - (x & 7))) & 1) != 0;
                    if (bpp != 32 || alpha == 0)
                        alpha = masked ? 0 : 255;
                    unsigned char *out = &image.rgba[(static_cast<size_t>(y) * width + x) * 4];
                    out[0] = static_cast<unsigned char>(bgr >> 16);
                    out[1] = static_cast<unsigned char>(bgr >> 8);
                    out[2] = static_cast<unsigned char>(bgr);
                    out[3] = alpha;
                }
            }
            return true;
        }

        // File names as in the Windows client's resource script.
        auto cursor_file_name(int resource_id) -> const char * {
            switch (resource_id) {
            case 102:
                return "cursor.cur";
            case 104:
                return "cursor_attack.cur";
            case 105:
                return "cursor_chair.cur";
            case 106:
                return "cursor_door.cur";
            case 107:
                return "cursor_no.cur";
            case 108:
                return "cursor_pick.cur";
            case 109:
                return "cursor_talk.cur";
            case 110:
                return "cursor_buy.cur";
            case 111:
                return "cursor_sell.cur";
            case 114:
                return "cursor_pick.cur"; // the pick-up cursor has no file of its own
            case 139:
                return "cursor_camera_rotate.cur";
            case 140:
                return "cursor_hsize.cur";
            case 141:
                return "cursor_vsize.cur";
            case 142:
                return "cursor_hvsize.cur";
            default:
                return nullptr;
            }
        }
    }

    auto cursor::load(int resource_id) -> std::unique_ptr<cursor> {
        const char *file_name = cursor_file_name(resource_id);
        if (!file_name) {
            metin2::log::error("No cursor file for cursor {}", resource_id);
            return nullptr;
        }

        const std::string path = std::string("ui:cursor/") + file_name;

        mapped_file file;
        const void *data = nullptr;
        cursor_image image;
        if (!get_file_manager().get(file, path.c_str(), &data) || !load_cur_file({static_cast<const unsigned char *>(data), file.size()}, image)) {
            metin2::log::error("Cannot load the cursor {}", path);
            return nullptr;
        }

        GLFWimage glfw_image = {image.width, image.height, image.rgba.data()};
        GLFWcursor *handle = glfwCreateCursor(&glfw_image, image.hot_x, image.hot_y);
        if (!handle)
            return nullptr;
        return std::unique_ptr<cursor>(new cursor(handle));
    }

    cursor::~cursor() {
        glfwDestroyCursor(static_cast<GLFWcursor *>(handle_));
    }

    void set_cursor(const cursor *c) {
        // raylib's GetWindowHandle() is the native handle; its GLFW window owns the current context.
        glfwSetCursor(glfwGetCurrentContext(), c ? static_cast<GLFWcursor *>(c->handle()) : nullptr);
    }

    void show_cursor(bool show) {
        if (show)
            ShowCursor();
        else
            HideCursor();
    }
}
