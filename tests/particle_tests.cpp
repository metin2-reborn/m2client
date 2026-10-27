#include "check.h"

#include <metin2/graphics/grp_particle_emitter.h>
#include <metin2/graphics/grp_particles.h>

#include <algorithm>
#include <cmath>
#include <string>

using namespace metin2;

namespace {
    const char *const EMBERS = "[asset]\ntype = \"Particles\"\n[particles]\nimage = \"ui:particles/spark.png\"\nblend = \"add\"\nrate = 12\n"
                               "lifetime = [1.5, 3.0]\norigin = [0.5, 1.0]\nspread = [20, 4]\nvelocity = [0, -30]\nvelocity_spread = [8, 12]\n"
                               "acceleration = [0, -10]\ndrag = 0.2\nturbulence = 20\nsize = [2, 5]\nsize_end = 0.3\ncolor = [1.0, 0.7, 0.3, 1.0]\n"
                               "color_end = [1.0, 0.2, 0.0, 1.0]\nfade_in = 0.1\nfade_out = 0.5\nrotation = [0, 360]\nspin = [-90, 90]\nprewarm = false\n";
}

TEST(particles_read_every_setting) {
    particle_settings settings;
    CHECK(particle_settings::load(EMBERS, "embers.toml", settings));
    CHECK_EQ(settings.image, std::string("ui:particles/spark.png"));
    CHECK_EQ(settings.blend, int(particle_settings::BLEND_ADD));
    CHECK_EQ(settings.rate, 12.0f);
    CHECK_EQ(settings.lifetime_min, 1.5f);
    CHECK_EQ(settings.lifetime_max, 3.0f);
    CHECK_EQ(settings.origin_y, 1.0f);
    CHECK_EQ(settings.spread_x, 20.0f);
    CHECK_EQ(settings.velocity_y, -30.0f);
    CHECK_EQ(settings.velocity_spread_y, 12.0f);
    CHECK_EQ(settings.acceleration_y, -10.0f);
    CHECK_EQ(settings.drag, 0.2f);
    CHECK_EQ(settings.turbulence, 20.0f);
    CHECK_EQ(settings.size_min, 2.0f);
    CHECK_EQ(settings.size_end, 0.3f);
    CHECK_EQ(settings.color[1], 0.7f);
    CHECK_EQ(settings.color_end[1], 0.2f);
    CHECK_EQ(settings.fade_out, 0.5f);
    CHECK_EQ(settings.rotation_max, 360.0f);
    CHECK_EQ(settings.spin_min, -90.0f);
    CHECK(!settings.prewarm);
}

TEST(particles_default_the_optional_settings) {
    particle_settings settings;
    CHECK(particle_settings::load("[asset]\ntype = \"Particles\"\n[particles]\nimage = \"a.png\"\nrate = 1\nlifetime = [1, 1]\nsize = [4, 4]\ncolor = [0.5, 0.5, 0.5, 1.0]\n", "a.toml", settings));
    CHECK_EQ(settings.blend, int(particle_settings::BLEND_ALPHA));
    CHECK_EQ(settings.origin_x, 0.5f);
    CHECK_EQ(settings.size_end, 1.0f);
    CHECK(!settings.fill);
    CHECK_EQ(settings.color_end[0], 0.5f); // the end colour is the start colour unless given
    CHECK(settings.prewarm);
}

TEST(particles_reject_bad_scripts) {
    particle_settings settings;
    // Another asset type, a missing sprite, a reversed range, a lifetime of zero, an unknown blend, a negative rate.
    CHECK(!particle_settings::load("[asset]\ntype = \"NinePatch\"\n[particles]\nimage = \"a.png\"\nrate = 1\nlifetime = [1, 1]\nsize = [1, 1]\n", "a.toml", settings));
    CHECK(!particle_settings::load("[asset]\ntype = \"Particles\"\n[particles]\nrate = 1\nlifetime = [1, 1]\nsize = [1, 1]\n", "a.toml", settings));
    CHECK(!particle_settings::load("[asset]\ntype = \"Particles\"\n[particles]\nimage = \"a.png\"\nrate = 1\nlifetime = [2, 1]\nsize = [1, 1]\n", "a.toml", settings));
    CHECK(!particle_settings::load("[asset]\ntype = \"Particles\"\n[particles]\nimage = \"a.png\"\nrate = 1\nlifetime = [0, 1]\nsize = [1, 1]\n", "a.toml", settings));
    CHECK(!particle_settings::load("[asset]\ntype = \"Particles\"\n[particles]\nimage = \"a.png\"\nblend = \"multiply\"\nrate = 1\nlifetime = [1, 1]\nsize = [1, 1]\n", "a.toml", settings));
    CHECK(!particle_settings::load("[asset]\ntype = \"Particles\"\n[particles]\nimage = \"a.png\"\nrate = -1\nlifetime = [1, 1]\nsize = [1, 1]\n", "a.toml", settings));
    // A rejected script leaves the settings alone.
    CHECK_EQ(settings.image, std::string());
}

TEST(emitter_births_at_the_rate_and_buries_at_the_lifetime) {
    particle_settings settings;
    CHECK(particle_settings::load(EMBERS, "embers.toml", settings));
    settings.lifetime_min = settings.lifetime_max = 1.0f;
    settings.turbulence = 0.0f;
    particle_emitter emitter;
    emitter.set_settings(settings);
    emitter.set_box(100.0f, 200.0f, 60.0f, 40.0f);
    CHECK_EQ(emitter.count(), size_t(0)); // not prewarmed
    for (int i = 0; i < 30; ++i) {
        emitter.update(1.0f / 60.0f); // half a second: 6 born
    }
    CHECK_EQ(emitter.count(), size_t(6));
    // Born around the origin at the bottom centre of the box, in box coordinates.
    for (const particle_emitter::particle &p : emitter.particles()) {
        CHECK(p.x > 30.0f - 10.0f - 20.0f && p.x < 30.0f + 10.0f + 20.0f);
        CHECK(p.y < 40.0f + 2.0f);
        CHECK(p.velocity_y < 0.0f); // upwards
    }
    for (int i = 0; i < 66; ++i) {
        emitter.update(1.0f / 60.0f); // at 1.6 s: 19 born, the 7 born before 0.6 s are gone
    }
    CHECK_EQ(emitter.count(), size_t(12));
    emitter.reset();
    CHECK_EQ(emitter.count(), size_t(0));
}

TEST(emitter_prewarms_to_a_steady_state) {
    particle_settings settings;
    CHECK(particle_settings::load(EMBERS, "embers.toml", settings));
    settings.prewarm = true;
    particle_emitter emitter;
    emitter.set_settings(settings);
    // Around rate * mean lifetime live at once (12 * 2.25 = 27) once the first ones have died.
    CHECK(emitter.count() >= size_t(18));
    CHECK(emitter.count() <= size_t(36));
    bool aged = false;
    for (const particle_emitter::particle &p : emitter.particles()) {
        aged = aged || p.age > 1.0f;
    }
    CHECK(aged);
}

TEST(emitter_moves_particles_by_their_velocity) {
    particle_settings settings;
    CHECK(particle_settings::load(EMBERS, "embers.toml", settings));
    settings.velocity_x = 10.0f;
    settings.velocity_y = 0.0f;
    settings.velocity_spread_x = settings.velocity_spread_y = 0.0f;
    settings.acceleration_x = settings.acceleration_y = 0.0f;
    settings.drag = 0.0f;
    settings.turbulence = 0.0f;
    settings.spread_x = settings.spread_y = 0.0f;
    settings.rate = 1.0f;
    settings.lifetime_min = settings.lifetime_max = 10.0f;
    particle_emitter emitter;
    emitter.set_settings(settings);
    emitter.set_box(0.0f, 0.0f, 100.0f, 50.0f);
    emitter.update(1.0f); // one born at (50, 50)
    CHECK_EQ(emitter.count(), size_t(1));
    emitter.update(0.5f);
    const particle_emitter::particle &p = emitter.particles().front();
    CHECK(std::fabs(p.x - 55.0f) < 0.01f);
    CHECK(std::fabs(p.y - 50.0f) < 0.01f);
}

TEST(emitter_fills_the_box_when_asked) {
    particle_settings settings;
    CHECK(particle_settings::load(EMBERS, "embers.toml", settings));
    settings.fill = true;
    settings.rate = 200.0f;
    particle_emitter emitter;
    emitter.set_settings(settings);
    emitter.set_box(0.0f, 0.0f, 300.0f, 100.0f);
    emitter.update(1.0f);
    CHECK_EQ(emitter.count(), size_t(200));
    float min_x = 300.0f;
    float max_x = 0.0f;
    for (const particle_emitter::particle &p : emitter.particles()) {
        min_x = std::min(min_x, p.x);
        max_x = std::max(max_x, p.x);
    }
    // Born all over the box, not just around the origin (a second's flight moves them little).
    CHECK(min_x < 60.0f);
    CHECK(max_x > 240.0f);
}

TEST(emitter_opacity_follows_the_fades) {
    particle_settings settings;
    settings.fade_in = 0.25f;
    settings.fade_out = 0.5f;
    CHECK(std::fabs(particle_emitter::opacity(settings, 0.0f)) < 0.001f);
    CHECK(std::fabs(particle_emitter::opacity(settings, 0.125f) - 0.5f) < 0.001f);
    CHECK(std::fabs(particle_emitter::opacity(settings, 0.4f) - 1.0f) < 0.001f);
    CHECK(std::fabs(particle_emitter::opacity(settings, 0.75f) - 0.5f) < 0.001f);
    CHECK(std::fabs(particle_emitter::opacity(settings, 1.0f)) < 0.001f);
    particle_settings plain;
    CHECK_EQ(particle_emitter::opacity(plain, 0.9f), 1.0f);
}
