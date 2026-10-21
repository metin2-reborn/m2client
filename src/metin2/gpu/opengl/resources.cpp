#include "device.h"

#include <algorithm>
#include <cstring>

namespace metin2::gpu::opengl {

    namespace {
        auto buffer_usage(uint32_t usage) -> GLenum {
            return (usage & usage::DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
        }

        // Buffers keep a CPU copy: locks write into it and unlock uploads the locked range.
        auto lock_range(uint32_t buffer_size, uint32_t &offset, uint32_t &size) -> bool {
            if (offset > buffer_size)
                return false;
            if (size == 0 || offset + size > buffer_size)
                size = buffer_size - offset;
            return true;
        }
    }

    gl_vertex_buffer::gl_vertex_buffer(uint32_t size, uint32_t usage) : vertex_buffer(size, usage), shadow_(size, 0) {
        glGenBuffers(1, &name);
        glBindBuffer(GL_ARRAY_BUFFER, name);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, buffer_usage(usage));
    }

    gl_vertex_buffer::~gl_vertex_buffer() {
        glDeleteBuffers(1, &name);
    }

    auto gl_vertex_buffer::lock(uint32_t offset, uint32_t size, void **data, uint32_t flags) -> bool {
        if (!data || locked_ || !lock_range(size_, offset, size))
            return false;
        locked_ = true;
        lock_offset_ = offset;
        lock_size_ = size;
        read_only_ = (flags & lock::READ_ONLY) != 0;
        *data = shadow_.data() + offset;
        return true;
    }

    void gl_vertex_buffer::unlock() {
        if (!locked_)
            return;
        locked_ = false;
        if (read_only_)
            return;
        glBindBuffer(GL_ARRAY_BUFFER, name);
        if (lock_offset_ == 0 && lock_size_ == size_)
            glBufferData(GL_ARRAY_BUFFER, size_, shadow_.data(), buffer_usage(usage_));
        else
            glBufferSubData(GL_ARRAY_BUFFER, lock_offset_, lock_size_, shadow_.data() + lock_offset_);
    }

    gl_index_buffer::gl_index_buffer(uint32_t size, uint32_t usage, format fmt) : index_buffer(size, usage, fmt), shadow_(size, 0) {
        glGenBuffers(1, &name);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, name);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, buffer_usage(usage));
    }

    gl_index_buffer::~gl_index_buffer() {
        glDeleteBuffers(1, &name);
    }

    auto gl_index_buffer::lock(uint32_t offset, uint32_t size, void **data, uint32_t flags) -> bool {
        if (!data || locked_ || !lock_range(size_, offset, size))
            return false;
        locked_ = true;
        lock_offset_ = offset;
        lock_size_ = size;
        read_only_ = (flags & lock::READ_ONLY) != 0;
        *data = shadow_.data() + offset;
        return true;
    }

    void gl_index_buffer::unlock() {
        if (!locked_)
            return;
        locked_ = false;
        if (read_only_)
            return;
        // The element binding belongs to the vertex array; changing it here is harmless because every
        // indexed draw binds its own.
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, name);
        if (lock_offset_ == 0 && lock_size_ == size_)
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_, shadow_.data(), buffer_usage(usage_));
        else
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, lock_offset_, lock_size_, shadow_.data() + lock_offset_);
    }

    gl_texture::gl_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, format fmt)
        : texture(width, height, levels, usage, fmt), info_(get_format_info(fmt)), gl_(get_gl_format(fmt)) {
        if (levels_ == 0) {
            levels_ = 1;
            for (uint32_t w = width, h = height; w > 1 || h > 1; w = std::max(1u, w / 2), h = std::max(1u, h / 2))
                ++levels_;
        }
        level_data_.resize(levels_);
        glGenTextures(1, &name);
        glBindTexture(GL_TEXTURE_2D, name);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(levels_) - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        if (info_->alpha_one)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        if (fmt == format::A8) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
        }
        // Storage for every level now, so render targets and partial uploads work.
        if (!info_->compressed)
            for (uint32_t i = 0; i < levels_; ++i)
                glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(i), static_cast<GLint>(gl_->internal_format), static_cast<GLsizei>(level_width(i)), static_cast<GLsizei>(level_height(i)), 0, gl_->pixel_format, gl_->pixel_type, nullptr);
    }

    gl_texture::~gl_texture() {
        if (framebuffer_)
            glDeleteFramebuffers(1, &framebuffer_);
        glDeleteTextures(1, &name);
    }

    auto gl_texture::lock(uint32_t level, locked_rect &out, const rectangle *area, uint32_t flags) -> bool {
        if (level >= levels_)
            return false;
        level_data &l = level_data_[level];
        if (l.locked)
            return false;
        const uint32_t w = level_width(level);
        const uint32_t h = level_height(level);
        const uint32_t pitch = format_pitch(*info_, w);
        const size_t size = format_level_size(*info_, w, h);
        const bool read_only = (flags & lock::READ_ONLY) != 0;
        // The CPU copy is made on the first lock; render targets read what the GPU drew.
        if (l.shadow.size() != size) {
            l.shadow.assign(size, 0);
            if (read_only || (usage_ & usage::RENDER_TARGET))
                read_back(level);
        } else if (read_only && (usage_ & usage::RENDER_TARGET)) {
            read_back(level);
        }
        l.locked = true;
        l.read_only = read_only;
        out.pitch = pitch;
        out.bits = l.shadow.data();
        if (area) {
            if (info_->compressed)
                out.bits += static_cast<size_t>(area->top / 4) * pitch + static_cast<size_t>(area->left / 4) * info_->block_bytes;
            else
                out.bits += static_cast<size_t>(area->top) * pitch + static_cast<size_t>(area->left) * info_->bytes_per_pixel;
        }
        return true;
    }

    void gl_texture::unlock(uint32_t level) {
        if (level >= levels_ || !level_data_[level].locked)
            return;
        level_data_[level].locked = false;
        if (!level_data_[level].read_only)
            upload(level, nullptr);
    }

    void gl_texture::upload(uint32_t level, const void *data) {
        if (level >= levels_)
            return;
        level_data &l = level_data_[level];
        const void *source = data ? data : (l.shadow.empty() ? nullptr : l.shadow.data());
        if (!source)
            return;
        const auto w = static_cast<GLsizei>(level_width(level));
        const auto h = static_cast<GLsizei>(level_height(level));
        glBindTexture(GL_TEXTURE_2D, name);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (info_->compressed)
            glCompressedTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), gl_->internal_format, w, h, 0, static_cast<GLsizei>(format_level_size(*info_, level_width(level), level_height(level))), source);
        else
            glTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), 0, 0, w, h, gl_->pixel_format, gl_->pixel_type, source);
    }

    void gl_texture::upload_rgba8(uint32_t level, const uint8_t *rgba) {
        if (level >= levels_ || info_->compressed)
            return;
        level_data &l = level_data_[level];
        const uint32_t w = level_width(level);
        const uint32_t h = level_height(level);
        l.shadow.assign(format_level_size(*info_, w, h), 0);
        convert_from_rgba8(*info_, rgba, w, h, l.shadow.data(), format_pitch(*info_, w));
        upload(level, nullptr);
    }

    void gl_texture::generate_mipmaps() {
        if (levels_ <= 1 || info_->compressed)
            return;
        glBindTexture(GL_TEXTURE_2D, name);
        glGenerateMipmap(GL_TEXTURE_2D);
        for (uint32_t i = 1; i < levels_; ++i)
            level_data_[i].shadow.clear();
    }

    auto gl_texture::framebuffer() -> GLuint {
        if (!framebuffer_) {
            glGenFramebuffers(1, &framebuffer_);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, name, 0);
        }
        level_data_[0].shadow.clear();
        return framebuffer_;
    }

    void gl_texture::read_back(uint32_t level) {
        level_data &l = level_data_[level];
        l.shadow.assign(format_level_size(*info_, level_width(level), level_height(level)), 0);
        glBindTexture(GL_TEXTURE_2D, name);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        if (info_->compressed)
            glGetCompressedTexImage(GL_TEXTURE_2D, static_cast<GLint>(level), l.shadow.data());
        else
            glGetTexImage(GL_TEXTURE_2D, static_cast<GLint>(level), gl_->pixel_format, gl_->pixel_type, l.shadow.data());
    }

    gl_depth_buffer::gl_depth_buffer(uint32_t width, uint32_t height, format fmt) : depth_buffer(width, height, fmt) {
        const gl_format *f = get_gl_format(fmt);
        stencil = f && f->internal_format == GL_DEPTH24_STENCIL8;
        glGenRenderbuffers(1, &name);
        glBindRenderbuffer(GL_RENDERBUFFER, name);
        glRenderbufferStorage(GL_RENDERBUFFER, f ? f->internal_format : GL_DEPTH_COMPONENT16, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    }

    gl_depth_buffer::~gl_depth_buffer() {
        glDeleteRenderbuffers(1, &name);
    }
}
