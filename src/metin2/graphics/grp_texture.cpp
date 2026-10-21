#include "grp_texture.h"
#include <cassert>
#include "state_manager.h"

namespace metin2::inline graphics {

    void graphic_texture::destroy_device_objects() {
        texture_.reset();
    }

    void graphic_texture::destroy() {
        destroy_device_objects();

        initialize();
    }

    void graphic_texture::initialize() {
        texture_.reset();
        width_ = 0;
        height_ = 0;
        empty_ = true;
    }

    auto graphic_texture::is_empty() const -> bool {
        return empty_;
    }

    void graphic_texture::set_texture_stage(int stage) const {
        assert(device_);
        STATEMANAGER.set_texture(stage, texture_.get());
    }

    auto graphic_texture::width() const -> int {
        return width_;
    }

    auto graphic_texture::height() const -> int {
        return height_;
    }

    graphic_texture::graphic_texture() {
        initialize();
    }

    graphic_texture::~graphic_texture() = default;
}
