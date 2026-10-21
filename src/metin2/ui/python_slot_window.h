#pragma once

#include <deque>
#include "python_window.h"

namespace metin2::inline ui {

    namespace widgets {
        enum {
            ITEM_WIDTH = 32,
            ITEM_HEIGHT = 32,

            SLOT_NUMBER_NONE = 0xffffffff,
        };

        enum slot_style {
            SLOT_STYLE_NONE,
            SLOT_STYLE_PICK_UP,
            SLOT_STYLE_SELECT,
        };

        enum slot_state {
            SLOT_STATE_LOCK = (1 << 0),
            SLOT_STATE_CANT_USE = (1 << 1),
            SLOT_STATE_DISABLE = (1 << 2),
            SLOT_STATE_ALWAYS_RENDER_COVER = (1 << 3), // The cover button normally renders only over a filled slot; with this flag it renders over an empty one too.
        };

        class slot_window : public ui_window {
          public:
            static auto type() -> uint32_t;

          public:
            class slot_button;
            class cover_button;
            class cool_time_finish_effect;

            friend class slot_button;
            friend class cover_button;

            struct slot {
                uint32_t state_value;
                uint32_t slot_number;
                uint32_t center_slot_number; // For a large item, the slot the item actually occupies.
                uint32_t item_index;         // "Item" means whatever the slot holds, not just an inventory item.
                bool is_item;

                float cool_time;
                float start_cool_time;

                bool active;

                int ix_position;
                int iy_position;

                int ix_cell_size;
                int iy_cell_size;

                uint8_t byx_placed_item_size;
                uint8_t byy_placed_item_size;

                graphic_image_instance *instance;
                number_line *number_line_value;

                bool render_base_slot_image;
                cover_button *cover_button_value;
                slot_button *slot_button_value;
                image_box *sign_image;
                ani_image_box *finish_cool_time_effect;
            };
            using slot_list = std::list<slot>;
            using slot_list_iterator = slot_list::iterator;

          public:
            slot_window(PyObject *ppy_object);
            ~slot_window() override;

            void destroy();

            void set_slot_style(uint32_t style);

            void append_slot(uint32_t index, int ix_position, int iy_position, int ix_cell_size, int iy_cell_size);
            void set_cover_button(uint32_t index, const char *up_image_name, const char *over_image_name, const char *down_image_name, const char *disable_image_name, bool left_button_enable, bool right_button_enable);
            void set_slot_base_image(const char *file_name, float fr, float fg, float fb, float fa);
            void append_slot_button(const char *up_image_name, const char *over_image_name, const char *down_image_name);
            void append_requirement_sign_image(const char *image_name);

            void enable_cover_button(uint32_t index);
            void disable_cover_button(uint32_t index);
            void set_always_render_cover_button(uint32_t index, bool always_render = false);

            void show_slot_base_image(uint32_t index);
            void hide_slot_base_image(uint32_t index);
            auto has_slot(uint32_t index) -> bool;

            void clear_all_slot();
            void clear_slot(uint32_t index);
            void set_slot(uint32_t index, uint32_t virtual_number, uint8_t width_value, uint8_t height, graphic_image *image, math::color &diffuse_color);
            void set_slot_count(uint32_t index, uint32_t count);
            void set_slot_count_new(uint32_t index, uint32_t grade, uint32_t count);
            void set_slot_cool_time(uint32_t index, float cool_time, float elapsed_time = 0.0f);
            void activate_slot(uint32_t index);
            void deactivate_slot(uint32_t index);
            void refresh_slot();

            auto slot_count() -> uint32_t;

            void lock_slot(uint32_t index);
            void unlock_slot(uint32_t index);
            void enable_slot(uint32_t index);
            void disable_slot(uint32_t index);

            void show_slot_button(uint32_t slot_number);
            void hide_all_slot_button();
            void on_pressed_slot_button(uint32_t type, uint32_t slot_number, bool is_left = true);

            void show_requirement_sign(uint32_t slot_number);
            void hide_requirement_sign(uint32_t slot_number);

            auto on_over_in_item(uint32_t slot_number) -> bool;
            void on_over_out_item();

            // For Usable Item
            void set_use_mode(bool flag);
            void set_usable_item(bool flag);

            void reserve_destroy_cool_time_finish_effect(uint32_t slot_index);

          protected:
            void initialize_impl();
            void create_slot_enable_effect();
            void create_finish_cool_time_effect(slot *slot_value);
            void create_base_image(const char *file_name, float fr, float fg, float fb, float fa);

            void destroy_slot_enable_effect();
            void destroy_finish_cool_time_effect(slot *slot_value);
            void destroy_base_image();

            void on_update() override;
            void on_render() override;
            auto on_mouse_left_button_down() -> bool override;
            auto on_mouse_left_button_up() -> bool override;
            auto on_mouse_right_button_down() -> bool override;
            auto on_mouse_left_button_double_click() -> bool override;
            void on_mouse_over_out() override;
            void on_mouse_over() override;
            void render_slot_base_image();
            void render_locked_slot();
            virtual void on_render_picking_slot();

            void on_select_empty_slot(int slot_number);
            void on_select_item_slot(int slot_number);
            void on_unselect_empty_slot(int slot_number);
            void on_unselect_item_slot(int slot_number);
            void on_use_slot();

            auto slot_pointer(uint32_t index, slot **slot_value) -> bool;
            auto selected_slot_pointer(slot **slot_value) -> bool;
            virtual auto picked_slot_pointer(slot **slot_value) -> bool;
            void clear_slot(slot *slot_value);
            virtual void on_refresh_slot();

            auto on_is_type(uint32_t type) -> bool override;

          protected:
            uint32_t slot_style_value_;
            slot_list slot_list_value_;
            uint32_t tool_tip_slot_number_;

            bool is_use_mode_;
            bool is_usable_item_;

            graphic_image_instance *base_image_instance_;
            ani_image_box *slot_active_effect_;
            std::deque<uint32_t> reserve_destroy_effect_deque_;
        };
    };
}
