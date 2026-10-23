#include "python_slot_window.h"
#include <functional>
#include <metin2/core/crc32.h>
#include <metin2/core/file_name.h>
#include <metin2/core/timer.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/script/python_utils.h>
#include "python_graphic.h"
#include "python_window.h"
#include "python_window_manager.h"

namespace metin2::inline ui {

    using namespace widgets;

    class widgets::slot_window::slot_button : public button {
      public:
        enum slot_button_type {
            SLOT_BUTTON_TYPE_PLUS,
            SLOT_BUTTON_TYPE_COVER,
        };

      public:
        slot_button(slot_button_type type, uint32_t slot_number, slot_window *parent) : button(nullptr) {
            slot_button_type_value_ = type;
            slot_number_ = slot_number;
            parent_ = parent;
        }
        ~slot_button() override = default;

        auto on_mouse_left_button_down() -> bool override {
            if (!is_enable())
                return true;
            if (widgets::get_window_manager().is_attaching())
                return true;

            is_pressed_ = true;
            down();

            return true;
        }
        auto on_mouse_left_button_up() -> bool override {
            if (!is_enable())
                return true;
            if (!is_pressed())
                return true;

            if (is_in())
                set_current_visual(&over_visual_);
            else
                set_current_visual(&up_visual_);

            parent_->on_pressed_slot_button(slot_button_type_value_, slot_number_);

            return true;
        }

        void on_mouse_over_in() override {
            if (is_enable()) {
                set_current_visual(&over_visual_);
            }
            parent_->on_over_in_item(slot_number_);
        }
        void on_mouse_over_out() override {
            if (is_enable()) {
                set_up();
                set_current_visual(&up_visual_);
            }
            parent_->on_over_out_item();
        }

      protected:
        slot_button_type slot_button_type_value_;
        uint32_t slot_number_;
        slot_window *parent_;
    };

    class widgets::slot_window::cover_button : public slot_button {
      public:
        cover_button(slot_button_type type, uint32_t slot_number, slot_window *parent) : slot_button(type, slot_number, parent) {
            left_button_enable_ = true;
            right_button_enable_ = true;
        }
        ~cover_button() override = default;

        void set_left_button_enable(bool enable) {
            left_button_enable_ = enable;
        }
        void set_right_button_enable(bool enable) {
            right_button_enable_ = enable;
        }

        void on_render() override {
        }

        void render_button() {
            button::on_render();
        }

        auto on_mouse_left_button_down() -> bool override {
            if (!is_enable())
                return true;
            if (left_button_enable_) {
                button::on_mouse_left_button_down();
            } else {
                parent_->on_mouse_left_button_down();
            }
            return true;
        }
        auto on_mouse_left_button_up() -> bool override {
            if (!is_enable())
                return true;
            if (left_button_enable_) {
                button::on_mouse_left_button_up();
                parent_->on_mouse_left_button_down();
            } else {
                parent_->on_mouse_left_button_up();
            }
            return true;
        }

        auto on_mouse_right_button_down() -> bool override {
            if (!is_enable())
                return true;
            if (right_button_enable_)
                button::on_mouse_left_button_down();
            return true;
        }
        auto on_mouse_right_button_up() -> bool override {
            if (!is_enable())
                return true;
            parent_->on_mouse_right_button_down();
            if (right_button_enable_)
                button::on_mouse_left_button_up();
            return true;
        }

      protected:
        bool left_button_enable_;
        bool right_button_enable_;
    };

    class widgets::slot_window::cool_time_finish_effect : public ani_image_box {
      public:
        cool_time_finish_effect(slot_window *parent, uint32_t slot_index) : ani_image_box(nullptr) {
            parent_ = parent;
            slot_index_ = slot_index;
        }
        ~cool_time_finish_effect() override = default;

        void on_end_frame() override {
            ((slot_window *)parent_)->reserve_destroy_cool_time_finish_effect(slot_index_);
        }

      protected:
        uint32_t slot_index_;
    };

    void slot_window::set_slot_style(uint32_t style) {
        slot_style_value_ = style;
    }

    void slot_window::append_slot(uint32_t index, int ix_position, int iy_position, int ix_cell_size, int iy_cell_size) {
        slot slot_value;
        slot_value.instance = nullptr;
        slot_value.number_line_value = nullptr;
        slot_value.cover_button_value = nullptr;
        slot_value.slot_button_value = nullptr;
        slot_value.sign_image = nullptr;
        slot_value.finish_cool_time_effect = nullptr;

        clear_slot(&slot_value);
        slot_value.slot_number = index;
        slot_value.center_slot_number = index;
        slot_value.ix_position = ix_position;
        slot_value.iy_position = iy_position;
        slot_value.ix_cell_size = ix_cell_size;
        slot_value.iy_cell_size = iy_cell_size;
        slot_list_value_.push_back(slot_value);
    }

    void slot_window::set_cover_button(uint32_t index, const char *up_image_name, const char *over_image_name, const char *down_image_name, const char *disable_image_name, bool left_button_enable, bool right_button_enable) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        cover_button *&cover_button_value = slot_value->cover_button_value;

        if (!cover_button_value) {
            cover_button_value = new cover_button(slot_button::SLOT_BUTTON_TYPE_COVER, slot_value->slot_number, this);
            get_window_manager().set_parent(cover_button_value, this);
        }

        cover_button_value->set_left_button_enable(left_button_enable);
        cover_button_value->set_right_button_enable(right_button_enable);
        cover_button_value->set_up_visual(up_image_name);
        cover_button_value->set_over_visual(over_image_name);
        cover_button_value->set_down_visual(down_image_name);
        cover_button_value->set_disable_visual(disable_image_name);
        cover_button_value->enable();
        cover_button_value->show();

        // Temporary: the cover button hides the plus button.
        if (slot_value->slot_button_value) {
            set_top(slot_value->slot_button_value);
        }
    }

    void slot_window::enable_cover_button(uint32_t index) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        if (!slot_value->cover_button_value)
            return;

        slot_value->cover_button_value->enable();
    }

    void slot_window::disable_cover_button(uint32_t index) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        if (!slot_value->cover_button_value)
            return;

        slot_value->cover_button_value->disable();
    }

    void slot_window::set_always_render_cover_button(uint32_t index, bool always_render) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        if (always_render)
            SET_BIT(slot_value->state_value, SLOT_STATE_ALWAYS_RENDER_COVER);
        else
            REMOVE_BIT(slot_value->state_value, SLOT_STATE_ALWAYS_RENDER_COVER);
    }

    void slot_window::show_slot_base_image(uint32_t index) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        slot_value->render_base_slot_image = true;
    }

    void slot_window::hide_slot_base_image(uint32_t index) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        slot_value->render_base_slot_image = false;
    }

    void slot_window::set_slot_base_image(const char *file_name, float fr, float fg, float fb, float fa) {
        create_base_image(file_name, fr, fg, fb, fa);
    }

    void slot_window::append_slot_button(const char *up_image_name, const char *over_image_name, const char *down_image_name) {
        for (auto itor = slot_list_value_.begin(); itor != slot_list_value_.end(); ++itor) {
            slot &slot_value = *itor;
            slot_button *&slot_button_value = slot_value.slot_button_value;

            if (!slot_button_value) {
                slot_button_value = new slot_button(slot_button::SLOT_BUTTON_TYPE_PLUS, slot_value.slot_number, this);
                slot_button_value->add_flag(FLAG_FLOAT);
                get_window_manager().set_parent(slot_button_value, this);
            }

            slot_button_value->set_up_visual(up_image_name);
            slot_button_value->set_over_visual(over_image_name);
            slot_button_value->set_down_visual(down_image_name);
            slot_button_value->set_position(slot_value.ix_position + 1, slot_value.iy_position + 19);
            slot_button_value->hide();
        }
    }

    void slot_window::append_requirement_sign_image(const char *image_name) {
        for (auto itor = slot_list_value_.begin(); itor != slot_list_value_.end(); ++itor) {
            slot &slot_value = *itor;
            image_box *&sign_image = slot_value.sign_image;

            if (!sign_image) {
                sign_image = new image_box(nullptr);
                get_window_manager().set_parent(sign_image, this);
            }

            sign_image->load_image(image_name);
            sign_image->hide();
        }
    }

    auto slot_window::has_slot(uint32_t index) -> bool {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return false;

        return true;
    }

    void slot_window::set_slot(uint32_t index, uint32_t virtual_number, uint8_t width_value, uint8_t height, graphic_image *image, math::color &diffuse_color) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        if (slot_value->is_item)
            if (slot_value->item_index == virtual_number) {
                slot_value->state_value = 0;
                slot_value->is_item = true;
                if (image && slot_value->instance) {
                    slot_value->instance->set_image_pointer(image);
                }
                return;
            }

        clear_slot(slot_value);
        slot_value->state_value = 0;
        slot_value->is_item = true;
        slot_value->item_index = virtual_number;

        if (image) {
            assert(nullptr == slot_value->instance);
            slot_value->instance = graphic_image_instance::new_value();
            slot_value->instance->set_diffuse_color(diffuse_color.r, diffuse_color.g, diffuse_color.b, diffuse_color.a);
            slot_value->instance->set_image_pointer(image);
        }

        slot_value->byx_placed_item_size = width_value;
        slot_value->byy_placed_item_size = height;

        if (slot_value->cover_button_value) {
            slot_value->cover_button_value->show();
        }
    }

    void slot_window::set_slot_count(uint32_t index, uint32_t dw_count) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        if (dw_count <= 0) {
            if (slot_value->number_line_value) {
                delete slot_value->number_line_value;
                slot_value->number_line_value = nullptr;
            }
        } else {
            char sz_count[16 + 1];
            snprintf(sz_count, sizeof(sz_count), "%d", dw_count);

            if (!slot_value->number_line_value) {
                auto *number_line = new widgets::number_line(this);
                number_line->set_horizontal_align(number_line::HORIZONTAL_ALIGN_RIGHT);
                number_line->show();
                slot_value->number_line_value = number_line;
            }

            slot_value->number_line_value->set_number(sz_count);
        }
    }

    void slot_window::set_slot_count_new(uint32_t index, uint32_t grade, uint32_t dw_count) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        if (dw_count <= 0) {
            if (slot_value->number_line_value) {
                delete slot_value->number_line_value;
                slot_value->number_line_value = nullptr;
            }
        } else {
            char sz_count[16 + 1];

            switch (grade) {
            case 0:
                snprintf(sz_count, sizeof(sz_count), "%d", dw_count);
                break;
            case 1:
                snprintf(sz_count, sizeof(sz_count), "m%d", dw_count);
                break;
            case 2:
                snprintf(sz_count, sizeof(sz_count), "g%d", dw_count);
                break;
            case 3:
                snprintf(sz_count, sizeof(sz_count), "p");
                break;
            }

            if (!slot_value->number_line_value) {
                auto *number_line = new widgets::number_line(this);
                number_line->set_horizontal_align(number_line::HORIZONTAL_ALIGN_RIGHT);
                number_line->show();
                slot_value->number_line_value = number_line;
            }

            slot_value->number_line_value->set_number(sz_count);
        }
    }

    void slot_window::set_slot_cool_time(uint32_t index, float cool_time, float elapsed_time) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        slot_value->cool_time = cool_time;
        slot_value->start_cool_time = get_timer().current_second() - elapsed_time;
    }

    void slot_window::activate_slot(uint32_t index) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        slot_value->active = true;

        if (!slot_active_effect_) {
            create_slot_enable_effect();
        }
    }

    void slot_window::deactivate_slot(uint32_t index) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        slot_value->active = false;
    }

    void slot_window::clear_slot(uint32_t index) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        clear_slot(slot_value);
    }

    void slot_window::clear_slot(slot *slot_value) {
        slot_value->active = false;
        slot_value->byx_placed_item_size = 1;
        slot_value->byy_placed_item_size = 1;

        slot_value->is_item = false;
        slot_value->state_value = 0;
        slot_value->cool_time = 0.0f;
        slot_value->start_cool_time = 0.0f;
        slot_value->center_slot_number = 0xffffffff;

        slot_value->item_index = 0;
        slot_value->render_base_slot_image = true;

        if (slot_value->instance) {
            graphic_image_instance::delete_value(slot_value->instance);
            slot_value->instance = nullptr;
        }
        if (slot_value->cover_button_value) {
            slot_value->cover_button_value->hide();
        }
        if (slot_value->slot_button_value) {
            slot_value->slot_button_value->hide();
        }
        if (slot_value->sign_image) {
            slot_value->sign_image->hide();
        }
        if (slot_value->finish_cool_time_effect) {
            slot_value->finish_cool_time_effect->hide();
        }
    }

    void slot_window::clear_all_slot() {
        destroy();
    }

    void slot_window::refresh_slot() {
        on_refresh_slot();

        // A refresh also updates the tooltip.
        if (is_rendering()) {
            slot *slot_value;
            if (picked_slot_pointer(&slot_value)) {
                on_over_out_item();
                on_over_in_item(slot_value->slot_number);
            }
        }
    }

    void slot_window::on_refresh_slot() {
    }

    auto slot_window::slot_count() -> uint32_t {
        return slot_list_value_.size();
    }

    void slot_window::lock_slot(uint32_t index) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        slot_value->state_value |= SLOT_STATE_LOCK;
    }
    void slot_window::unlock_slot(uint32_t index) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        slot_value->state_value ^= SLOT_STATE_LOCK;
    }
    void slot_window::enable_slot(uint32_t index) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;

        REMOVE_BIT(slot_value->state_value, SLOT_STATE_DISABLE);
    }
    void slot_window::disable_slot(uint32_t index) {
        slot *slot_value;
        if (!slot_pointer(index, &slot_value))
            return;
        SET_BIT(slot_value->state_value, SLOT_STATE_DISABLE);
    }

    void slot_window::show_slot_button(uint32_t slot_number) {
        slot *slot_value;
        if (!slot_pointer(slot_number, &slot_value))
            return;

        if (slot_value->slot_button_value) {
            slot_value->slot_button_value->show();
        }
    }

    void slot_window::hide_all_slot_button() {
        for (auto itor = slot_list_value_.begin(); itor != slot_list_value_.end(); ++itor) {
            slot &slot_value = *itor;

            if (slot_value.slot_button_value) {
                slot_value.slot_button_value->hide();
            }
        }
    }

    void slot_window::show_requirement_sign(uint32_t slot_number) {
        slot *slot_value;
        if (!slot_pointer(slot_number, &slot_value))
            return;

        if (!slot_value->sign_image)
            return;

        slot_value->sign_image->show();
    }

    void slot_window::hide_requirement_sign(uint32_t slot_number) {
        slot *slot_value;
        if (!slot_pointer(slot_number, &slot_value))
            return;

        if (!slot_value->sign_image)
            return;

        slot_value->sign_image->hide();
    }

    auto slot_window::on_mouse_left_button_down() -> bool {
        slot *slot_value;
        if (!picked_slot_pointer(&slot_value)) {
            widgets::get_window_manager().deattach_icon();
            return true;
        }

        if (slot_value->is_item && !(slot_value->state_value & SLOT_STATE_LOCK)) {
            on_select_item_slot(slot_value->slot_number);
        } else {
            on_select_empty_slot(slot_value->slot_number);
        }

        return true;
    }

    auto slot_window::on_mouse_left_button_up() -> bool {
        if (widgets::get_window_manager().is_attaching())
            if (widgets::get_window_manager().is_dragging())
                if (is_in()) {
                    widgets::ui_window *win = widgets::get_window_manager().point_window();

                    if (win)
                        if (win->is_type(widgets::slot_window::type())) {
                            auto *slot_win = (widgets::slot_window *)win;

                            slot *slot_value;
                            if (!slot_win->picked_slot_pointer(&slot_value)) {
                                widgets::get_window_manager().deattach_icon();
                                return true;
                            }

                            if (slot_value->is_item)
                                slot_win->on_select_item_slot(slot_value->slot_number);
                            else
                                slot_win->on_select_empty_slot(slot_value->slot_number);

                            return true;
                        }

                    return false;
                }

        return false;
    }

    auto slot_window::on_mouse_right_button_down() -> bool {
        slot *slot_value;
        if (!picked_slot_pointer(&slot_value))
            return true;

        if (slot_value->is_item) {
            on_unselect_item_slot(slot_value->slot_number);
        } else {
            on_unselect_empty_slot(slot_value->slot_number);
        }

        return true;
    }

    auto slot_window::on_mouse_left_button_double_click() -> bool {
        on_use_slot();

        return true;
    }

    void slot_window::on_mouse_over_out() {
        on_over_out_item();
    }

    void slot_window::on_mouse_over() {
        // FIXME: a set_top while the window is being dragged releases the capture; this works
        // around it, a real fix is still needed.

        ui_window *point_window = widgets::get_window_manager().point_window();
        if (this == point_window) {
            slot *slot_value;
            if (picked_slot_pointer(&slot_value)) {
                if (on_over_in_item(slot_value->slot_number))
                    return;
            }
        }

        on_over_out_item();
    }

    void slot_window::on_select_empty_slot(int slot_number) {
        py_call_class_member_func(handler_, "on_select_empty_slot", Py_BuildValue("(i)", slot_number));
    }
    void slot_window::on_select_item_slot(int slot_number) {
        py_call_class_member_func(handler_, "on_select_item_slot", Py_BuildValue("(i)", slot_number));

        if (widgets::get_window_manager().is_attaching())
            on_over_out_item();
    }
    void slot_window::on_unselect_empty_slot(int slot_number) {
        py_call_class_member_func(handler_, "on_unselect_empty_slot", Py_BuildValue("(i)", slot_number));
    }
    void slot_window::on_unselect_item_slot(int slot_number) {
        py_call_class_member_func(handler_, "on_unselect_item_slot", Py_BuildValue("(i)", slot_number));
    }
    void slot_window::on_use_slot() {
        slot *slot_value;
        if (picked_slot_pointer(&slot_value))
            if (slot_value->is_item) {
                py_call_class_member_func(handler_, "on_use_slot", Py_BuildValue("(i)", slot_value->slot_number));
            }
    }

    auto slot_window::on_over_in_item(uint32_t slot_number) -> bool {
        slot *slot_value;
        if (!slot_pointer(slot_number, &slot_value))
            return false;

        if (!slot_value->is_item)
            return false;

        if (slot_value->slot_number == tool_tip_slot_number_)
            return true;

        tool_tip_slot_number_ = slot_number;
        py_call_class_member_func(handler_, "on_over_in_item", Py_BuildValue("(i)", slot_number));

        return true;
    }

    void slot_window::on_over_out_item() {
        if (SLOT_NUMBER_NONE == tool_tip_slot_number_)
            return;

        tool_tip_slot_number_ = SLOT_NUMBER_NONE;
        py_call_class_member_func(handler_, "on_over_out_item", Py_BuildValue("()"));
    }

    void slot_window::on_pressed_slot_button(uint32_t type, uint32_t slot_number, bool is_left) {
        if (slot_button::SLOT_BUTTON_TYPE_PLUS == type) {
            py_call_class_member_func(handler_, "on_pressed_slot_button", Py_BuildValue("(i)", slot_number));
        } else if (slot_button::SLOT_BUTTON_TYPE_COVER == type) {
            if (is_left) {
                on_mouse_left_button_down();
            }
        }
    }

    void slot_window::on_update() {
        for (auto itor = reserve_destroy_effect_deque_.begin(); itor != reserve_destroy_effect_deque_.end(); ++itor) {
            uint32_t slot_index = *itor;

            slot *slot_value;
            if (!slot_pointer(slot_index, &slot_value))
                continue;

            destroy_finish_cool_time_effect(slot_value);
        }
        reserve_destroy_effect_deque_.clear();

        if (slot_active_effect_)
            slot_active_effect_->update();
    }

    void slot_window::on_render() {
        render_slot_base_image();

        switch (slot_style_value_) {
        case SLOT_STYLE_PICK_UP:
            on_render_picking_slot();
            break;
        }

        std::for_each(child_list_.begin(), child_list_.end(), std::mem_fn(&ui_window::on_render));

        slot_list_iterator itor;

        for (itor = slot_list_value_.begin(); itor != slot_list_value_.end(); ++itor) {
            slot &slot_value = *itor;

            if (slot_value.slot_button_value) {
                slot_value.slot_button_value->set_position(slot_value.ix_position + 1, slot_value.iy_position + 19);
            }
            if (slot_value.sign_image) {
                slot_value.sign_image->set_position(slot_value.ix_position - 7, slot_value.iy_position + 10);
            }

            if (slot_value.instance) {
                slot_value.instance->set_position(rect_.left + slot_value.ix_position, rect_.top + slot_value.iy_position);
                slot_value.instance->render();
            }

            if (!slot_value.is_item) {
                if (IS_SET(slot_value.state_value, SLOT_STATE_ALWAYS_RENDER_COVER)) {
                    slot_value.cover_button_value->show();
                    slot_value.cover_button_value->set_position(slot_value.ix_position, slot_value.iy_position);
                    slot_value.cover_button_value->render_button();
                }

                continue;
            }

            if (IS_SET(slot_value.state_value, SLOT_STATE_DISABLE)) {
                get_python_graphic().set_diffuse_color(1.0f, 0.0f, 0.0f, 0.3f);
                get_python_graphic().render_bar2d(rect_.left + slot_value.ix_position,
                    rect_.top + slot_value.iy_position,
                    rect_.left + slot_value.ix_position + slot_value.ix_cell_size,
                    rect_.top + slot_value.iy_position + slot_value.iy_cell_size);
            }

            if (slot_value.cool_time != 0.0f) {
                float fcur_time = get_timer().current_second();
                float percentage = (fcur_time - slot_value.start_cool_time) / slot_value.cool_time;
                get_python_graphic().render_cool_time_box(rect_.left + slot_value.ix_position + 16.0f, rect_.top + slot_value.iy_position + 16.0f, 16.0f, percentage);

                if (fcur_time - slot_value.start_cool_time >= slot_value.cool_time) {
                    // Within one second after the cool time ended.
                    if ((fcur_time - slot_value.start_cool_time) - slot_value.cool_time < 1.0f)
                        create_finish_cool_time_effect(&slot_value);

                    slot_value.cool_time = 0.0f;
                    slot_value.start_cool_time = 0.0f;
                }
            }

            if (slot_value.cover_button_value) {
                slot_value.cover_button_value->set_position(slot_value.ix_position, slot_value.iy_position);
                slot_value.cover_button_value->render_button();
            }

            if (slot_value.number_line_value) {
                int ix = slot_value.byx_placed_item_size * ITEM_WIDTH + slot_value.ix_position - 4;
                int iy = slot_value.iy_position + slot_value.byy_placed_item_size * ITEM_HEIGHT - 12 + 2;
                slot_value.number_line_value->set_position(ix, iy);
                slot_value.number_line_value->update();
                slot_value.number_line_value->render();
            }

            if (slot_value.finish_cool_time_effect) {
                slot_value.finish_cool_time_effect->set_position(slot_value.ix_position, slot_value.iy_position);
                slot_value.finish_cool_time_effect->update();
                slot_value.finish_cool_time_effect->render();
            }

            if (slot_value.active)
                if (slot_active_effect_) {
                    int ix = rect_.left + slot_value.ix_position;
                    int iy = rect_.top + slot_value.iy_position;
                    slot_active_effect_->set_position(ix, iy);
                    slot_active_effect_->render();
                }
        }

        render_locked_slot();
    }

    void slot_window::render_slot_base_image() {
        if (!base_image_instance_)
            return;

        for (auto itor = slot_list_value_.begin(); itor != slot_list_value_.end(); ++itor) {
            slot &slot_value = *itor;

            if (!slot_value.render_base_slot_image)
                continue;

            base_image_instance_->set_position(rect_.left + slot_value.ix_position, rect_.top + slot_value.iy_position);
            base_image_instance_->render();
        }
    }

    void slot_window::on_render_picking_slot() {
        if (!widgets::get_window_manager().is_attaching())
            return;

        slot *slot_value;
        if (!selected_slot_pointer(&slot_value))
            return;

        get_python_graphic().set_diffuse_color(1.0f, 1.0f, 1.0f, 0.5f);
        get_python_graphic().render_bar2d(rect_.left + slot_value->ix_position,
            rect_.top + slot_value->iy_position,
            rect_.left + slot_value->ix_position + slot_value->ix_cell_size,
            rect_.top + slot_value->iy_position + slot_value->iy_cell_size);
    }

    void slot_window::render_locked_slot() {
        get_python_graphic().set_diffuse_color(0.0f, 0.0f, 0.0f, 0.5f);
        for (auto itor = slot_list_value_.begin(); itor != slot_list_value_.end(); ++itor) {
            slot &slot_value = *itor;

            if (!slot_value.is_item)
                continue;

            if (slot_value.state_value & SLOT_STATE_LOCK) {
                get_python_graphic().render_bar2d(rect_.left + slot_value.ix_position,
                    rect_.top + slot_value.iy_position,
                    rect_.left + slot_value.ix_position + slot_value.ix_cell_size,
                    rect_.top + slot_value.iy_position + slot_value.iy_cell_size);
            }
        }
    }

    auto slot_window::slot_pointer(uint32_t index, slot **pp_slot) -> bool {
        for (auto itor = slot_list_value_.begin(); itor != slot_list_value_.end(); ++itor) {
            slot &r_slot = *itor;

            if (index == r_slot.slot_number) {
                *pp_slot = &r_slot;
                return true;
            }
        }

        return false;
    }

    auto slot_window::selected_slot_pointer(slot **pp_slot) -> bool {
        int32_t lx, ly;
        mouse_local_position(lx, ly);

        for (auto itor = slot_list_value_.begin(); itor != slot_list_value_.end(); ++itor) {
            slot &r_slot = *itor;

            if (lx >= r_slot.ix_position)
                if (ly >= r_slot.iy_position)
                    if (lx <= r_slot.ix_position + r_slot.ix_cell_size)
                        if (ly <= r_slot.iy_position + r_slot.iy_cell_size) {
                            *pp_slot = &r_slot;
                            return true;
                        }
        }

        return false;
    }

    auto slot_window::picked_slot_pointer(slot **pp_slot) -> bool {
        int32_t lx, ly;
        get_window_manager().mouse_position(lx, ly);

        int ix_local = lx - rect_.left;
        int iy_local = ly - rect_.top;

        // The top-left slot is the reference, so searching from the top left to the bottom
        // right skips the covered slots automatically.
        for (auto itor = slot_list_value_.begin(); itor != slot_list_value_.end(); ++itor) {
            slot &r_slot = *itor;

            int ix_cell_size = r_slot.ix_cell_size;
            int iy_cell_size = r_slot.iy_cell_size;

            // A placed item may be larger than the cell.
            if (r_slot.is_item) {
                ix_cell_size = std::max(r_slot.ix_cell_size, int(r_slot.byx_placed_item_size * ITEM_WIDTH));
                iy_cell_size = std::max(r_slot.iy_cell_size, int(r_slot.byy_placed_item_size * ITEM_HEIGHT));
            }

            if (ix_local >= r_slot.ix_position)
                if (iy_local >= r_slot.iy_position)
                    if (ix_local <= r_slot.ix_position + ix_cell_size)
                        if (iy_local <= r_slot.iy_position + iy_cell_size) {
                            *pp_slot = &r_slot;
                            return true;
                        }
        }

        return false;
    }

    void slot_window::set_use_mode(bool flag) {
        is_use_mode_ = flag;
    }

    void slot_window::set_usable_item(bool flag) {
        is_usable_item_ = flag;
    }

    void slot_window::reserve_destroy_cool_time_finish_effect(uint32_t slot_index) {
        reserve_destroy_effect_deque_.push_back(slot_index);
    }

    auto slot_window::type() -> uint32_t {
        static int type = get_crc32("CSlotWindow", strlen("CSlotWindow"));
        return type;
    }

    auto slot_window::on_is_type(uint32_t type) -> bool {
        if (slot_window::type() == type)
            return true;

        return ui_window::on_is_type(type);
    }

    void slot_window::create_slot_enable_effect() {
        destroy_slot_enable_effect();

        slot_active_effect_ = new ani_image_box(nullptr);
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/00.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/01.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/02.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/03.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/04.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/05.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/06.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/07.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/08.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/09.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/10.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/11.sub");
        slot_active_effect_->append_image("d:/ymir work/ui/public/slotactiveeffect/12.sub");
        slot_active_effect_->set_rendering_mode(graphic_expanded_image_instance::RENDERING_MODE_SCREEN);
        slot_active_effect_->show();
    }

    void slot_window::create_finish_cool_time_effect(slot *slot_value) {
        destroy_finish_cool_time_effect(slot_value);

        ani_image_box *finish_cool_time_effect = new cool_time_finish_effect(this, slot_value->slot_number);
        finish_cool_time_effect->append_image("d:/ymir work/ui/public/slotfinishcooltimeeffect/00.sub");
        finish_cool_time_effect->append_image("d:/ymir work/ui/public/slotfinishcooltimeeffect/01.sub");
        finish_cool_time_effect->append_image("d:/ymir work/ui/public/slotfinishcooltimeeffect/02.sub");
        finish_cool_time_effect->append_image("d:/ymir work/ui/public/slotfinishcooltimeeffect/03.sub");
        finish_cool_time_effect->append_image("d:/ymir work/ui/public/slotfinishcooltimeeffect/04.sub");
        finish_cool_time_effect->append_image("d:/ymir work/ui/public/slotfinishcooltimeeffect/05.sub");
        finish_cool_time_effect->append_image("d:/ymir work/ui/public/slotfinishcooltimeeffect/06.sub");
        finish_cool_time_effect->append_image("d:/ymir work/ui/public/slotfinishcooltimeeffect/07.sub");
        finish_cool_time_effect->append_image("d:/ymir work/ui/public/slotfinishcooltimeeffect/08.sub");
        finish_cool_time_effect->append_image("d:/ymir work/ui/public/slotfinishcooltimeeffect/09.sub");
        finish_cool_time_effect->append_image("d:/ymir work/ui/public/slotfinishcooltimeeffect/10.sub");
        finish_cool_time_effect->set_rendering_mode(graphic_expanded_image_instance::RENDERING_MODE_SCREEN);
        finish_cool_time_effect->reset_frame();
        finish_cool_time_effect->set_delay(2);
        finish_cool_time_effect->show();

        slot_value->finish_cool_time_effect = finish_cool_time_effect;
    }

    void slot_window::create_base_image(const char *file_name, float fr, float fg, float fb, float fa) {
        destroy_base_image();

        auto *image = (graphic_image *)get_resource_manager().resource_pointer(file_name);
        base_image_instance_ = graphic_image_instance::new_value();
        base_image_instance_->set_image_pointer(image);
        base_image_instance_->set_diffuse_color(fr, fg, fb, fa);
    }

    void slot_window::destroy_slot_enable_effect() {
        if (slot_active_effect_) {
            delete slot_active_effect_;
            slot_active_effect_ = nullptr;
        }
    }

    void slot_window::destroy_finish_cool_time_effect(slot *slot_value) {
        if (slot_value->finish_cool_time_effect) {
            delete slot_value->finish_cool_time_effect;
            slot_value->finish_cool_time_effect = nullptr;
        }
    }

    void slot_window::destroy_base_image() {
        if (base_image_instance_) {
            graphic_image_instance::delete_value(base_image_instance_);
            base_image_instance_ = nullptr;
        }
    }

    void slot_window::initialize_impl() {
        slot_style_value_ = SLOT_STYLE_PICK_UP;
        tool_tip_slot_number_ = SLOT_NUMBER_NONE;

        is_use_mode_ = false;
        is_usable_item_ = false;

        slot_active_effect_ = nullptr;
        base_image_instance_ = nullptr;
    }

    void slot_window::destroy() {
        for (auto itor = slot_list_value_.begin(); itor != slot_list_value_.end(); ++itor) {
            slot &slot_value = *itor;

            clear_slot(&slot_value);

            if (slot_value.number_line_value) {
                delete slot_value.number_line_value;
                slot_value.number_line_value = nullptr;
            }
            if (slot_value.cover_button_value) {
                get_window_manager().destroy_window(slot_value.cover_button_value);
            }
            if (slot_value.slot_button_value) {
                get_window_manager().destroy_window(slot_value.slot_button_value);
            }
            if (slot_value.sign_image) {
                get_window_manager().destroy_window(slot_value.sign_image);
            }
            if (slot_value.finish_cool_time_effect) {
                get_window_manager().destroy_window(slot_value.finish_cool_time_effect);
            }
        }

        slot_list_value_.clear();

        destroy_slot_enable_effect();
        destroy_base_image();

        initialize_impl();
    }

    slot_window::slot_window(PyObject *ppy_object) : ui_window(ppy_object) {
        initialize_impl();
    }

    slot_window::~slot_window() {
        destroy();
    }
}
