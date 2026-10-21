#pragma once
#include <cstdint>
#include <metin2/gpu/types.h>
#include <metin2/graphics/grp_screen.h>
#include <string>
#include "map_type.h"

namespace metin2::inline world::inline maps {

    class map_base : public screen {
      public:
        enum emaptype {
            MAPTYPE_INVALID,
            MAPTYPE_INDOOR,
            MAPTYPE_OUTDOOR,
        };

      public:
        map_base();
        ~map_base() override;

        virtual void clear();
        virtual auto initialize() -> bool = 0;
        virtual auto destroy() -> bool = 0;
        virtual auto load(float x, float y, float z) -> bool = 0;

        virtual auto update(float x, float y, float z) -> bool = 0;
        virtual void update_around_ambience(float x, float y, float z) = 0;
        virtual auto height(float fx, float fy) -> float = 0;
        virtual void on_begin_environment() = 0; // Called when rendering; sets up the environment state.

        virtual void apply_light(uintptr_t version, const gpu::light &light) = 0;

      protected:
        virtual void on_render() = 0;
        virtual void on_set_environment_data_ptr() = 0;
        virtual void on_reset_environment_data_ptr() = 0;

      public:
        void render();
        void set_environment_data_ptr(const environment_data *environment_data);
        void reset_environment_data_ptr(const environment_data *environment_data);
        auto enter() -> bool;
        auto leave() -> bool;
        auto is_ready() -> bool { return ready_; }

        auto load_property() -> bool;

        auto get_type() const -> emaptype { return type_value_; }
        void set_type(emaptype type) { type_value_ = type; }

        auto get_name() const -> const std::string & { return name_value_; }
        void set_name(const std::string &cr_name) { name_value_ = cr_name; }

        auto is_copied_map() const -> bool { return parent_map_name_.length() > 0; }
        auto map_data_directory() const -> const std::string & { return is_copied_map() ? parent_map_name_ : name_value_; }

      protected:
        emaptype type_value_;
        std::string name_value_;
        std::string parent_map_name_; // When set, all map data is read from the parent map's directory.
        bool ready_;

        const environment_data *mc_environment_data_;
    };
}
