#include "python_grid_slot_window.h"
#include <metin2/core/crc32.h>
#include "python_graphic.h"
#include "python_window_manager.h"

namespace metin2::inline ui {

    using namespace widgets;

    void grid_slot_window::on_render_picking_slot() {
        if (!widgets::get_window_manager().is_attaching())
            return;

        uint8_t width_value, height_value;
        widgets::get_window_manager().attaching_icon_size(&width_value, &height_value);

        std::list<slot *> slot_list_value;
        if (picked_slot_list(width_value, height_value, &slot_list_value)) {
            uint32_t slot_number = widgets::get_window_manager().attaching_slot_number();
            uint32_t item_index_value = widgets::get_window_manager().attaching_index();

            if (is_use_mode_) {
                slot *slot_value = *slot_list_value.begin();
                slot *center_slot;
                if (slot_pointer(slot_value->center_slot_number, &center_slot))
                    if (center_slot->is_item) {
                        if (is_usable_item_)
                            get_python_graphic().set_diffuse_color(1.0f, 1.0f, 0.0f, 0.5f);
                        else
                            get_python_graphic().set_diffuse_color(1.0f, 0.0f, 0.0f, 0.5f);

                        get_python_graphic().render_bar2d(rect_.left + center_slot->ix_position,
                            rect_.top + center_slot->iy_position,
                            rect_.left + center_slot->ix_position + center_slot->byx_placed_item_size * ITEM_WIDTH,
                            rect_.top + center_slot->iy_position + center_slot->byy_placed_item_size * ITEM_HEIGHT);
                        return;
                    }
            }

            if (check_moving(slot_number, item_index_value, slot_list_value))
                get_python_graphic().set_diffuse_color(1.0f, 1.0f, 1.0f, 0.5f);
            else
                get_python_graphic().set_diffuse_color(1.0f, 0.0f, 0.0f, 0.5f);

            rectangle rect_value;
            rect_value.left = rect_.right;
            rect_value.top = rect_.bottom;
            rect_value.right = 0;
            rect_value.bottom = 0;

            for (auto itor = slot_list_value.begin(); itor != slot_list_value.end(); ++itor) {
                slot *slot_value = *itor;
                rect_value.left = std::min(rect_value.left, rect_.left + slot_value->ix_position);
                rect_value.top = std::min(rect_value.top, rect_.top + slot_value->iy_position);
                rect_value.right = std::max(rect_value.right, rect_.left + slot_value->ix_position + int32_t(slot_value->byx_placed_item_size * ITEM_WIDTH));
                rect_value.bottom = std::max(rect_value.bottom, rect_.top + slot_value->iy_position + int32_t(slot_value->byy_placed_item_size * ITEM_HEIGHT));
            }

            get_python_graphic().render_bar2d(rect_value.left, rect_value.top, rect_value.right, rect_value.bottom);
        }
    }

    auto grid_slot_window::picked_slot_pointer(slot **pp_slot) -> bool {
        if (!widgets::get_window_manager().is_attaching())
            return slot_window::picked_slot_pointer(pp_slot);

        uint8_t width_value, height;
        widgets::get_window_manager().attaching_icon_size(&width_value, &height);

        std::list<slot *> slot_list;
        if (!picked_slot_list(width_value, height, &slot_list))
            return false;

        slot *min_slot = nullptr;

        for (auto itor = slot_list.begin(); itor != slot_list.end(); ++itor) {
            slot *p_slot = *itor;

            // An item larger than one slot returns its top-left slot.
            if (!min_slot) {
                min_slot = p_slot;
            } else {
                if (p_slot->slot_number < min_slot->slot_number) {
                    min_slot = p_slot;
                } else {
                    if (!min_slot->is_item && p_slot->is_item) {
                        min_slot = p_slot;
                    }
                }
            }
        }

        if (!min_slot) {
            return false;
        } else {
            slot *center_slot;
            if (!slot_pointer(min_slot->center_slot_number, &center_slot))
                return false;

            *pp_slot = center_slot;

            if (widgets::get_window_manager().is_attaching()) {
                uint32_t slot_number = widgets::get_window_manager().attaching_slot_number();

                if (slot_number == center_slot->slot_number) {
                    *pp_slot = min_slot;
                }
            }
        }

        return true;
    }

    auto grid_slot_window::picked_slot_list(int width_value, int height_value, std::list<slot *> *slot_pointer_list) -> bool {
        int32_t lx, ly;
        mouse_local_position(lx, ly);

        if (lx < 0)
            return false;
        if (ly < 0)
            return false;
        if (lx >= width())
            return false;
        if (ly >= height())
            return false;

        slot_pointer_list->clear();

        int ix, iy;
        if (picked_grid_slot_position(lx, ly, &ix, &iy)) {
            int ix_half_step = (width_value / 2);

            int ix_start = int(ix) - int(ix_half_step - (ix_half_step % 2));
            int ix_end = int(ix) + int(ix_half_step);

            // FIXME: find the proper formula.
            int iy_start = 0, iy_end = 0;

            if (1 == height_value) {
                iy_start = iy;
                iy_end = iy;
            } else if (2 == height_value) {
                iy_start = iy;
                iy_end = iy + 1;
            } else if (3 == height_value) {
                iy_start = iy - 1;
                iy_end = iy + 1;
            }

            if (ix_start < 0) {
                ix_end += -ix_start;
                ix_start = 0;
            }

            if (iy_start < 0) {
                iy_end += -iy_start;
                iy_start = 0;
            }

            if (uint32_t(ix_end) >= dwx_count_) {
                int ix_temporary = uint32_t(ix_end) - dwx_count_ + 1;
                ix_start -= ix_temporary;
                ix_end -= ix_temporary;
            }

            if (uint32_t(iy_end) >= dwy_count_) {
                int iy_temporary = uint32_t(iy_end) - dwy_count_ + 1;
                iy_start -= iy_temporary;
                iy_end -= iy_temporary;
            }

            for (int i = ix_start; i <= ix_end; ++i)
                for (int j = iy_start; j <= iy_end; ++j) {
                    slot *slot_value;
                    if (grid_slot_pointer(uint32_t(i), uint32_t(j), &slot_value)) {
                        slot_pointer_list->push_back(slot_value);
                    }
                }

            // Exception for refine scrolls and the like.
            if (is_use_mode_ && 1 == slot_pointer_list->size()) {
                slot *main_slot = *slot_pointer_list->begin();

                auto itor = slot_vector_.begin();
                for (; itor != slot_vector_.end(); ++itor) {
                    slot *slot_value = *itor;
                    if (slot_value->center_slot_number == main_slot->center_slot_number)
                        if (slot_pointer_list->end() == std::find(slot_pointer_list->begin(), slot_pointer_list->end(), slot_value)) {
                            slot_pointer_list->push_back(slot_value);
                        }
                }
            }

            if (!slot_pointer_list->empty())
                return true;
        }

        return false;
    }

    auto grid_slot_window::grid_slot_pointer(int ix, int iy, slot **slot) -> bool {
        uint32_t slot_index = ix + iy * dwx_count_;
        if (slot_index >= slot_vector_.size())
            return false;

        *slot = slot_vector_[slot_index];

        return true;
    }

    auto grid_slot_window::picked_grid_slot_position(int ix_local, int iy_local, int *pix, int *piy) -> bool {
        for (uint32_t x = 0; x < dwx_count_; ++x)
            for (uint32_t y = 0; y < dwy_count_; ++y) {
                slot *slot;
                if (!grid_slot_pointer(x, y, &slot))
                    continue;

                if (ix_local >= slot->ix_position)
                    if (iy_local >= slot->iy_position)
                        if (ix_local <= slot->ix_position + slot->ix_cell_size)
                            if (iy_local <= slot->iy_position + slot->iy_cell_size) {
                                *pix = x;
                                *piy = y;
                                return true;
                            }
            }

        return false;
    }

    void grid_slot_window::arrange_grid_slot(uint32_t start_index, uint32_t dwx_count, uint32_t dwy_count, int ix_slot_size, int iy_slot_size, int ix_temporary_size, int iy_temporary_size) {
        destroy();

        dwx_count_ = dwx_count;
        dwy_count_ = dwy_count;

        slot_vector_.clear();
        slot_vector_.resize(dwx_count * dwy_count);

        for (uint32_t x = 0; x < dwx_count; ++x)
            for (uint32_t y = 0; y < dwy_count; ++y) {
                uint32_t index = start_index + x + y * dwx_count;
                int ix_position = x * (ix_slot_size + ix_temporary_size);
                int iy_position = y * (iy_slot_size + iy_temporary_size);

                append_slot(index, ix_position, iy_position, ix_slot_size, iy_slot_size);

                slot_vector_[x + y * dwx_count] = &(*slot_list_value_.rbegin());
            }

        int width_value = dwx_count * (ix_slot_size + ix_temporary_size);
        int height = dwy_count * (iy_slot_size + iy_temporary_size);
        set_size(width_value, height);
    }

    void grid_slot_window::on_refresh_slot() {
        uint32_t x, y;

        for (x = 0; x < dwx_count_; ++x)
            for (y = 0; y < dwy_count_; ++y) {
                slot *slot_value;
                if (!grid_slot_pointer(x, y, &slot_value))
                    continue;

                slot_value->center_slot_number = slot_value->slot_number;
            }

        for (x = 0; x < dwx_count_; ++x)
            for (y = 0; y < dwy_count_; ++y) {
                slot *slot_value;
                if (!grid_slot_pointer(x, y, &slot_value))
                    continue;

                if (slot_value->is_item) {
                    for (uint32_t x_sub = 0; x_sub < slot_value->byx_placed_item_size; ++x_sub)
                        for (uint32_t y_sub = 0; y_sub < slot_value->byy_placed_item_size; ++y_sub) {
                            slot *sub_slot;
                            if (!grid_slot_pointer(x + x_sub, y + y_sub, &sub_slot))
                                continue;

                            sub_slot->center_slot_number = slot_value->slot_number;
                            sub_slot->item_index = slot_value->item_index;
                        }
                } else {
                    slot_value->item_index = 0;
                }
            }
    }

    auto grid_slot_window::check_moving(uint32_t slot_number, uint32_t item_index, const std::list<slot *> &slot_list) -> bool {
        if (slot_style_value_ != SLOT_STYLE_PICK_UP)
            return true;

        for (auto itor = slot_list.begin(); itor != slot_list.end(); ++itor) {
            slot *slot = *itor;

            if (slot_number != slot->center_slot_number) // Not the slot it was picked from.
            {
                if (0 != slot->item_index || slot->center_slot_number != slot->slot_number) // There is an item.
                {
                    if (item_index != slot->item_index) // A different item blocks the move.
                        return false;
                }
            }
        }

        return true;
    }

    void grid_slot_window::destroy() {
        slot_window::destroy();

        slot_vector_.clear();

        initialize_impl();
    }

    void grid_slot_window::initialize_impl() {
        dwx_count_ = 0;
        dwy_count_ = 0;
    }

    auto grid_slot_window::type() -> uint32_t {
        static int type = get_crc32("CGridSlotWindow", strlen("CGridSlotWindow"));
        return type;
    }

    auto grid_slot_window::on_is_type(uint32_t type) -> bool {
        if (grid_slot_window::type() == type)
            return true;

        return slot_window::on_is_type(type);
    }

    grid_slot_window::grid_slot_window(PyObject *ppy_object) : slot_window(ppy_object) {
    }

    grid_slot_window::~grid_slot_window() = default;
}
