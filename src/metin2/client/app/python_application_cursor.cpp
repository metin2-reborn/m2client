#include <metin2/script/python_utils.h>
#include "python_application.h"
#include "resource_ids.h"

namespace metin2::inline client::inline app {

    auto python_application::create_cursors() -> bool {
        cursor_visible_ = true;
        liar_cursor_on_ = false;

        int resource_id[CURSOR_COUNT] =
            {
                IDC_CURSOR_NORMAL,
                IDC_CURSOR_ATTACK,
                IDC_CURSOR_ATTACK,
                IDC_CURSOR_TALK,
                IDC_CURSOR_NO,
                IDC_CURSOR_PICK,

                IDC_CURSOR_DOOR,
                IDC_CURSOR_CHAIR,
                IDC_CURSOR_CHAIR, // Magic
                IDC_CURSOR_BUY,   // Buy
                IDC_CURSOR_SELL,  // Sell

                IDC_CURSOR_CAMERA_ROTATE, // Camera Rotate
                IDC_CURSOR_HSIZE,         // Horizontal Size
                IDC_CURSOR_VSIZE,         // Vertical Size
                IDC_CURSOR_HVSIZE,        // Horizontal & Vertical Size
        };

        cursors_.clear();

        for (int i = 0; i < CURSOR_COUNT; ++i) {
            std::unique_ptr<window::cursor> cursor = window::cursor::load(resource_id[i]);
            if (!cursor)
                return false;

            cursors_.emplace(i, std::move(cursor));
        }

        return true;
    }

    void python_application::destroy_cursors() {
        window::set_cursor(nullptr);
        cursors_.clear();
    }

    void python_application::set_cursor_visible(bool flag, bool liar_cursor_on) {
        cursor_visible_ = flag;
        liar_cursor_on_ = liar_cursor_on;

        if (CURSOR_MODE_HARDWARE == cursor_mode_value_)
            window::show_cursor(cursor_visible_);
    }

    auto python_application::cursor_visible() -> bool {
        return cursor_visible_;
    }

    auto python_application::liar_cursor_on() -> bool {
        return liar_cursor_on_;
    }

    auto python_application::get_cursor_mode() -> int {
        return cursor_mode_value_;
    }

    auto python_application::is_continuous_change_type_cursor(int cursor_num) -> bool {
        switch (cursor_num) {
        case CURSOR_SHAPE_NORMAL:
        case CURSOR_SHAPE_ATTACK:
        case CURSOR_SHAPE_TARGET:
        case CURSOR_SHAPE_MAGIC:
        case CURSOR_SHAPE_BUY:
        case CURSOR_SHAPE_SELL:
            return true;
            break;
        }

        return false;
    }

    auto python_application::set_cursor_num(int cursor_num) -> bool {
        if (CURSOR_SHAPE_NORMAL == cursor_num) {
            if (!is_continuous_change_type_cursor(cursor_num_)) {
                cursor_num = continuous_cursor_num_;
            }
        } else {
            if (is_continuous_change_type_cursor(cursor_num_)) {
                continuous_cursor_num_ = cursor_num_;
            }
        }

        if (CURSOR_MODE_HARDWARE == cursor_mode_value_) {
            auto itor = cursors_.find(cursor_num);
            if (cursors_.end() == itor)
                return false;

            window::set_cursor(itor->second.get());
        }

        cursor_num_ = cursor_num;

        py_call_class_member_func(mouse_handler_, "ChangeCursor", Py_BuildValue("(i)", cursor_num_));

        return true;
    }

    void python_application::set_cursor_mode(int mode) {
        switch (mode) {
        case CURSOR_MODE_HARDWARE:
            cursor_mode_value_ = CURSOR_MODE_HARDWARE;
            window::show_cursor(true);
            break;

        case CURSOR_MODE_SOFTWARE:
            cursor_mode_value_ = CURSOR_MODE_SOFTWARE;
            window::set_cursor(nullptr);
            window::show_cursor(false);
            break;
        }
    }
}
