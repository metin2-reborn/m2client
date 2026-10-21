#include "grp_vertex_buffer.h"
#include <cassert>
#include <metin2/gpu/types.h>
#include "state_manager.h"

namespace metin2::inline graphics {

    auto graphic_vertex_buffer::vertex_stride() const -> int {
        int ret_size = gpu::vertex_size(fvf_);
        return ret_size;
    }

    auto graphic_vertex_buffer::vertex_count() const -> int {
        return vtx_count_;
    }

    void graphic_vertex_buffer::set_stream(int stride, int layer) const {
        assert(device_);
        STATEMANAGER.set_stream_source(layer, buffer_.get(), stride);
    }

    auto graphic_vertex_buffer::lock_range(unsigned count, void **pret_vertices) const -> bool {
        if (!buffer_)
            return false;

        uint32_t lock_size = vertex_stride() * count;
        if (!buffer_->lock(0, lock_size, pret_vertices, lock_flag_))
            return false;

        return true;
    }

    auto graphic_vertex_buffer::lock(void **pret_vertices) const -> bool {
        if (!buffer_)
            return false;

        uint32_t lock_size = vertex_stride() * vertex_count();
        if (!buffer_->lock(0, lock_size, pret_vertices, lock_flag_))
            return false;

        return true;
    }

    auto graphic_vertex_buffer::unlock() const -> bool {
        if (!buffer_)
            return false;

        buffer_->unlock();
        return true;
    }

    auto graphic_vertex_buffer::is_empty() const -> bool {
        if (buffer_)
            return true;
        else
            return false;
    }

    auto graphic_vertex_buffer::lock(void **pret_vertices) -> bool {
        if (!buffer_)
            return false;

        if (!buffer_->lock(0, 0, pret_vertices, lock_flag_))
            return false;

        return true;
    }

    auto graphic_vertex_buffer::unlock() -> bool {
        if (!buffer_)
            return false;

        buffer_->unlock();
        return true;
    }

    auto graphic_vertex_buffer::copy(int buf_size, const void *src_vertices) -> bool {
        void *dst_vertices;

        if (!lock(&dst_vertices))
            return false;

        memcpy(dst_vertices, src_vertices, buf_size);

        unlock();
        return true;
    }

    auto graphic_vertex_buffer::create_device_objects() -> bool {
        assert(device_);
        assert(!buffer_);

        buffer_ = device_->create_vertex_buffer(buffer_size_, usage_);
        return buffer_ != nullptr;
    }

    void graphic_vertex_buffer::destroy_device_objects() {
        buffer_.reset();
    }

    auto graphic_vertex_buffer::create(int vtx_count, uint32_t fvf, uint32_t usage) -> bool {
        assert(device_);
        assert(vtx_count > 0);

        destroy();

        vtx_count_ = vtx_count;
        buffer_size_ = gpu::vertex_size(fvf) * vtx_count_;
        usage_ = usage;
        fvf_ = fvf;

        if (usage == gpu::usage::WRITE_ONLY || usage == gpu::usage::DYNAMIC)
            lock_flag_ = 0;
        else
            lock_flag_ = gpu::lock::READ_ONLY;

        return create_device_objects();
    }

    void graphic_vertex_buffer::destroy() {
        destroy_device_objects();
    }

    void graphic_vertex_buffer::initialize() {
        buffer_.reset();
        vtx_count_ = 0;
        buffer_size_ = 0;
    }

    graphic_vertex_buffer::graphic_vertex_buffer() {
        initialize();
    }

    graphic_vertex_buffer::~graphic_vertex_buffer() {
        destroy();
    }
}
