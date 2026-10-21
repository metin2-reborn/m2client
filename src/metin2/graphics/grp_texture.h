#pragma once

#include "grp_base.h"

namespace metin2::inline graphics {

    class graphic_texture : public graphic_base {
      public:
        virtual auto is_empty() const -> bool;

        auto width() const -> int;
        auto height() const -> int;

        void set_texture_stage(int stage) const;
        auto gpu_texture() const -> gpu::texture * { return texture_.get(); }
        auto shared_texture() const -> const std::shared_ptr<gpu::texture> & { return texture_; }

        void destroy_device_objects();

      protected:
        graphic_texture();
        ~graphic_texture() override;

        void destroy();
        void initialize();

      protected:
        bool empty_;

        int width_;
        int height_;

        std::shared_ptr<gpu::texture> texture_;
    };
}
