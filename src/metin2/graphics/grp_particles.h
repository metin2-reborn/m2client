#pragma once

#include "grp_image.h"
#include "resource.h"

#include <string>
#include <string_view>

namespace metin2::inline graphics {

    // What a 2D particle emitter does (particle_emitter): sprites born in an area of its box that
    // fly with a velocity, an acceleration, drag and turbulence, turn, and change colour, size and
    // opacity over their life. TOML, [asset] type "Particles"; every size is in interface pixels:
    //
    //   [particles]
    //   image = "ui:particles/ember.png"  # the sprite, an image or a sub image
    //   blend = "add"                     # "alpha" (default) or "add" (light, for sparks and glows)
    //   rate = 12.0                       # born per second
    //   lifetime = [1.5, 3.0]             # seconds, drawn from the range for each particle
    //   origin = [0.5, 1.0]               # the centre of the birth area as fractions of the box
    //   spread = [20, 4]                  # the birth area around it, width and height
    //   fill = false                      # true: born anywhere in the box instead
    //   velocity = [0, -30]               # at birth, pixels per second (y grows downwards) ...
    //   velocity_spread = [8, 12]         # ... plus up to this much either way
    //   acceleration = [0, -10]           # pixels per second squared
    //   drag = 0.2                        # the fraction of the velocity lost per second
    //   turbulence = 20                   # a wandering acceleration, pixels per second squared
    //   size = [2, 5]                     # at birth, drawn from the range
    //   size_end = 0.3                    # the size at death relative to birth
    //   color = [1.0, 0.7, 0.3, 1.0]      # at birth (r, g, b, a) ...
    //   color_end = [1.0, 0.2, 0.0, 1.0]  # ... and at death
    //   fade_in = 0.1                     # the parts of the life the opacity ramps up and down in
    //   fade_out = 0.5
    //   rotation = [0, 360]               # degrees at birth
    //   spin = [-90, 90]                  # degrees per second
    //   prewarm = true                    # starts as if it had been running for a while
    struct particle_settings {
        enum blend_mode {
            BLEND_ALPHA,
            BLEND_ADD,
        };

        std::string image;
        int blend = BLEND_ALPHA;
        float rate = 0.0f;
        float lifetime_min = 1.0f;
        float lifetime_max = 1.0f;
        float origin_x = 0.5f;
        float origin_y = 0.5f;
        float spread_x = 0.0f;
        float spread_y = 0.0f;
        bool fill = false;
        float velocity_x = 0.0f;
        float velocity_y = 0.0f;
        float velocity_spread_x = 0.0f;
        float velocity_spread_y = 0.0f;
        float acceleration_x = 0.0f;
        float acceleration_y = 0.0f;
        float drag = 0.0f;
        float turbulence = 0.0f;
        float size_min = 1.0f;
        float size_max = 1.0f;
        float size_end = 1.0f;
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        float color_end[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        float fade_in = 0.0f;
        float fade_out = 0.0f;
        float rotation_min = 0.0f;
        float rotation_max = 0.0f;
        float spin_min = 0.0f;
        float spin_max = 0.0f;
        bool prewarm = true;

        // Parses the asset text; false (logged) when it is not a valid Particles asset.
        static auto load(std::string_view text, std::string_view name, particle_settings &settings) -> bool;
    };

    // The asset: its settings and the sprite image.
    class graphic_particles : public resource {
      public:
        using ref = ref_ptr<graphic_particles>;

        static auto type() -> type_type;

        explicit graphic_particles(const char *file_name);
        ~graphic_particles() override;

        auto settings() const -> const particle_settings & { return settings_; }
        auto image() -> graphic_image * { return image_.pointer(); }

      protected:
        auto on_load(int size, const void *buf) -> bool override;
        void on_clear() override;
        auto on_is_empty() const -> bool override;
        auto on_is_type(type_type type) -> bool override;

      private:
        particle_settings settings_;
        graphic_image::ref image_;
        bool loaded_ = false;
    };
}
