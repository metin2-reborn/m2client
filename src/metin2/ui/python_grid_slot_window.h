#pragma once

#include "python_slot_window.h"

namespace metin2::inline ui {

    namespace widgets {
        class grid_slot_window : public slot_window {
          public:
            static auto type() -> uint32_t;

          public:
            grid_slot_window(PyObject *ppy_object);
            ~grid_slot_window() override;

            void destroy();

            void arrange_grid_slot(uint32_t start_index, uint32_t dwx_count, uint32_t dwy_count, int ix_slot_size, int iy_slot_size, int ix_temporary_size, int iy_temporary_size);

          protected:
            void initialize_impl();

            auto picked_slot_pointer(slot **slot) -> bool override;
            auto picked_slot_list(int width_value, int height, std::list<slot *> *slot_pointer_list) -> bool;
            auto grid_slot_pointer(int ix, int iy, slot **slot) -> bool;
            auto picked_grid_slot_position(int ix_local, int iy_local, int *pix, int *piy) -> bool;
            auto check_moving(uint32_t slot_number, uint32_t item_index, const std::list<slot *> &slot_list) -> bool;

            auto on_is_type(uint32_t type) -> bool override;

            void on_refresh_slot() override;
            void on_render_picking_slot() override;

          protected:
            uint32_t dwx_count_;
            uint32_t dwy_count_;

            std::vector<slot *> slot_vector_;
        };
    };
}
