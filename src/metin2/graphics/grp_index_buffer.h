#pragma once

#include "grp_base.h"

namespace metin2::inline graphics {

    class graphic_index_buffer : public graphic_base {
      public:
        graphic_index_buffer();
        ~graphic_index_buffer() override;

        void destroy();
        auto create(int idx_count, gpu::format fmt) -> bool;
        auto create(int face_count, face *faces) -> bool;

        auto create_device_objects() -> bool;
        void destroy_device_objects();

        auto copy(int buf_size, const void *src_indices) -> bool;

        auto lock(void **pret_indices) const -> bool;
        void unlock() const;

        auto lock(void **pret_indices) -> bool;
        void unlock();

        void set_indices(int start_index = 0) const;

        auto gpu_buffer() const -> gpu::index_buffer * { return buffer_.get(); }

        auto index_count() const -> int { return iidx_count_; }

      protected:
        void initialize();

      protected:
        std::shared_ptr<gpu::index_buffer> buffer_;
        uint32_t buffer_size_;
        gpu::format format_;
        int iidx_count_;
    };
}
