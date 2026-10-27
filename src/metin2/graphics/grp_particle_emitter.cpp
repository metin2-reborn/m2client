#include "grp_particle_emitter.h"
#include <metin2/core/random.h>
#include <metin2/gpu/types.h>
#include "grp_base.h"
#include "state_manager.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace metin2::inline graphics {

    namespace {
        constexpr float DEGREES = std::numbers::pi_v<float> / 180.0f;
        constexpr float PREWARM_STEP = 1.0f / 30.0f;

        auto pack(const float (&a)[4], const float (&b)[4], float t, float opacity) -> uint32_t {
            auto channel = [&](int i) -> uint32_t {
                const float v = std::clamp(a[i] + (b[i] - a[i]) * t, 0.0f, 1.0f);
                return static_cast<uint32_t>(v * 255.0f + 0.5f);
            };
            const auto alpha = static_cast<uint32_t>(std::clamp(a[3] + (b[3] - a[3]) * t, 0.0f, 1.0f) * opacity * 255.0f + 0.5f);
            return (alpha << 24) | (channel(0) << 16) | (channel(1) << 8) | channel(2);
        }
    }

    void particle_emitter::set_settings(const particle_settings &settings) {
        settings_ = settings;
        reset();
    }

    void particle_emitter::set_image(graphic_image *image) {
        image_ = image;
    }

    void particle_emitter::set_box(float x, float y, float width, float height) {
        x_ = x;
        y_ = y;
        width_ = width;
        height_ = height;
    }

    void particle_emitter::reset() {
        particles_.clear();
        birth_debt_ = 0.0f;
        time_ = 0.0f;
        if (settings_.prewarm) {
            prewarm();
        }
    }

    void particle_emitter::prewarm() {
        // The oldest possible particle has lived a whole life by the first frame.
        for (float t = 0.0f; t < settings_.lifetime_max; t += PREWARM_STEP) {
            step(PREWARM_STEP);
        }
    }

    auto particle_emitter::opacity(const particle_settings &settings, float t) -> float {
        float value = 1.0f;
        if (settings.fade_in > 0.0f) {
            value = std::min(value, t / settings.fade_in);
        }
        if (settings.fade_out > 0.0f) {
            value = std::min(value, (1.0f - t) / settings.fade_out);
        }
        return std::clamp(value, 0.0f, 1.0f);
    }

    void particle_emitter::emit() {
        particle &p = particles_.emplace_back();
        if (settings_.fill) {
            p.x = random::fnumber(0.0f, width_);
            p.y = random::fnumber(0.0f, height_);
        } else {
            p.x = width_ * settings_.origin_x + random::fnumber(-settings_.spread_x, settings_.spread_x) / 2.0f;
            p.y = height_ * settings_.origin_y + random::fnumber(-settings_.spread_y, settings_.spread_y) / 2.0f;
        }
        p.velocity_x = settings_.velocity_x + random::fnumber(-settings_.velocity_spread_x, settings_.velocity_spread_x);
        p.velocity_y = settings_.velocity_y + random::fnumber(-settings_.velocity_spread_y, settings_.velocity_spread_y);
        p.lifetime = random::fnumber(settings_.lifetime_min, settings_.lifetime_max);
        p.size = random::fnumber(settings_.size_min, settings_.size_max);
        p.rotation = random::fnumber(settings_.rotation_min, settings_.rotation_max) * DEGREES;
        p.spin = random::fnumber(settings_.spin_min, settings_.spin_max) * DEGREES;
        p.seed = random::fnumber(0.0f, 100.0f);
    }

    void particle_emitter::update(float elapsed_seconds) {
        step(std::max(0.0f, elapsed_seconds));
    }

    void particle_emitter::step(float dt) {
        time_ += dt;
        const float keep = std::max(0.0f, 1.0f - settings_.drag * dt);
        for (particle &p : particles_) {
            p.age += dt;
            // The turbulence wanders smoothly: two unrelated waves per particle, offset by its seed.
            float ax = settings_.acceleration_x;
            float ay = settings_.acceleration_y;
            if (settings_.turbulence > 0.0f) {
                ax += settings_.turbulence * std::sin(p.age * 2.3f + p.seed * 6.28f);
                ay += settings_.turbulence * std::cos(p.age * 1.7f + p.seed * 12.6f);
            }
            p.velocity_x = (p.velocity_x + ax * dt) * keep;
            p.velocity_y = (p.velocity_y + ay * dt) * keep;
            p.x += p.velocity_x * dt;
            p.y += p.velocity_y * dt;
            p.rotation += p.spin * dt;
        }
        std::erase_if(particles_, [](const particle &p) -> bool { return p.age >= p.lifetime; });

        birth_debt_ += settings_.rate * dt;
        while (birth_debt_ >= 1.0f) {
            birth_debt_ -= 1.0f;
            emit();
        }
    }

    void particle_emitter::render() {
        graphic_image *image = image_.pointer();
        if (!image || image->is_empty() || particles_.empty()) {
            return;
        }
        graphic_texture *texture = image->texture_pointer();
        if (!texture) {
            return;
        }
        const rectangle &rect = image->rect_reference();
        const float u0 = float(rect.left) / float(texture->width());
        const float v0 = float(rect.top) / float(texture->height());
        const float u1 = float(rect.right) / float(texture->width());
        const float v1 = float(rect.bottom) / float(texture->height());
        const float half = graphic_base::half_pixel();

        static std::vector<pdt_vertex> vertices;
        vertices.clear();
        vertices.reserve(particles_.size() * 6);
        for (const particle &p : particles_) {
            const float t = std::clamp(p.age / p.lifetime, 0.0f, 1.0f);
            const float size = p.size * (1.0f + (settings_.size_end - 1.0f) * t);
            const uint32_t color = pack(settings_.color, settings_.color_end, t, opacity(settings_, t));
            const float cx = x_ + p.x - half;
            const float cy = y_ + p.y - half;
            const float c = std::cos(p.rotation) * size / 2.0f;
            const float s = std::sin(p.rotation) * size / 2.0f;
            // The corners of the sprite turned about its centre: (±1, ±1) rotated.
            auto corner = [&](float dx, float dy, float u, float v) -> void {
                pdt_vertex &out = vertices.emplace_back();
                out.position_value = {cx + dx * c - dy * s, cy + dx * s + dy * c, 0.0f};
                out.diffuse_value = color;
                out.tex_coord = {u, v};
            };
            corner(-1.0f, -1.0f, u0, v0);
            corner(-1.0f, 1.0f, u0, v1);
            corner(1.0f, -1.0f, u1, v0);
            corner(1.0f, -1.0f, u1, v0);
            corner(-1.0f, 1.0f, u0, v1);
            corner(1.0f, 1.0f, u1, v1);
        }

        STATEMANAGER.set_texture(0, texture->gpu_texture());
        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
        // Sprites are drawn far from their own size, so they are filtered; the interface's point
        // sampling comes back afterwards, as does its blending.
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);
        const bool additive = settings_.blend == particle_settings::BLEND_ADD;
        if (additive) {
            STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::ONE);
        }
        constexpr size_t MAX_VERTICES = 4096 * 6;
        for (size_t at = 0; at < vertices.size(); at += MAX_VERTICES) {
            const size_t count = std::min(MAX_VERTICES, vertices.size() - at);
            STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_LIST, static_cast<uint32_t>(count / 3), vertices.data() + at, sizeof(pdt_vertex));
        }
        if (additive) {
            STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
        }
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MAG_FILTER);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MIN_FILTER);
    }
}
