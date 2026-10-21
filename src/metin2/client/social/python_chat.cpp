#include "python_chat.h"
#include <metin2/log/log.h>

#include <metin2/client/app/abstract_application.h>
#include <metin2/client/characters/python_character_manager.h>
#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline social {

    int python_chat::chat_set::chat_mode_size = CHAT_TYPE_MAX_NUM;

    const float c_fStartDisappearingTime = 5.0f;
    const int c_iMaxLineCount = 5;

    dynamic_pool<python_chat::chat_line> python_chat::chat_line::pool_value;

    void python_chat::set_chat_color(uint32_t type, uint32_t r, uint32_t g, uint32_t b) {
        if (type >= CHAT_TYPE_MAX_NUM)
            return;

        uint32_t color = (0xff000000) | (r << 16) | (g << 8) | (b);
        chat_colors_[type] = math::color(color);
    }

    auto python_chat::chat_line::new_value() -> python_chat::chat_line * {
        return pool_value.alloc();
    }

    void python_chat::chat_line::delete_value(python_chat::chat_line *chat_line) {
        chat_line->instance.destroy();
        pool_value.free(chat_line);
    }

    void python_chat::chat_line::destroy_system() {
        pool_value.destroy();
    }

    void python_chat::chat_line::set_color(uint32_t id, uint32_t color) {
        assert(id < CHAT_LINE_COLOR_ARRAY_MAX_NUM);

        if (id >= CHAT_LINE_COLOR_ARRAY_MAX_NUM)
            return;

        color_value[id] = color;
    }

    void python_chat::chat_line::set_color_all(uint32_t color) {
        for (int i = 0; i < CHAT_LINE_COLOR_ARRAY_MAX_NUM; ++i)
            color_value[i] = color;
    }

    auto python_chat::chat_line::color_ref(uint32_t id) -> math::color & {
        assert(id < CHAT_LINE_COLOR_ARRAY_MAX_NUM);

        if (id >= CHAT_LINE_COLOR_ARRAY_MAX_NUM) {
            static math::color color(1.0f, 0.0f, 0.0f, 1.0f);
            return color;
        }

        return color_value[id];
    }

    python_chat::chat_line::chat_line() {
        for (int i = 0; i < CHAT_LINE_COLOR_ARRAY_MAX_NUM; ++i)
            color_value[i] = 0xff0000ff;
    }
    python_chat::chat_line::~chat_line() {
        instance.destroy();
    }

    auto python_chat::create_chat_set(uint32_t id) -> int {
        chat_set_map_value_.insert(std::make_pair(id, chat_set()));
        return id;
    }

    void python_chat::update_view_mode(uint32_t id) {
        abstract_application &app = get_abstract_application();

        float fcur_time = app.global_time();

        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        chat_line_list *line_list = &(chat_set_value->showing_chat_line_list);
        int line_index = line_list->size();
        int height = -(int(line_list->size() + 1) * chat_set_value->step);

        chat_line_list::iterator itor;
        for (itor = line_list->begin(); itor != line_list->end();) {
            chat_line *chat_line_value = (*itor);

            math::color &color = chat_line_value->color_ref(id);

            float elapsed_time = (fcur_time - chat_line_value->appended_time);
            if (elapsed_time >= c_fStartDisappearingTime || line_index >= c_iMaxLineCount) {
                color.a -= color.a / 10.0f;

                if (color.a <= 0.1f) {
                    itor = line_list->erase(itor);
                } else {
                    ++itor;
                }
            } else {
                ++itor;
            }

            height += chat_set_value->step;
            --line_index;

            chat_line_value->instance.set_position(chat_set_value->ix, chat_set_value->iy + height);
            chat_line_value->instance.set_color(color);
            chat_line_value->instance.update();
        }
    }

    void python_chat::update_edit_mode(uint32_t id) {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        const int alpha_line = std::max(0, visible_line_count(id) - editable_line_count(id) + 2);

        int line_index = 0;
        float alpha = 0.0f;
        float alpha_step = 0.0f;

        if (alpha_line > 0)
            alpha_step = 1.0f / float(alpha_line);

        chat_line_list *line_list = &(chat_set_value->showing_chat_line_list);
        int height = -(int(line_list->size() + 1) * chat_set_value->step);

        for (auto itor = line_list->begin(); itor != line_list->end(); ++itor) {
            chat_line *chat_line_value = (*itor);

            math::color &color = chat_line_value->color_ref(id);

            if (line_index < alpha_line) {
                color.a += (alpha - color.a) / 10.0f;
                alpha = f_min(alpha + alpha_step, 1.0f);
            } else {
                color.a = f_min(color.a + 0.05f, 1.0f);
            }

            height += chat_set_value->step;
            chat_line_value->instance.set_position(chat_set_value->ix, chat_set_value->iy + height);
            chat_line_value->instance.set_color(color);
            chat_line_value->instance.update();
        }
    }

    void python_chat::update_log_mode(uint32_t id) {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        chat_line_list *line_list = &(chat_set_value->showing_chat_line_list);
        int height = 0;

        for (auto itor = line_list->rbegin(); itor != line_list->rend(); ++itor) {
            chat_line *chat_line_value = (*itor);

            height -= chat_set_value->step;
            chat_line_value->instance.set_position(chat_set_value->ix, chat_set_value->iy + height);
            chat_line_value->instance.set_color(chat_line_value->color_ref(id));
            chat_line_value->instance.update();
        }
    }

    void python_chat::update(uint32_t id) {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        switch (chat_set_value->board_state_value) {
        case BOARD_STATE_VIEW:
            update_view_mode(id);
            break;
        case BOARD_STATE_EDIT:
            update_edit_mode(id);
            break;
        case BOARD_STATE_LOG:
            update_log_mode(id);
            break;
        }

        uint32_t dwcur_time = get_timer().current_millisecond();
        for (auto itor = wait_chat_list_value_.begin(); itor != wait_chat_list_value_.end();) {
            wait_chat &wait_chat_value = *itor;

            if (wait_chat_value.appending_time < dwcur_time) {
                append_chat(wait_chat_value.type_value, wait_chat_value.chat.c_str());

                itor = wait_chat_list_value_.erase(itor);
            } else {
                ++itor;
            }
        }
    }

    void python_chat::render(uint32_t id) {
        chat_line_list *line_list = chat_line_list_ptr(id);
        if (!line_list)
            return;

        for (auto itor = line_list->begin(); itor != line_list->end(); ++itor) {
            graphic_text_instance &instance = (*itor)->instance;
            instance.render();
        }
    }

    void python_chat::set_board_state(uint32_t id, int state) {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        chat_set_value->board_state_value = state;
        arrange_showing_chat(id);
    }
    void python_chat::set_position(uint32_t id, int ix, int iy) {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        chat_set_value->ix = ix;
        chat_set_value->iy = iy;
    }
    void python_chat::set_height(uint32_t id, int height) {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        chat_set_value->height = height;
    }
    void python_chat::toggle_chat_mode(uint32_t id, int mode) {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        chat_set_value->mode_value[mode] = 1 - chat_set_value->mode_value[mode];
        arrange_showing_chat(id);
    }
    void python_chat::enable_chat_mode(uint32_t id, int mode) {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        chat_set_value->mode_value[mode] = true;
        arrange_showing_chat(id);
    }
    void python_chat::disable_chat_mode(uint32_t id, int mode) {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        chat_set_value->mode_value[mode] = false;
        arrange_showing_chat(id);
    }
    void python_chat::set_end_pos(uint32_t id, float pos) {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        pos = std::max(0.0f, pos);
        pos = std::min(1.0f, pos);
        if (chat_set_value->end_pos != pos) {
            chat_set_value->end_pos = pos;
            arrange_showing_chat(id);
        }
    }

    auto python_chat::visible_line_count(uint32_t id) -> int {
        chat_line_list *line_list = chat_line_list_ptr(id);
        if (!line_list)
            return 0;

        return line_list->size();
    }

    auto python_chat::editable_line_count(uint32_t id) -> int {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return 0;

        return chat_set_value->height / chat_set_value->step + 1;
    }

    auto python_chat::line_count(uint32_t id) -> int {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return 0;

        int count = 0;
        for (uint32_t i = 0; i < chat_line_deque_value_.size(); ++i) {
            if (!chat_set_value->check_mode(chat_line_deque_value_[i]->type_value))
                continue;

            ++count;
        }

        return count;
    }

    auto python_chat::line_step(uint32_t id) -> int {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return 0;

        return chat_set_value->step;
    }

    auto python_chat::chat_line_list_ptr(uint32_t id) -> python_chat::chat_line_list * {
        auto itor = chat_set_map_value_.find(id);
        if (chat_set_map_value_.end() == itor)
            return nullptr;

        chat_set &chat_set_value = itor->second;
        return &(chat_set_value.showing_chat_line_list);
    }

    auto python_chat::chat_set_ptr(uint32_t id) -> python_chat::chat_set * {
        auto itor = chat_set_map_value_.find(id);
        if (chat_set_map_value_.end() == itor)
            return nullptr;

        chat_set &chat_set_value = itor->second;
        return &chat_set_value;
    }

    void python_chat::arrange_showing_chat(uint32_t id) {
        chat_set *chat_set_value = chat_set_ptr(id);
        if (!chat_set_value)
            return;

        chat_set_value->showing_chat_line_list.clear();

        chat_line_deque temp_chat_line_deque;
        for (auto itor = chat_line_deque_value_.begin(); itor != chat_line_deque_value_.end(); ++itor) {
            chat_line *chat_line_value = *itor;
            if (chat_set_value->check_mode(chat_line_value->type_value))
                temp_chat_line_deque.push_back(chat_line_value);
        }

        int icur_line_count = temp_chat_line_deque.size();
        int visible_line_count = std::min(icur_line_count, (chat_set_value->height + chat_set_value->step) / chat_set_value->step);
        int end_line = visible_line_count + int(float(icur_line_count - visible_line_count - 1) * chat_set_value->end_pos);

        int height = 12;
        for (int i = std::min(icur_line_count - 1, end_line); i >= 0; --i) {
            if (!chat_set_value->check_mode(temp_chat_line_deque[i]->type_value))
                continue;

            if (chat_set_value->height + chat_set_value->step <= height) {
                break;
            }

            chat_set_value->showing_chat_line_list.push_front(temp_chat_line_deque[i]);

            height += chat_set_value->step;
        }
    }

    void python_chat::append_chat(int type, const char *chat) {

        auto *default_font_value = static_cast<graphic_text *>(default_font_get_resource());
        if (!default_font_value) {
            metin2::log::error("Cannot find the default font");
            return;
        }

        abstract_application &app = get_abstract_application();
        chat_line *chat_line_value = chat_line::new_value();
        chat_line_value->type_value = type;
        chat_line_value->instance.set_value(chat);

        chat_line_value->instance.set_text_pointer(default_font_value);

        chat_line_value->appended_time = app.global_time();
        chat_line_value->set_color_all(chat_color(type));

        chat_line_deque_value_.push_back(chat_line_value);
        if (chat_line_deque_value_.size() > CHAT_LINE_MAX_NUM) {
            chat_line *chat_line_value = chat_line_deque_value_.front();
            chat_line::delete_value(chat_line_value);
            chat_line_deque_value_.pop_front();
        }

        for (auto itor = chat_set_map_value_.begin(); itor != chat_set_map_value_.end(); ++itor) {
            chat_set *chat_set_value = &(itor->second);

            // A board in edit mode rebuilds its visible lines instead.
            if (BOARD_STATE_EDIT == chat_set_value->board_state_value) {
                arrange_showing_chat(itor->first);
            } else {
                chat_set_value->showing_chat_line_list.push_back(chat_line_value);
                if (chat_set_value->showing_chat_line_list.size() > CHAT_LINE_MAX_NUM) {
                    chat_set_value->showing_chat_line_list.pop_front();
                }
            }
        }
    }

    void python_chat::append_chat_with_delay(int type, const char *chat, int delay) {
        wait_chat wait_chat_value;
        wait_chat_value.type_value = type;
        wait_chat_value.chat = chat;
        wait_chat_value.appending_time = get_timer().current_millisecond() + delay;
        wait_chat_list_value_.push_back(wait_chat_value);
    }

    auto python_chat::chat_color(int type) -> uint32_t {
        if (type < CHAT_TYPE_MAX_NUM) {
            return chat_colors_[type];
        }

        return math::color(0.0f, 0.0f, 1.0f, 1.0f);
    }

    auto python_chat::create_whisper(const char *name) -> whisper * {
        whisper *whisper = whisper::new_value();
        whisper_map_value_.insert(whisper_map::value_type(name, whisper));
        return whisper;
    }

    void python_chat::append_whisper(int type, const char *name, const char *chat) {
        auto itor = whisper_map_value_.find(name);

        whisper *whisper;
        if (itor == whisper_map_value_.end()) {
            whisper = create_whisper(name);
        } else {
            whisper = itor->second;
        }

        whisper->append_chat(type, chat);
    }

    void python_chat::clear_whisper(const char *name) {
        auto itor = whisper_map_value_.find(name);

        if (itor != whisper_map_value_.end()) {
            whisper *whisper = itor->second;
            whisper::delete_value(whisper);

            whisper_map_value_.erase(itor);
        }
    }

    auto python_chat::get_whisper(const char *name, whisper **whisper) -> bool {
        auto itor = whisper_map_value_.find(name);

        if (itor == whisper_map_value_.end())
            return false;

        *whisper = itor->second;

        return true;
    }

    void python_chat::init_whisper(PyObject *ppy_object) {
        auto itor = whisper_map_value_.begin();
        for (; itor != whisper_map_value_.end(); ++itor) {
            std::string name = itor->first;
            py_call_class_member_func(ppy_object, "MakeWhisperButton", Py_BuildValue("(s)", name.c_str()));
        }
    }

    void python_chat::destroy_whisper_map() {
        auto itor = whisper_map_value_.begin();
        for (; itor != whisper_map_value_.end(); ++itor) {
            whisper::delete_value(itor->second);
        }
        whisper_map_value_.clear();
    }

    void python_chat::close() {
        auto itor = chat_set_map_value_.begin();
        for (; itor != chat_set_map_value_.end(); ++itor) {
            chat_set &chat_set_value = itor->second;
            chat_line_list *line_list = &(chat_set_value.showing_chat_line_list);
            for (auto itor = line_list->begin(); itor != line_list->end(); ++itor) {
                chat_line *chat_line_value = (*itor);
                chat_line_value->appended_time = 0.0f;
            }
        }
    }

    void python_chat::destroy() {
        destroy_whisper_map();

        showing_chat_line_list_.clear();
        chat_set_map_value_.clear();
        chat_line_deque_value_.clear();

        chat_line::destroy_system();
        whisper::destroy_system();

        initialize_impl();
    }

    void python_chat::initialize_impl() {
        chat_colors_[CHAT_TYPE_TALKING] = math::color(1.0f, 1.0f, 1.0f, 1.0f);
        chat_colors_[CHAT_TYPE_INFO] = math::color(1.0f, 0.785f, 0.785f, 1.0f);
        chat_colors_[CHAT_TYPE_NOTICE] = math::color(1.0f, 0.902f, 0.730f, 1.0f);
        chat_colors_[CHAT_TYPE_PARTY] = math::color(0.542f, 1.0f, 0.949f, 1.0f);
        chat_colors_[CHAT_TYPE_GUILD] = math::color(0.906f, 0.847f, 1.0f, 1.0f);
        chat_colors_[CHAT_TYPE_COMMAND] = math::color(0.658f, 1.0f, 0.835f, 1.0f);
        chat_colors_[CHAT_TYPE_SHOUT] = math::color(0.658f, 1.0f, 0.835f, 1.0f);
        chat_colors_[CHAT_TYPE_WHISPER] = math::color(0xff4AE14A);
        chat_colors_[CHAT_TYPE_BIG_NOTICE] = math::color(1.0f, 0.902f, 0.730f, 1.0f);
    }

    python_chat::python_chat() {
        initialize_impl();
    }

    python_chat::~python_chat() {
        assert(chat_line_deque_value_.empty());
        assert(showing_chat_line_list_.empty());
        assert(chat_set_map_value_.empty());
        assert(whisper_map_value_.empty());
    }

    dynamic_pool<whisper> whisper::pool_value;

    auto whisper::new_value() -> whisper * {
        return pool_value.alloc();
    }

    void whisper::delete_value(whisper *whisper) {
        whisper->destroy();
        pool_value.free(whisper);
    }

    void whisper::destroy_system() {
        pool_value.destroy();

        chat_line::destroy_system();
    }

    void whisper::set_position(float position) {
        fcur_position_ = position;
        arrange_chat();
    }

    void whisper::set_box_size(float width_value, float height) {
        width_ = width_value;
        height_ = height;

        for (auto itor = chat_line_deque_value_.begin(); itor != chat_line_deque_value_.end(); ++itor) {
            chat_line *chat_line_value = *itor;
            chat_line_value->instance.set_limit_width(width_value);
        }
    }

    void whisper::append_chat(int type, const char *chat) {

        graphic_text *default_font_value = (type == python_chat::WHISPER_TYPE_GM) ? static_cast<graphic_text *>(default_italic_font_get_resource()) : static_cast<graphic_text *>(default_font_get_resource());

        if (!default_font_value) {
            metin2::log::error("Cannot find the default font");
            return;
        }

        chat_line *chat_line_value = chat_line::new_value();
        chat_line_value->instance.set_value(chat);

        chat_line_value->instance.set_text_pointer(default_font_value);

        chat_line_value->instance.set_limit_width(width_);
        chat_line_value->instance.set_multi_line(true);

        switch (type) {
        case python_chat::WHISPER_TYPE_SYSTEM:
            chat_line_value->instance.set_color(math::color(1.0f, 0.785f, 0.785f, 1.0f));
            break;
        case python_chat::WHISPER_TYPE_GM:
            chat_line_value->instance.set_color(math::color(1.0f, 0.632f, 0.0f, 1.0f));
            break;
        case python_chat::WHISPER_TYPE_CHAT:
        default:
            chat_line_value->instance.set_color(0xffffffff);
            break;
        }

        chat_line_deque_value_.push_back(chat_line_value);

        arrange_chat();
    }

    void whisper::arrange_chat() {
        for (auto itor = chat_line_deque_value_.begin(); itor != chat_line_deque_value_.end(); ++itor) {
            chat_line *chat_line_value = *itor;
            chat_line_value->instance.update();
        }
    }

    void whisper::render(float fx, float fy) {
        float height = fy + height_;

        int view_count = int(height_ / line_step_) - 1;
        int i_line_count = int(chat_line_deque_value_.size());
        int start_line = -1;
        if (i_line_count > view_count) {
            start_line = int(float(i_line_count - view_count) * fcur_position_) + view_count - 1;
        } else if (!chat_line_deque_value_.empty()) {
            start_line = i_line_count - 1;
        }

        rectangle rect_value = {static_cast<int32_t>(fx), static_cast<int32_t>(fy), static_cast<int32_t>(fx + width_), static_cast<int32_t>(fy + height_)};

        for (int i = start_line; i >= 0; --i) {
            assert(i >= 0 && i < int(chat_line_deque_value_.size()));
            chat_line *chat_line_value = chat_line_deque_value_[i];

            uint16_t w_line_count = chat_line_value->instance.text_line_count();
            height -= w_line_count * line_step_;

            chat_line_value->instance.set_position(fx, height);
            chat_line_value->instance.render(&rect_value);

            if (height < fy)
                break;
        }
    }

    void whisper::initialize_impl() {
        line_step_ = 15.0f;
        width_ = 300.0f;
        height_ = 120.0f;
        fcur_position_ = 1.0f;
    }

    void whisper::destroy() {
        std::for_each(chat_line_deque_value_.begin(), chat_line_deque_value_.end(), chat_line::delete_value);
        chat_line_deque_value_.clear();
        showing_chat_line_list_.clear();
    }

    whisper::whisper() {
        initialize_impl();
    }

    whisper::~whisper() {
        destroy();
    }

    dynamic_pool<whisper::chat_line> whisper::chat_line::pool_value;

    auto whisper::chat_line::new_value() -> whisper::chat_line * {
        return pool_value.alloc();
    }

    void whisper::chat_line::delete_value(whisper::chat_line *chat_line) {
        chat_line->instance.destroy();
        pool_value.free(chat_line);
    }

    void whisper::chat_line::destroy_system() {
        pool_value.destroy();
    }
}

namespace metin2::inline client::inline app {

    auto get_abstract_chat() -> abstract_chat & {
        return get_python_chat();
    }
}
