#pragma once

#include "grp_particles.h"

#include <cstddef>
#include <vector>

namespace metin2::inline graphics {

    // A running particle effect in a box of the interface (grp_particles.h describes the
    // settings). The simulation needs no device, so it can be tested; render() draws the live
    // particles as sprites with the sprite's blend mode and leaves the render states as they were.
    class particle_emitter {
      public:
        // Starts over with these settings (prewarmed when they ask for it); the sprite may be set
        // later or be null, then nothing is drawn.
        void set_settings(const particle_settings &settings);
        void set_image(graphic_image *image);
        // The box in interface pixels: the birth area is placed in it.
        void set_box(float x, float y, float width, float height);
        // Drops every particle and restarts (prewarmed again when the settings ask for it).
        void reset();

        void update(float elapsed_seconds);
        void render();

        auto count() const -> size_t { return particles_.size(); }

        struct particle {
            float x = 0.0f;
            float y = 0.0f;
            float velocity_x = 0.0f;
            float velocity_y = 0.0f;
            float age = 0.0f;
            float lifetime = 1.0f;
            float size = 1.0f;
            float rotation = 0.0f; // radians
            float spin = 0.0f;     // radians per second
            float seed = 0.0f;     // phase of this particle's turbulence
        };
        auto particles() const -> const std::vector<particle> & { return particles_; }

        // The opacity of a particle at the fraction t of its life, from the fade settings.
        static auto opacity(const particle_settings &settings, float t) -> float;

      private:
        void emit();
        void step(float elapsed_seconds);
        void prewarm();

        particle_settings settings_;
        graphic_image::ref image_;
        float x_ = 0.0f;
        float y_ = 0.0f;
        float width_ = 0.0f;
        float height_ = 0.0f;
        float birth_debt_ = 0.0f; // fractional particles owed since the last frame
        float time_ = 0.0f;
        std::vector<particle> particles_;
    };
}
