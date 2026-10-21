#pragma once

#include "grp_base.h"
#include "state_manager.h"

namespace metin2::inline graphics {

    // Owns the gpu device and the objects every frame shares: the state cache, the default index
    // buffers, the small dynamic vertex buffers, the vertex layouts and the debug meshes.
    class graphic_device : public graphic_base {
      public:
        graphic_device();
        ~graphic_device() override;

        void destroy();
        // On the existing window; renderer names a gpu backend (another one is used if it fails).
        auto create(int hres, int vres, const char *renderer) -> bool;

        auto resize_back_buffer(uint32_t width_value, uint32_t height) -> bool;

      protected:
        void initialize_impl();

        void destroy_default_index_buffer_list();
        auto create_default_index_buffer_list() -> bool;
        auto create_default_index_buffer(uint32_t def_ib, uint32_t idx_count, const uint16_t *indices) -> bool;

        void destroy_pdt_vertex_buffer_list();
        auto create_pdt_vertex_buffer_list() -> bool;
    };
}
