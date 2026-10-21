#include "grp_index_buffer.h"
#include <cassert>
#include "state_manager.h"

namespace metin2::inline graphics {

    void graphic_index_buffer::set_indices(int start_index) const {
        assert(device_);
        STATEMANAGER.set_indices(buffer_.get(), start_index);
    }

    auto graphic_index_buffer::lock(void **pret_indices) const -> bool {
        assert(buffer_);

        if (!buffer_->lock(0, 0, pret_indices))
            return false;

        return true;
    }

    void graphic_index_buffer::unlock() const {
        assert(buffer_);

        buffer_->unlock();
    }

    auto graphic_index_buffer::lock(void **pret_indices) -> bool {
        assert(buffer_);

        if (!buffer_->lock(0, 0, pret_indices))
            return false;

        return true;
    }

    void graphic_index_buffer::unlock() {
        assert(buffer_);

        buffer_->unlock();
    }

    auto graphic_index_buffer::copy(int buf_size, const void *src_indices) -> bool {
        assert(buffer_);

        void *dst_indices;
        if (!buffer_->lock(0, 0, &dst_indices))
            return false;

        memcpy(dst_indices, src_indices, buf_size);

        buffer_->unlock();

        return true;
    }

    auto graphic_index_buffer::create(int face_count, face *faces) -> bool {
        int idx_count = face_count * 3;
        iidx_count_ = idx_count;
        if (!create(idx_count, gpu::format::INDEX16))
            return false;

        void *data;
        if (!buffer_->lock(0, 0, &data))
            return false;

        auto *dst_indices = static_cast<uint16_t *>(data);

        for (int i = 0; i < face_count; ++i, dst_indices += 3) {
            face *cur_face = faces + i;
            dst_indices[0] = cur_face->indices[0];
            dst_indices[1] = cur_face->indices[1];
            dst_indices[2] = cur_face->indices[2];
        }

        buffer_->unlock();
        return true;
    }

    auto graphic_index_buffer::create_device_objects() -> bool {
        buffer_ = device_->create_index_buffer(buffer_size_, gpu::usage::WRITE_ONLY, format_);
        return buffer_ != nullptr;
    }

    void graphic_index_buffer::destroy_device_objects() {
        buffer_.reset();
    }

    auto graphic_index_buffer::create(int idx_count, gpu::format fmt) -> bool {
        destroy();

        iidx_count_ = idx_count;
        buffer_size_ = (fmt == gpu::format::INDEX32 ? sizeof(uint32_t) : sizeof(uint16_t)) * idx_count;
        format_ = fmt;

        return create_device_objects();
    }

    void graphic_index_buffer::destroy() {
        destroy_device_objects();
    }

    void graphic_index_buffer::initialize() {
        buffer_.reset();
    }

    graphic_index_buffer::graphic_index_buffer() {
        initialize();
    }

    graphic_index_buffer::~graphic_index_buffer() {
        destroy();
    }
}
