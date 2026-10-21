#pragma once

#include "grp_base.h"

namespace metin2::inline graphics {

    class graphic_vertex_buffer : public graphic_base {
      public:
        graphic_vertex_buffer();
        ~graphic_vertex_buffer() override;

        void destroy();
        virtual auto create(int vtx_count, uint32_t fvf, uint32_t usage) -> bool;

        auto create_device_objects() -> bool;
        void destroy_device_objects();

        auto copy(int buf_size, const void *src_vertices) -> bool;

        auto lock_range(unsigned count, void **pret_vertices) const -> bool;
        auto lock(void **pret_vertices) const -> bool;
        auto unlock() const -> bool;

        virtual auto lock(void **pret_vertices) -> bool;
        auto unlock() -> bool;

        void set_stream(int stride, int layer = 0) const;

        auto vertex_count() const -> int;
        auto vertex_stride() const -> int;

        auto gpu_buffer() const -> gpu::vertex_buffer * { return buffer_.get(); }
        inline auto buffer_size() const -> uint32_t { return buffer_size_; }

        auto is_empty() const -> bool;

      protected:
        void initialize();

      protected:
        std::shared_ptr<gpu::vertex_buffer> buffer_;

        uint32_t buffer_size_;
        uint32_t fvf_;
        uint32_t usage_;
        int vtx_count_;
        uint32_t lock_flag_;
    };
}
